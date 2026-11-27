#include "pch.h"
#include "AudioManager.h"
#include "Platform.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>
#include <chrono>

sdlgf::AudioManager* sdlgf::AudioManager::instance = nullptr;

sdlgf::AudioManager* sdlgf::AudioManager::GetInstance()
{
	if (!instance) {
		instance = new AudioManager();
	}
	return instance;
}

void sdlgf::AudioManager::DestroyInstance()
{
	if (instance) {
		instance->Shutdown();
		delete instance;
		instance = nullptr;
	}
}

void SDLCALL sdlgf::AudioManager::StreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount)
{
	// Runs on SDL's audio thread: the device wants `additionalAmount` more bytes
	AudioManager* self = static_cast<AudioManager*>(userdata);
	constexpr int kFrameBytes = 2 * static_cast<int>(sizeof(float));
	int frames = additionalAmount / kFrameBytes;
	static thread_local std::vector<float> scratch;
	while (frames > 0) {
		const int chunk = (std::min)(frames, 4096);
		scratch.resize(static_cast<size_t>(chunk) * 2);
		self->mixer.Render(scratch.data(), chunk);
		SDL_PutAudioStreamData(stream, scratch.data(), chunk * kFrameBytes);
		frames -= chunk;
	}
}

bool sdlgf::AudioManager::Init()
{
	if (stream != nullptr) return true;
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		sdlgf::Platform::DebugLog(std::string("Audio unavailable: ") + SDL_GetError() + "\n");
		return false;
	}
	const SDL_AudioSpec spec{ SDL_AUDIO_F32, 2, kMixRate };
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &AudioManager::StreamCallback, this);
	if (stream == nullptr) {
		sdlgf::Platform::DebugLog(std::string("Could not open an audio device: ") + SDL_GetError() + "\n");
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}
	SDL_ResumeAudioStreamDevice(stream);
	return true;
}

void sdlgf::AudioManager::Load(std::string name, const std::string& path)
{
	if (sounds.count(name)) return;
	SoundEntry entry;
	entry.path = path;
	SDL_PathInfo info;
	// A file that cannot even be stat'ed goes through the decoder too, so the log says why
	const bool small = !SDL_GetPathInfo(path.c_str(), &info) || static_cast<size_t>(info.size) <= residentFileLimit;
	if (small) {
		std::string error;
		auto sound = LoadSoundFile(path, &error);
		if (!sound) {
			sdlgf::Platform::DebugLog("Could not load sound '" + name + "' from '" + path + "': " + error + "\n");
			return;
		}
		entry.weak = sound;
		entry.pinned = std::move(sound);
	}
	sounds[name] = std::move(entry);
}

std::shared_ptr<const sdlgf::Sound> sdlgf::AudioManager::Acquire(const std::string& name)
{
	auto it = sounds.find(name);
	if (it == sounds.end()) return nullptr;
	if (auto sound = it->second.weak.lock()) return sound;
	std::string error;
	auto sound = LoadSoundFile(it->second.path, &error);
	if (!sound) {
		sdlgf::Platform::DebugLog("Could not load sound '" + name + "' from '" + it->second.path + "': " + error + "\n");
		return nullptr;
	}
	it->second.weak = sound;
	return sound;
}

size_t sdlgf::AudioManager::GetLoadedSoundBytes() const
{
	size_t bytes = 0;
	for (const auto& [name, entry] : sounds) {
		if (auto sound = entry.weak.lock()) bytes += sound->samples.size() * sizeof(std::int16_t);
	}
	return bytes;
}

bool sdlgf::AudioManager::IsFinished(const ActiveVoice* av) const
{
	return av->stopped || mixer.IsFinished(av->voice);
}

void sdlgf::AudioManager::ApplyVoiceVolume(ActiveVoice* av)
{
	float typeVol = (av->type == AudioType::MUSIC) ? currentMusicVolume : currentSfxVolume;
	mixer.SetGain(av->voice, av->baseVolume * av->fadeMul * av->stemLevel * typeVol * currentMasterVolume);
}

void sdlgf::AudioManager::Play(std::string name, bool loop, float volume, AudioType type)
{
	PlayInternal(name, loop, volume, type, name, 1.0f, true);
}

sdlgf::ActiveVoice* sdlgf::AudioManager::PlayInternal(std::string name, bool loop, float volume, AudioType type, std::string group, float fadeMul, bool startNow, const std::string& introName)
{
	//set a limit voice count to protect the engine
	if (activeVoices.size() > 64) {
		return nullptr;
	}

	//can't find sound name from cache
	if (sounds.find(name) == sounds.end()) return nullptr;

	// Init() can fail on machines with no audio device (VMs, RDP sessions,
	// sandbox). stream stays null then - stay silent instead of piling up voices
	if (stream == nullptr) return nullptr;

	//optional intro: played once first, the main sound follows gaplessly
	std::shared_ptr<const Sound> intro;
	if (!introName.empty()) intro = Acquire(introName);

	auto main = Acquire(name);
	if (!main) return nullptr;
	Voice* voice = mixer.AddVoice(std::move(main), std::move(intro), loop, 0.0f, startNow);
	ActiveVoice* av = new ActiveVoice{ name, voice, type, volume };
	av->group = group;
	av->fadeMul = fadeMul;
	av->fadeFrom = fadeMul;
	ApplyVoiceVolume(av);

	//save into list to control it easily
	activeVoices.push_back(av);
	return av;
}

void sdlgf::AudioManager::Update(unsigned long long deltaTime) {

	// A track that was being decoded in the background is ready: install it and start it
	if (pendingBgm && pendingBgm->decoding.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		std::unique_ptr<PendingBgm> pending = std::move(pendingBgm);
		auto decoded = pending->decoding.get();
		for (size_t i = 0; i < decoded.size() && i < pending->names.size(); ++i) {
			auto it = sounds.find(pending->names[i]);
			if (it == sounds.end()) continue;
			if (decoded[i]) it->second.weak = decoded[i];
			else sdlgf::Platform::DebugLog("Could not load sound '" + pending->names[i] + "' from '" + it->second.path + "'\n");
		}
		// `decoded` keeps the sounds alive until the voices created by StartBgm hold them
		if (!pending->cancelled) StartBgm(pending->name, pending->volume, pending->duration);
	}

	if (isFading) {
		fadeTimer += (deltaTime / 1000.0f);

		float progress = fadeTimer / fadeDuration;
		if (progress > 1.0f) progress = 1.0f;

		for (auto av : activeVoices) {
			if (av->type == AudioType::MUSIC && !IsFinished(av)) {

				if (av->group == fadingOutSound) {
					av->fadeMul = av->fadeFrom * (1.0f - progress);
					ApplyVoiceVolume(av);
				}

				//intro music starts at full level so its first notes aren't faded away
				if (av->group == fadingInSound && !introLoops.count(av->group)) {
					av->fadeMul = progress;
					ApplyVoiceVolume(av);
				}
			}
		}

		if (progress >= 1.0f) {
			isFading = false;
			if (fadingOutSound != "") {
				Stop(fadingOutSound);
			}
		}
	}

	//crossfade the stems of any playing stem set towards their active state
	const float dt = deltaTime / 1000.0f;
	for (auto av : activeVoices) {
		if (av->stemIndex < 0 || IsFinished(av)) continue;
		auto setIt = stemSets.find(av->group);
		if (setIt == stemSets.end()) continue;
		const StemSet& set = setIt->second;
		float target = set.active[av->stemIndex] ? 1.0f : 0.0f;
		float step = dt / (std::max)(set.fadeTime, 0.001f);
		if (av->stemT < target) av->stemT = (std::min)(av->stemT + step, target);
		else if (av->stemT > target) av->stemT = (std::max)(av->stemT - step, target);
		//equal-power curve: a stem fading in and one fading out keep constant loudness
		av->stemLevel = std::sin(av->stemT * 1.5707963f);
		ApplyVoiceVolume(av);
	}

	bool musicEnded = false;
	for (auto it = activeVoices.begin(); it != activeVoices.end(); )
	{
		if (IsFinished(*it)) //finished sound should be destroyed
		{
			musicEnded = musicEnded || (*it)->type == AudioType::MUSIC;
			mixer.RemoveVoice((*it)->voice);
			delete (*it);
			it = activeVoices.erase(it);
		}
		else
		{
			++it;
		}
	}
	// A finished track's decoded audio (tens of MB) was just freed: give it back to the OS
	if (musicEnded) sdlgf::Platform::ReleaseFreedMemory();
}

void sdlgf::AudioManager::Stop(std::string name)
{
	if (pendingBgm && pendingBgm->name == name) pendingBgm->cancelled = true;
	for (auto av : activeVoices)
	{
		if ((av->name == name || av->group == name) && !IsFinished(av))
		{
			mixer.Stop(av->voice);
			av->stopped = true;
		}
	}
}

void sdlgf::AudioManager::StopAll()
{
	if (pendingBgm) pendingBgm->cancelled = true;
	for (auto av : activeVoices)
	{
		if (!IsFinished(av))
		{
			mixer.Stop(av->voice);
			av->stopped = true;
		}
	}
}

void sdlgf::AudioManager::Shutdown() {
	//closing the device first guarantees the audio thread is no longer inside the mixer
	if (stream != nullptr) {
		SDL_DestroyAudioStream(stream);
		stream = nullptr;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}

	//destroy playing sound
	for (auto av : activeVoices) delete av;
	activeVoices.clear();
	mixer.Clear();
	soundBanks.clear();
	pendingBgm.reset();
	sounds.clear();
}

void sdlgf::AudioManager::SetMasterVolume(float volume)
{
	currentMasterVolume = volume;
	for (auto av : activeVoices)
	{
		ApplyVoiceVolume(av);
	}
}

void sdlgf::AudioManager::SetMusicVolume(float volume)
{
	currentMusicVolume = volume;
	for (auto av : activeVoices)
	{
		if (av->type == AudioType::MUSIC)
		{
			ApplyVoiceVolume(av);
		}
	}
}

void sdlgf::AudioManager::SetSfxVolume(float volume)
{
	currentSfxVolume = volume;
	for (auto av : activeVoices)
	{
		if (av->type == AudioType::SFX)
		{
			ApplyVoiceVolume(av);
		}
	}
}

void sdlgf::AudioManager::RegisterBank(std::string bankName, std::vector<std::string> soundNames)
{
	soundBanks[bankName] = soundNames;
}

void sdlgf::AudioManager::PlayRandom(std::string bankName, float volume, AudioType type)
{
	if (soundBanks.find(bankName) == soundBanks.end() || soundBanks[bankName].empty())
		return;

	int index = rand() % soundBanks[bankName].size();
	std::string selectedSound = soundBanks[bankName][index];

	Play(selectedSound, false, volume, type);
}

std::vector<std::string> sdlgf::AudioManager::SoundsOfTrack(const std::string& name) const
{
	auto stems = stemSets.find(name);
	if (stems != stemSets.end()) return stems->second.stems;
	auto introLoop = introLoops.find(name);
	if (introLoop != introLoops.end()) return { introLoop->second.intro, introLoop->second.loop };
	return { name };
}

void sdlgf::AudioManager::PlayBGM_Fade(std::string name, float targetVolume, float duration)
{
	if (pendingBgm && !pendingBgm->cancelled && pendingBgm->name == name) return; // already on its way
	if (!activeVoices.empty()) {
		for (auto av : activeVoices) {
			if (av->group == name && av->type == AudioType::MUSIC && !IsFinished(av)) return;
		}
	}

	// Music is only in memory while it plays: if any file of this track is not decoded, decode it on a
	// worker thread (a few hundred ms for a long track) and start the track from Update when ready,
	// instead of freezing the frame.
	std::vector<std::string> missing, paths;
	for (const auto& soundName : SoundsOfTrack(name)) {
		auto it = sounds.find(soundName);
		if (it != sounds.end() && !it->second.weak.lock()) {
			missing.push_back(soundName);
			paths.push_back(it->second.path);
		}
	}
	if (missing.empty() || stream == nullptr) {
		pendingBgm.reset();
		StartBgm(std::move(name), targetVolume, duration);
		return;
	}
	auto pending = std::make_unique<PendingBgm>();
	pending->name = name;
	pending->volume = targetVolume;
	pending->duration = duration;
	pending->names = std::move(missing);
	pending->decoding = std::async(std::launch::async, [paths = std::move(paths)]() {
		Platform::SdlThreadCleanup sdlCleanup; // the decoder sets SDL's per-thread error buffer
		std::vector<std::shared_ptr<const Sound>> decoded;
		for (const auto& path : paths) decoded.push_back(LoadSoundFile(path));
		return decoded;
	});
	pendingBgm = std::move(pending); // replaces (and waits for) an older request, the newest one wins
}

void sdlgf::AudioManager::StartBgm(std::string name, float targetVolume, float duration)
{
	if (!activeVoices.empty()) {
		bool isAlreadyPlaying = false;
		for (auto av : activeVoices) {
			if (av->group == name && av->type == AudioType::MUSIC && !IsFinished(av)) {
				isAlreadyPlaying = true;
				break;
			}
		}
		if (isAlreadyPlaying) return;
	}

	isFading = true;
	fadeTimer = 0.0f;
	fadeDuration = duration;
	fadingInSound = name;
	fadingInTargetVolume = targetVolume;

	//the loudest playing music group is the one that fades out
	fadingOutSound = "";
	float loudest = -1.0f;
	for (auto av : activeVoices) {
		if (av->type == AudioType::MUSIC && !IsFinished(av) && av->fadeMul > loudest) {
			loudest = av->fadeMul;
			fadingOutSound = av->group;
		}
	}
	//any other music left over from an interrupted fade is cut
	for (auto av : activeVoices) {
		if (av->type != AudioType::MUSIC || IsFinished(av)) continue;
		if (av->group == fadingOutSound) av->fadeFrom = av->fadeMul;
		else Stop(av->group);
	}

	// Start the incoming track silent right away so it crossfades with the outgoing one
	if (stemSets.count(name)) PlayStemSet(name, targetVolume);
	else if (introLoops.count(name)) {
		const IntroLoop& il = introLoops[name];
		PlayInternal(il.loop, true, targetVolume, AudioType::MUSIC, name, 1.0f, true, il.intro);
	}
	else PlayInternal(name, true, targetVolume, AudioType::MUSIC, name, 0.0f, true);
}

void sdlgf::AudioManager::PlayStemSet(std::string setName, float volume)
{
	const StemSet& set = stemSets[setName];
	std::vector<ActiveVoice*> started;
	for (size_t i = 0; i < set.stems.size(); ++i) {
		ActiveVoice* av = PlayInternal(set.stems[i], true, volume, AudioType::MUSIC, setName, 0.0f, false);
		if (!av) continue;
		av->stemIndex = static_cast<int>(i);
		//start at the current active state so there is no fade-in pop on the stems
		av->stemT = set.active[i] ? 1.0f : 0.0f;
		av->stemLevel = std::sin(av->stemT * 1.5707963f);
		ApplyVoiceVolume(av);
		started.push_back(av);
	}
	//start every stem in the same mixer pass so they are sample-aligned
	std::vector<Voice*> voices;
	for (auto av : started) voices.push_back(av->voice);
	mixer.StartTogether(voices);
}

void sdlgf::AudioManager::RegisterIntroLoop(std::string name, std::string introName, std::string loopName)
{
	introLoops[name] = IntroLoop{ introName, loopName };
}

void sdlgf::AudioManager::RegisterStemSet(std::string setName, std::vector<std::string> stemNames)
{
	StemSet set;
	set.stems = stemNames;
	set.active.assign(stemNames.size(), true);
	stemSets[setName] = set;
}

void sdlgf::AudioManager::SetActiveStems(std::string setName, std::vector<int> activeIndices, float fadeTime)
{
	auto it = stemSets.find(setName);
	if (it == stemSets.end()) return;
	StemSet& set = it->second;
	set.active.assign(set.stems.size(), false);
	for (int idx : activeIndices) {
		if (idx >= 0 && idx < static_cast<int>(set.active.size())) set.active[idx] = true;
	}
	set.fadeTime = fadeTime;
}

void sdlgf::AudioManager::PlayRandomBGM_Fade(std::string bankName, float targetVolume, float duration)
{
	if (soundBanks.find(bankName) == soundBanks.end() || soundBanks[bankName].empty())
		return;

	int index = rand() % soundBanks[bankName].size();
	std::string selectedSound = soundBanks[bankName][index];

	PlayBGM_Fade(selectedSound, targetVolume, duration);
}
