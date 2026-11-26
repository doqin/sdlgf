#include "pch.h"
#include "AudioMixer.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>

std::shared_ptr<const sdlgf::Sound> sdlgf::LoadSoundFile(const std::string& utf8Path, std::string* error)
{
	const auto fail = [&](const std::string& what) -> std::shared_ptr<const Sound> {
		if (error) *error = what;
		return nullptr;
	};

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
