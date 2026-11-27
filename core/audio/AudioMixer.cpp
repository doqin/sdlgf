#include "pch.h"
#include "AudioMixer.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace {
	bool HasExtension(const std::string& path, const char* ext)
	{
		const size_t n = std::strlen(ext);
		if (path.size() < n) return false;
		for (size_t i = 0; i < n; ++i) {
			const char c = path[path.size() - n + i];
			if (std::tolower(static_cast<unsigned char>(c)) != ext[i]) return false;
		}
		return true;
	}

	// Decodes a whole file through SDL_mixer's decoder (Ogg Vorbis, ...) straight into the mixer's
	// format, so no separate conversion pass is needed. The decoder resamples and remixes itself.
	std::shared_ptr<const sdlgf::Sound> DecodeWithMixer(const std::string& utf8Path, std::string* error)
	{
		const auto fail = [&](const std::string& what) -> std::shared_ptr<const sdlgf::Sound> {
			if (error) *error = what;
			return nullptr;
		};
		// Initialised once and kept for the life of the process (decoders are stateless between
		// files; tearing the library down after every file only costs time). Loading happens on the
		// main thread, and MIX_Init is not thread safe.
		static const bool initialised = MIX_Init();
		if (!initialised) {
			return fail(std::string("cannot initialise SDL_mixer: ") + SDL_GetError());
		}
		// Open the file ourselves and keep ownership of the stream: when SDL_mixer rejects a file
		// (not Ogg, corrupt) MIX_CreateAudioDecoder(path) leaves the handle open, which on Windows
		// then blocks deleting or replacing the file for the rest of the process.
		SDL_IOStream* io = SDL_IOFromFile(utf8Path.c_str(), "rb");
		if (!io) {
			return fail(std::string("cannot read audio: ") + SDL_GetError());
		}
		MIX_AudioDecoder* decoder = MIX_CreateAudioDecoder_IO(io, false, 0);
		if (!decoder) {
			const std::string reason = SDL_GetError();
			SDL_CloseIO(io);
			return fail("cannot read audio: " + reason);
		}

		const SDL_AudioSpec target{ SDL_AUDIO_S16, 2, sdlgf::kMixRate };
		auto sound = std::make_shared<sdlgf::Sound>();
		// Collect the decoded chunks and copy them into ONE exactly-sized buffer at the end. Growing a
		// single vector while decoding keeps up to 2x the track (doubling) and shrink_to_fit then needs
		// a third copy - tens of MB of transient peak for a long track.
		constexpr size_t kChunkSamples = sdlgf::kMixRate; // 0.5 s of stereo per read
		std::vector<std::vector<std::int16_t>> chunks;
		size_t total = 0;
		std::string failure;
		for (;;) {
			std::vector<std::int16_t> chunk(kChunkSamples);
			const int bytes = MIX_DecodeAudio(decoder, chunk.data(), static_cast<int>(chunk.size() * sizeof(std::int16_t)), &target);
			if (bytes < 0) { failure = std::string("cannot decode audio: ") + SDL_GetError(); break; }
			if (bytes == 0) break;
			chunk.resize(bytes / sizeof(std::int16_t));
			chunk.shrink_to_fit();
			total += chunk.size();
			chunks.push_back(std::move(chunk));
		}
		MIX_DestroyAudioDecoder(decoder);
		SDL_CloseIO(io);

		if (!failure.empty()) return fail(failure);
		if (total == 0) return fail("audio file contains no samples");
		sound->samples.reserve(total);
		for (auto& chunk : chunks) {
			sound->samples.insert(sound->samples.end(), chunk.begin(), chunk.end());
			std::vector<std::int16_t>().swap(chunk); // free as we go, so the peak stays near 1x + one chunk
		}
		return sound;
	}
}

std::shared_ptr<const sdlgf::Sound> sdlgf::LoadSoundFile(const std::string& utf8Path, std::string* error)
{
	const auto fail = [&](const std::string& what) -> std::shared_ptr<const Sound> {
		if (error) *error = what;
		return nullptr;
	};

	// Anything that is not a WAV (.ogg, ...) is decoded by SDL_mixer.
	if (!HasExtension(utf8Path, ".wav")) {
		return DecodeWithMixer(utf8Path, error);
	}

	SDL_AudioSpec source{};
	Uint8* data = nullptr;
	Uint32 length = 0;
	if (!SDL_LoadWAV(utf8Path.c_str(), &source, &data, &length)) {
		return fail(std::string("cannot read WAV: ") + SDL_GetError());
	}

	const SDL_AudioSpec target{ SDL_AUDIO_S16, 2, kMixRate };
	Uint8* converted = nullptr;
	int convertedLength = 0;
	const bool ok = SDL_ConvertAudioSamples(&source, data, static_cast<int>(length), &target, &converted, &convertedLength);
	SDL_free(data);
	if (!ok) {
		return fail(std::string("cannot convert WAV: ") + SDL_GetError());
	}

	auto sound = std::make_shared<Sound>();
	sound->samples.resize(static_cast<size_t>(convertedLength) / sizeof(std::int16_t));
	std::memcpy(sound->samples.data(), converted, sound->samples.size() * sizeof(std::int16_t));
	SDL_free(converted);
	return sound;
}

sdlgf::Voice* sdlgf::Mixer::AddVoice(std::shared_ptr<const Sound> main, std::shared_ptr<const Sound> intro, bool loop, float gain, bool start)
{
	auto voice = std::make_unique<Voice>();
	voice->main = std::move(main);
	voice->intro = std::move(intro);
	voice->loop = loop;
	voice->gain = gain;
	voice->started = start;
	Voice* raw = voice.get();
	std::lock_guard<std::mutex> lock(mutex);
	voices.push_back(std::move(voice));
	return raw;
}

void sdlgf::Mixer::StartTogether(const std::vector<Voice*>& toStart)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (Voice* voice : toStart) voice->started = true;
}

void sdlgf::Mixer::SetGain(Voice* voice, float gain)
{
	std::lock_guard<std::mutex> lock(mutex);
	voice->gain = gain;
}

void sdlgf::Mixer::Stop(Voice* voice)
{
	std::lock_guard<std::mutex> lock(mutex);
	voice->finished = true;
}

bool sdlgf::Mixer::IsFinished(const Voice* voice) const
{
	std::lock_guard<std::mutex> lock(mutex);
	return voice->finished;
}

void sdlgf::Mixer::RemoveVoice(Voice* voice)
{
	std::lock_guard<std::mutex> lock(mutex);
	voices.erase(std::remove_if(voices.begin(), voices.end(), [voice](const auto& v) { return v.get() == voice; }), voices.end());
}

std::size_t sdlgf::Mixer::VoiceCount() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return voices.size();
}

void sdlgf::Mixer::Clear()
{
	std::lock_guard<std::mutex> lock(mutex);
	voices.clear();
}

void sdlgf::Mixer::Render(float* out, int frames)
{
	std::fill(out, out + static_cast<size_t>(frames) * 2, 0.0f);
	constexpr float kScale = 1.0f / 32768.0f;

	std::lock_guard<std::mutex> lock(mutex);
	for (auto& owned : voices) {
		Voice& v = *owned;
		if (!v.started || v.finished) continue;

		const size_t introFrames = v.intro ? v.intro->Frames() : 0;
		const size_t mainFrames = v.main ? v.main->Frames() : 0;
		float* dst = out;
		int remaining = frames;
		while (remaining > 0) {
			const std::int16_t* src = nullptr;
			size_t available = 0;
			if (v.position < introFrames) {
				src = v.intro->samples.data() + v.position * 2;
				available = introFrames - v.position;
			}
			else {
				size_t inMain = v.position - introFrames;
				if (inMain >= mainFrames) {
					if (!v.loop || mainFrames == 0) break; // played to the end
					v.position = introFrames;
					inMain = 0;
				}
				src = v.main->samples.data() + inMain * 2;
				available = mainFrames - inMain;
			}
			const size_t n = std::min(available, static_cast<size_t>(remaining));
			const float gain = v.gain * kScale;
			for (size_t i = 0; i < n * 2; ++i) dst[i] += static_cast<float>(src[i]) * gain;
			dst += n * 2;
			v.position += n;
			remaining -= static_cast<int>(n);
		}
		if (!v.loop && v.position >= introFrames + mainFrames) v.finished = true;
		if (v.loop && mainFrames == 0 && v.position >= introFrames) v.finished = true;
	}
}
