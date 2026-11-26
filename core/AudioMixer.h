#pragma once
// Software mixer behind AudioManager: decodes WAV files to one common format and mixes any number
// of voices into a float stereo buffer. It knows nothing about audio devices (AudioManager feeds
// its output to SDL), so it can be tested by rendering to memory.
//
// Everything is converted at load time to 16-bit stereo at kMixRate. That keeps the memory of the
// bundled music (about 215 MB of WAV) near its size on disk, and makes sample-accurate things
// trivial: stems of one track start in the same Render call and stay aligned, an intro runs
// straight into its loop without a gap.
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sdlgf {
	inline constexpr int kMixRate = 48000;

	/// Interleaved 16-bit stereo samples at kMixRate.
	struct Sound {
		std::vector<std::int16_t> samples;
		std::size_t Frames() const { return samples.size() / 2; }
	};

	/// Reads a WAV file (8/16/24/32-bit PCM, float, ADPCM; any channel count SDL supports) and
	/// converts it. Returns null and fills `error` on failure. `utf8Path` is relative to the
	/// working directory.
	std::shared_ptr<const Sound> LoadSoundFile(const std::string& utf8Path, std::string* error = nullptr);

	/// One playing instance of a sound. Created and owned by a Mixer; the pointer stays valid
	/// until Mixer::RemoveVoice.
	struct Voice {
		std::shared_ptr<const Sound> intro; // played once before `main`, may be null
		std::shared_ptr<const Sound> main;
		bool loop = false;                  // `main` repeats forever (the intro does not)
		float gain = 1.0f;
		bool started = false;
		bool finished = false;
		std::size_t position = 0;           // frames played of intro + main
	};

	class Mixer {
	public:
		/// Adds a voice. A voice that is not started yet is silent until Start/StartTogether.
		Voice* AddVoice(std::shared_ptr<const Sound> main, std::shared_ptr<const Sound> intro, bool loop, float gain, bool start);
		/// Starts all of these voices in the same Render call, so they play sample-aligned.
		void StartTogether(const std::vector<Voice*>& voices);
		void SetGain(Voice* voice, float gain);
		/// Silences a voice for good; it reports finished and can be removed.
		void Stop(Voice* voice);
		bool IsFinished(const Voice* voice) const;
		void RemoveVoice(Voice* voice);
		std::size_t VoiceCount() const;
		void Clear();

		/// Overwrites `frames` stereo float frames (interleaved) with the sum of all started
		/// voices. Safe to call from the audio thread while the main thread uses the other methods.
		void Render(float* out, int frames);

	private:
		mutable std::mutex mutex;
		std::vector<std::unique_ptr<Voice>> voices;
	};
}
