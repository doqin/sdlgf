// Tests for the audio path: WAV and OGG decoding/conversion, the software mixer (looping, intro -> loop,
// aligned stems, gain, stop), and AudioManager driving a real SDL audio device (the "dummy"
// driver, which consumes audio in real time without making a sound).
#include "AudioManager.h"
#include "AudioMixer.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <system_error>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

static int g_failures = 0;
#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

using namespace sdlgf;

namespace {
	std::filesystem::path g_dir;

	template <class T> void Put(std::string& out, T value) { out.append(reinterpret_cast<const char*>(&value), sizeof(T)); }

	// Writes a canonical WAV file. `frame(i, channel)` returns the sample as a double in [-1, 1).
	std::string WriteWav(const std::string& name, int rate, int channels, int bits, bool isFloat, int frames,
		const std::function<double(int, int)>& frame)
	{
		std::string data;
		for (int i = 0; i < frames; ++i) {
			for (int c = 0; c < channels; ++c) {
				const double v = frame(i, c);
				if (isFloat) { Put(data, static_cast<float>(v)); }
				else if (bits == 8) { Put(data, static_cast<std::uint8_t>(std::lround(v * 128.0) + 128)); }
				else if (bits == 16) { Put(data, static_cast<std::int16_t>(std::lround(v * 32768.0))); }
				else if (bits == 24) {
					const std::int32_t s = static_cast<std::int32_t>(std::lround(v * 8388608.0));
					data.push_back(static_cast<char>(s & 0xFF));
					data.push_back(static_cast<char>((s >> 8) & 0xFF));
					data.push_back(static_cast<char>((s >> 16) & 0xFF));
				}
			}
		}
		std::string file = "RIFF";
		Put<std::uint32_t>(file, 36 + static_cast<std::uint32_t>(data.size()));
		file += "WAVEfmt ";
		Put<std::uint32_t>(file, 16);
		Put<std::uint16_t>(file, isFloat ? 3 : 1);
		Put<std::uint16_t>(file, static_cast<std::uint16_t>(channels));
		Put<std::uint32_t>(file, static_cast<std::uint32_t>(rate));
		Put<std::uint32_t>(file, static_cast<std::uint32_t>(rate * channels * bits / 8));
		Put<std::uint16_t>(file, static_cast<std::uint16_t>(channels * bits / 8));
		Put<std::uint16_t>(file, static_cast<std::uint16_t>(bits));
		file += "data";
		Put<std::uint32_t>(file, static_cast<std::uint32_t>(data.size()));
		file += data;
		const std::filesystem::path path = g_dir / name;
		std::ofstream(path, std::ios::binary).write(file.data(), static_cast<std::streamsize>(file.size()));
		return path.string();
	}

	// A sound whose left channel counts up from `first` (as a raw 16-bit value), right = -left.
	std::shared_ptr<const Sound> Ramp(int first, int frames)
	{
		auto s = std::make_shared<Sound>();
		for (int i = 0; i < frames; ++i) {
			s->samples.push_back(static_cast<std::int16_t>(first + i));
			s->samples.push_back(static_cast<std::int16_t>(-(first + i)));
		}
		return s;
	}

	// Renders `frames` frames and returns the left channel in raw 16-bit units.
	std::vector<int> RenderLeft(Mixer& mixer, int frames)
	{
		std::vector<float> buffer(static_cast<size_t>(frames) * 2);
		mixer.Render(buffer.data(), frames);
		std::vector<int> left;
		for (int i = 0; i < frames; ++i) left.push_back(static_cast<int>(std::lround(buffer[static_cast<size_t>(i) * 2] * 32768.0f)));
		return left;
	}

	bool Near(int a, int b, int tolerance) { return std::abs(a - b) <= tolerance; }
}

static void TestDecoding()
{
	std::string error;

	// 16-bit stereo at the mix rate is passed through exactly
	{
		auto path = WriteWav("s16.wav", 48000, 2, 16, false, 480, [](int, int c) { return c == 0 ? 1000 / 32768.0 : -2000 / 32768.0; });
		auto sound = LoadSoundFile(path, &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(sound->Frames() == 480);
			CHECK(sound->samples[0] == 1000 && sound->samples[1] == -2000);
			CHECK(sound->samples[958] == 1000 && sound->samples[959] == -2000);
		}
	}
	// 8-bit mono becomes stereo with equal channels
	{
		auto path = WriteWav("u8.wav", 48000, 1, 8, false, 300, [](int, int) { return 0.5; });
		auto sound = LoadSoundFile(path, &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(sound->Frames() == 300);
			CHECK(sound->samples[100] == sound->samples[101]);
			CHECK(Near(sound->samples[100], 16384, 256));
		}
	}
	// 24-bit stereo
	{
		auto path = WriteWav("s24.wav", 48000, 2, 24, false, 300, [](int, int c) { return c == 0 ? 0.5 : -0.25; });
		auto sound = LoadSoundFile(path, &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(sound->Frames() == 300);
			CHECK(Near(sound->samples[100], 16384, 2) && Near(sound->samples[101], -8192, 2));
		}
	}
	// 32-bit float mono (one of the bundled files is float)
	{
		auto path = WriteWav("f32.wav", 48000, 1, 32, true, 300, [](int, int) { return 0.25; });
		auto sound = LoadSoundFile(path, &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(sound->Frames() == 300);
			CHECK(Near(sound->samples[100], 8192, 2) && Near(sound->samples[101], 8192, 2));
		}
	}
	// 44.1 kHz is resampled to the mix rate: length scales, a constant stays constant
	{
		auto path = WriteWav("s16_44k.wav", 44100, 1, 16, false, 4410, [](int, int) { return 1000 / 32768.0; });
		auto sound = LoadSoundFile(path, &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(Near(static_cast<int>(sound->Frames()), 4800, 4));
			CHECK(Near(sound->samples[2400], 1000, 3));
		}
	}
	// failures are reported, not thrown
	{
		error.clear();
		CHECK(LoadSoundFile((g_dir / "does-not-exist.wav").string(), &error) == nullptr);
		CHECK(!error.empty());
		const std::string notWav = (g_dir / "text.wav").string();
		std::ofstream(notWav) << "this is not a wave file";
		error.clear();
		CHECK(LoadSoundFile(notWav, &error) == nullptr);
		CHECK(!error.empty());
	}
}

// Ogg Vorbis goes through SDL_mixer. The fixtures (tests/audio) are libvorbis q4 encodes of sine
// waves: the mono one has amplitude 0.5 (peak ~16384); the stereo one was upmixed by ffmpeg with
// its -3 dB pan law, so it peaks near 0.36 (~11700; ffmpeg's own volumedetect reports -8.9 dB).
// A correct decode also keeps the length of the source.
static void TestOgg()
{
	std::string error;
	const auto peak = [](const Sound& sound) {
		int p = 0;
		for (std::int16_t v : sound.samples) p = (std::max)(p, std::abs(static_cast<int>(v)));
		return p;
	};

	// 44.1 kHz mono, 0.5 s: resampled to the mix rate and widened to stereo
	{
		auto sound = LoadSoundFile(std::string(SDLGF_AUDIO_FIXTURE_DIR) + "/tone_44k_mono.ogg", &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(Near(static_cast<int>(sound->Frames()), 24000, 48));
			CHECK(sound->samples.size() % 2 == 0);
			CHECK(Near(peak(*sound), 16384, 2500));
			bool channelsMatch = true;
			for (size_t i = 0; i + 1 < sound->samples.size(); i += 2) channelsMatch &= sound->samples[i] == sound->samples[i + 1];
			CHECK(channelsMatch);
		}
	}
	// 48 kHz stereo, 0.25 s: already in the mix format
	{
		auto sound = LoadSoundFile(std::string(SDLGF_AUDIO_FIXTURE_DIR) + "/tone_48k_stereo.ogg", &error);
		CHECK(sound != nullptr);
		if (sound) {
			CHECK(Near(static_cast<int>(sound->Frames()), 12000, 48));
			CHECK(Near(peak(*sound), 11700, 1500));
		}
	}
	// the extension check is case-insensitive: .OGG must not fall into the WAV reader
	{
		const std::string upper = (g_dir / "TONE.OGG").string();
		std::filesystem::copy_file(std::string(SDLGF_AUDIO_FIXTURE_DIR) + "/tone_48k_stereo.ogg", upper);
		CHECK(LoadSoundFile(upper, &error) != nullptr);
	}
	// bad input is reported, not thrown
	{
		error.clear();
		CHECK(LoadSoundFile((g_dir / "does-not-exist.ogg").string(), &error) == nullptr);
		CHECK(!error.empty());
		const std::string notOgg = (g_dir / "text.ogg").string();
		std::ofstream(notOgg) << "this is not an ogg file";
		error.clear();
		CHECK(LoadSoundFile(notOgg, &error) == nullptr);
		CHECK(!error.empty());
		// a rejected file must not stay open (SDL_mixer leaks the handle when it is given a path);
		// Windows refuses to delete a file that is still open, other systems cannot tell
		std::error_code removeError;
		CHECK(std::filesystem::remove(notOgg, removeError) && !removeError);
	}
	// a decoded OGG plays through the mixer like any other sound
	{
		auto sound = LoadSoundFile(std::string(SDLGF_AUDIO_FIXTURE_DIR) + "/tone_48k_stereo.ogg", &error);
		CHECK(sound != nullptr);
		if (sound) {
			Mixer mixer;
			mixer.AddVoice(sound, nullptr, false, 1.0f, true);
			std::vector<float> out(static_cast<size_t>(sound->Frames()) * 2 + 2000);
			mixer.Render(out.data(), static_cast<int>(out.size() / 2));
			float maxSample = 0;
			for (float v : out) maxSample = (std::max)(maxSample, std::fabs(v));
			CHECK(maxSample > 0.3f && maxSample < 0.7f);
		}
	}
}

static void TestMixer()
{
	// one-shot: plays once, then reports finished (and is silent afterwards)
	{
		Mixer mixer;
		Voice* v = mixer.AddVoice(Ramp(1, 10), nullptr, false, 1.0f, true);
		auto first = RenderLeft(mixer, 6);
		CHECK((first == std::vector<int>{ 1, 2, 3, 4, 5, 6 }));
		CHECK(!mixer.IsFinished(v));
		auto second = RenderLeft(mixer, 6);
		CHECK((second == std::vector<int>{ 7, 8, 9, 10, 0, 0 }));
		CHECK(mixer.IsFinished(v));
		CHECK((RenderLeft(mixer, 3) == std::vector<int>{ 0, 0, 0 }));
		mixer.RemoveVoice(v);
		CHECK(mixer.VoiceCount() == 0);
	}
	// a voice that ends exactly at the end of a Render call is finished right away
	{
		Mixer mixer;
		Voice* v = mixer.AddVoice(Ramp(1, 4), nullptr, false, 1.0f, true);
		RenderLeft(mixer, 4);
		CHECK(mixer.IsFinished(v));
	}
	// looping wraps seamlessly across Render calls of any size
	{
		Mixer mixer;
		Voice* v = mixer.AddVoice(Ramp(1, 4), nullptr, true, 1.0f, true);
		std::vector<int> all;
		for (int n : { 3, 7, 1, 5 }) { auto part = RenderLeft(mixer, n); all.insert(all.end(), part.begin(), part.end()); }
		const std::vector<int> expected = { 1,2,3,4, 1,2,3,4, 1,2,3,4, 1,2,3,4 };
		CHECK(all == expected);
		CHECK(!mixer.IsFinished(v));
	}
	// intro plays once and runs straight into the loop
	{
		Mixer mixer;
		mixer.AddVoice(Ramp(11, 3), Ramp(1, 2), true, 1.0f, true);
		std::vector<int> all;
		for (int n : { 1, 4, 6 }) { auto part = RenderLeft(mixer, n); all.insert(all.end(), part.begin(), part.end()); }
		const std::vector<int> expected = { 1,2, 11,12,13, 11,12,13, 11,12,13 };
		CHECK(all == expected);
	}
	// gain, summing and the right channel
	{
		Mixer mixer;
		Voice* a = mixer.AddVoice(Ramp(1000, 4), nullptr, false, 0.5f, true);
		mixer.AddVoice(Ramp(2000, 4), nullptr, false, 1.0f, true);
		std::vector<float> buffer(8);
		mixer.Render(buffer.data(), 4);
		CHECK(Near(static_cast<int>(std::lround(buffer[0] * 32768.0f)), 500 + 2000, 1));
		CHECK(Near(static_cast<int>(std::lround(buffer[1] * 32768.0f)), -(500 + 2000), 1));
		(void)a;
	}
	// SetGain applies from the next Render; Stop silences at once
	{
		Mixer mixer;
		Voice* v = mixer.AddVoice(Ramp(1000, 100), nullptr, false, 1.0f, true);
		CHECK(RenderLeft(mixer, 1)[0] == 1000);
		mixer.SetGain(v, 0.0f);
		CHECK(RenderLeft(mixer, 1)[0] == 0);
		mixer.SetGain(v, 1.0f);
		mixer.Stop(v);
		CHECK(mixer.IsFinished(v));
		CHECK(RenderLeft(mixer, 1)[0] == 0);
	}
	// a voice that was not started is silent; StartTogether starts several in the same pass
	{
		Mixer mixer;
		Voice* a = mixer.AddVoice(Ramp(1, 8), nullptr, true, 1.0f, false);
		Voice* b = mixer.AddVoice(Ramp(100, 8), nullptr, true, 1.0f, false);
		CHECK((RenderLeft(mixer, 5) == std::vector<int>(5, 0)));
		mixer.StartTogether({ a, b });
		auto out = RenderLeft(mixer, 10);
		const std::vector<int> expected = { 101,103,105,107,109,111,113,115, 101,103 };
		CHECK(out == expected);
	}
	// an empty looping voice does not spin forever
	{
		Mixer mixer;
		Voice* v = mixer.AddVoice(std::make_shared<Sound>(), nullptr, true, 1.0f, true);
		RenderLeft(mixer, 16);
		CHECK(mixer.IsFinished(v));
	}
}

static void WaitFor(const std::function<bool()>& done, int timeoutMs, const std::function<void()>& tick)
{
	for (int waited = 0; waited < timeoutMs && !done(); waited += 10) {
		tick();
		SDL_Delay(10);
	}
}

static void TestManager()
{
	const std::string click = WriteWav("click.wav", 44100, 1, 16, false, 2205, [](int i, int) { return 0.3 * std::sin(i * 0.2); }); // 50 ms
	const std::string loopA = WriteWav("loopA.wav", 48000, 2, 16, false, 48000, [](int i, int) { return 0.2 * std::sin(i * 0.05); });
	const std::string loopB = WriteWav("loopB.wav", 48000, 2, 16, false, 48000, [](int i, int) { return 0.2 * std::sin(i * 0.07); });
	const std::string intro = WriteWav("intro.wav", 48000, 2, 16, false, 4800, [](int i, int) { return 0.2 * std::sin(i * 0.09); });

	std::fputs("audio_test:   manager: init\n", stderr);
	SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
	AudioManager* audio = AudioManager::GetInstance();
	CHECK(audio->Init());
	CHECK(audio->Init()); // second call is harmless
	std::fputs("audio_test:   manager: load\n", stderr);

	audio->Load("click", click);
	audio->Load("loopA", loopA);
	audio->Load("loopB", loopB);
	audio->Load("intro", intro);
	audio->Load("missing", (g_dir / "nope.wav").string());
	CHECK(audio->HasSound("click") && audio->HasSound("loopA"));
	CHECK(!audio->HasSound("missing"));

	// unknown sounds are ignored
	audio->Play("missing");
	audio->Play("never-loaded");
	CHECK(audio->GetActiveVoiceCount() == 0);

	std::fputs("audio_test:   manager: one-shot\n", stderr);
	// a one-shot plays through the real device and is cleaned up by Update
	audio->Play("click");
	CHECK(audio->GetActiveVoiceCount() == 1);
	WaitFor([&] { return audio->GetActiveVoiceCount() == 0; }, 3000, [&] { audio->Update(10); });
	CHECK(audio->GetActiveVoiceCount() == 0);

	// banks
	audio->RegisterBank("clicks", { "click", "click" });
	audio->PlayRandom("clicks");
	CHECK(audio->GetActiveVoiceCount() == 1);
	audio->StopAll();
	audio->Update(10);
	CHECK(audio->GetActiveVoiceCount() == 0);

	std::fputs("audio_test:   manager: bgm\n", stderr);
	// background music loops until it is stopped
	audio->PlayBGM_Fade("loopA", 0.5f, 0.1f);
	CHECK(audio->GetActiveVoiceCount() == 1);
	for (int i = 0; i < 40; ++i) { audio->Update(10); SDL_Delay(10); }
	CHECK(audio->GetActiveVoiceCount() == 1); // still playing after 0.4 s (and the 1 s loop has not wrapped yet)
	// crossfading to another track removes the first one once the fade is over
	audio->PlayBGM_Fade("loopB", 0.5f, 0.1f);
	CHECK(audio->GetActiveVoiceCount() == 2);
	for (int i = 0; i < 40; ++i) { audio->Update(10); SDL_Delay(10); }
	CHECK(audio->GetActiveVoiceCount() == 1);
	audio->Stop("loopB");
	audio->Update(10);
	CHECK(audio->GetActiveVoiceCount() == 0);

	// stem sets start all their stems; intro loops start one voice
	audio->RegisterStemSet("stems", { "loopA", "loopB" });
	audio->SetActiveStems("stems", { 0 }, 0.1f);
	audio->PlayBGM_Fade("stems", 0.5f, 0.1f);
	CHECK(audio->GetActiveVoiceCount() == 2);
	audio->Stop("stems");
	audio->Update(10);
	CHECK(audio->GetActiveVoiceCount() == 0);

	audio->RegisterIntroLoop("song", "intro", "loopA");
	audio->PlayBGM_Fade("song", 0.5f, 0.1f);
	CHECK(audio->GetActiveVoiceCount() == 1);
	audio->StopAll();
	audio->Update(10);
	CHECK(audio->GetActiveVoiceCount() == 0);

	std::fputs("audio_test:   manager: volumes\n", stderr);
	// volumes can be changed while sounds play
	audio->Play("loopA", true, 1.0f, AudioType::MUSIC);
	audio->SetMasterVolume(0.5f);
	audio->SetMusicVolume(0.25f);
	audio->SetSfxVolume(0.75f);
	audio->Update(10);

	std::fputs("audio_test:   manager: shutdown\n", stderr);
	audio->Shutdown();
	std::fputs("audio_test:   manager: shut down\n", stderr);
	CHECK(audio->GetActiveVoiceCount() == 0);
	CHECK(!audio->HasSound("click"));
	// after Shutdown there is no device: playing does nothing instead of crashing
	audio->Play("click");
	CHECK(audio->GetActiveVoiceCount() == 0);
	std::fputs("audio_test:   manager: destroy\n", stderr);
	AudioManager::DestroyInstance();
}

int main()
{
	// Report what killed the test: on Windows an uncaught exception otherwise just shows up as
	// exit code 0xc0000409 in the CI log, with no message.
	std::set_terminate([] {
		std::fputs("audio_test: std::terminate called\n", stderr);
		try {
			if (std::current_exception()) std::rethrow_exception(std::current_exception());
			std::fputs("audio_test: ...no active exception (noexcept violation or a joinable thread)\n", stderr);
		}
		catch (const std::exception& e) { std::fprintf(stderr, "audio_test: active exception: %s\n", e.what()); }
		catch (...) { std::fputs("audio_test: active exception of unknown type\n", stderr); }
		std::abort();
	});

	g_dir = std::filesystem::temp_directory_path() / "sdlgf_audio_test";
	std::filesystem::remove_all(g_dir);
	std::filesystem::create_directories(g_dir);

	try {
		std::fputs("audio_test: decoding\n", stderr);
		TestDecoding();
		std::fputs("audio_test: ogg\n", stderr);
		TestOgg();
		std::fputs("audio_test: mixer\n", stderr);
		TestMixer();
		std::fputs("audio_test: manager\n", stderr);
		TestManager();
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "audio_test: uncaught exception: %s\n", e.what());
		return 1;
	}

	std::error_code cleanupError; // a leftover temp folder must not fail the run
	std::filesystem::remove_all(g_dir, cleanupError);
	if (g_failures == 0) std::puts("audio_test: all tests passed");
	else std::fprintf(stderr, "audio_test: %d check(s) failed\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
