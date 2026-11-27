#pragma once
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "AudioMixer.h"

struct SDL_AudioStream;

namespace sdlgf {
	enum class AudioType 
	{
		SFX,
		MUSIC
	};

	struct ActiveVoice
	{
		std::string name; //identify
		Voice* voice;     //owned by the mixer
		AudioType type;
		float baseVolume; //save the volume level when play
		std::string group; //music fades act on a whole group (a stem set's voices share its name)
		float fadeMul = 1.0f; //crossfade envelope for the whole group
		float fadeFrom = 1.0f; //fadeMul when a fade-out started, so an interrupted fade doesn't jump
		int stemIndex = -1; //>= 0 when this voice is one stem of a stem set
		float stemT = 1.0f; //0..1 progress of this stem towards active
		float stemLevel = 1.0f; //equal-power curve of stemT
		bool stopped = false; //Stop() was called
	};

	//dynamic music: every stem loops in sync, inactive ones sit at silence
	struct StemSet
	{
		std::vector<std::string> stems;
		std::vector<bool> active;
		float fadeTime = 2.0f;
	};

	//music that plays an intro once, then loops the main track
	struct IntroLoop
	{
		std::string intro;
		std::string loop;
	};

	class AudioManager {
	private:
		SDL_AudioStream* stream = nullptr;        //the output device; null when there is no audio device
		Mixer mixer;                              //called from the audio thread, see Init
		// Every registered sound. Small ones (sound effects) are decoded at Load and pinned for good;
		// big ones (music) are decoded when first needed and only stay in memory while something
		// uses them (a playing voice, a pending start): `weak` finds one that is still loaded.
		struct SoundEntry {
			std::string path;
			std::shared_ptr<const Sound> pinned;
			std::weak_ptr<const Sound> weak;
		};
		std::map<std::string, SoundEntry> sounds;
		size_t residentFileLimit = 64 * 1024;     // files up to this many bytes on disk stay decoded
		std::shared_ptr<const Sound> Acquire(const std::string& name); // decodes on the calling thread if needed

		// A background track whose files are being decoded on a worker thread. The music already playing
		// carries on until the new one is ready, then crossfades exactly as PlayBGM_Fade always did.
		struct PendingBgm {
			std::string name;
			float volume;
			float duration;
			bool cancelled = false;                                                // Stop/StopAll came first
			std::vector<std::string> names;                                        // sounds being decoded
			std::future<std::vector<std::shared_ptr<const Sound>>> decoding;
		};
		std::unique_ptr<PendingBgm> pendingBgm;
		std::vector<std::string> SoundsOfTrack(const std::string& name) const;
		void StartBgm(std::string name, float targetVolume, float duration);
		std::map<std::string, std::vector<std::string>> soundBanks;
		std::vector<ActiveVoice*> activeVoices; //list of playing sound
		std::map<std::string, StemSet> stemSets;
		std::map<std::string, IntroLoop> introLoops;

		ActiveVoice* PlayInternal(std::string name, bool loop, float volume, AudioType type, std::string group, float fadeMul, bool startNow, const std::string& introName = "");
		void PlayStemSet(std::string setName, float volume);
		void ApplyVoiceVolume(ActiveVoice* av);
		bool IsFinished(const ActiveVoice* av) const;
		static void StreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount);

		//settings manager will push values to these vars
		float currentMasterVolume = 1.0f;
		float currentMusicVolume = 1.0f;
		float currentSfxVolume = 1.0f;

		bool isFading = false;
		float fadeTimer = 0.0f;
		float fadeDuration = 0.0f;

		std::string fadingOutSound = "";

		std::string fadingInSound = "";
		float fadingInTargetVolume = 1.0f;
		bool fadingInLoop = true;

		AudioManager() {}
		~AudioManager() {}
		static AudioManager* instance;
	public:
		static AudioManager* GetInstance();
		static void DestroyInstance();
		/// Opens the default audio device. Returns false when there is none (the game then runs
		/// silently: every Play call does nothing).
		bool Init();

		/// Registers a WAV or OGG file (UTF-8 path relative to the working directory) under `name`.
		/// Small files are decoded now; big ones (music) when first used, so registering all the
		/// game's music costs no memory. Failures are logged and the name stays unknown, so
		/// Play(name) does nothing.
		void Load(std::string name, const std::string& path);
		bool HasSound(const std::string& name) const { return sounds.count(name) != 0; }
		/// Files up to `bytes` on disk are decoded at Load and stay in memory; larger ones are decoded when
		/// first played and freed once nothing plays them. Applies to later Load calls.
		void SetResidentFileLimit(size_t bytes) { residentFileLimit = bytes; }
		/// Memory of the decoded sounds that are in memory right now (for the debug overlay and tests).
		size_t GetLoadedSoundBytes() const;
		/// True while a PlayBGM_Fade is waiting for its files to finish decoding.
		bool IsMusicLoading() const { return pendingBgm != nullptr; }

		void Play(std::string name, bool loop = false, float volume = 1.0f, AudioType type = AudioType::SFX);
		void Update(unsigned long long deltaTime);
		void Stop(std::string name);
		void StopAll();
		void Shutdown();
		/// Voices currently playing or waiting to be cleaned up by Update (for tests).
		size_t GetActiveVoiceCount() const { return activeVoices.size(); }

		//set game volume
		void SetMasterVolume(float volume);
		void SetMusicVolume(float volume);
		void SetSfxVolume(float volume);
		
		//manage various types of footstep
		void RegisterBank(std::string bankName, std::vector<std::string> soundNames);
		void PlayRandom(std::string bankName, float volume = 1.0f, AudioType type = AudioType::SFX);

		//background music
		void PlayBGM_Fade(std::string name, float targetVolume = 0.5f, float duration = 1.5f);
		void PlayRandomBGM_Fade(std::string bankName, float targetVolume = 0.5f, float duration = 1.5f);

		//intro-then-loop music: register already-loaded tracks, then play with PlayBGM_Fade(name).
		//the intro is heard at full level (no fade-in) and flows seamlessly into the looping track
		void RegisterIntroLoop(std::string name, std::string introName, std::string loopName);

		//dynamic music: register already-loaded, equal-length stems, then play the set with PlayBGM_Fade(setName)
		void RegisterStemSet(std::string setName, std::vector<std::string> stemNames);
		//which stems are audible (indices into the set); can be called before or while the set plays
		void SetActiveStems(std::string setName, std::vector<int> activeIndices, float fadeTime = 2.0f);
	};
}
