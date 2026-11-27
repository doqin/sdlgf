// AssetManager (shared texture/font pool) and AssetLoader (worker-thread decode, main-thread
// upload), headless on SDL's dummy video driver + software renderer.
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include "sdlgf.h"

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

static void TestPool(sdlgf::GraphicsDevice& gd)
{
	auto& assets = gd.GetAssets();
	auto a = assets.GetTexture(L"assets/ui.png");
	auto b = assets.GetTexture(L"assets/ui.png");
	CHECK(a && a->IsLoaded());
	CHECK(a == b); // one copy in memory, however many users
	CHECK(assets.FindTexture(L"assets/ui.png") == a);

	// The pool does not keep assets alive: once every user is gone the texture is freed
	std::weak_ptr<sdlgf::Texture> weak = a;
	a.reset();
	CHECK(!weak.expired());
	b.reset();
	CHECK(weak.expired());
	CHECK(assets.FindTexture(L"assets/ui.png") == nullptr);

	// ...and asking again just loads it again
	CHECK(assets.GetTexture(L"assets/ui.png")->IsLoaded());

	bool threw = false;
	try { assets.GetTexture(L"assets/does-not-exist.png"); }
	catch (const std::runtime_error& e) { threw = std::string(e.what()).find("does-not-exist") != std::string::npos; }
	CHECK(threw); // the error names the file

	// Sprites share ownership of their texture
	std::shared_ptr<sdlgf::StaticSprite> sprite;
	{
		auto tex = assets.GetTexture(L"assets/ui.png");
		sprite = std::make_shared<sdlgf::StaticSprite>(tex);
	}
	CHECK(assets.FindTexture(L"assets/ui.png") != nullptr); // the sprite still holds it
	sprite.reset();
	CHECK(assets.FindTexture(L"assets/ui.png") == nullptr);

	// Recording lists each requested path once, in first-use order, including pool hits
	assets.StartRecording();
	auto t1 = assets.GetTexture(L"assets/ui.png");
	auto t2 = assets.GetTexture(L"assets/ui.png");
	auto t3 = assets.GetTexture(L"assets/popup-borders.png");
	const auto recorded = assets.StopRecording();
	CHECK(recorded.size() == 2 && recorded[0] == L"assets/ui.png" && recorded[1] == L"assets/popup-borders.png");
}

static void TestLoader(sdlgf::GraphicsDevice& gd)
{
	auto& assets = gd.GetAssets();
	std::weak_ptr<sdlgf::Texture> weakBorders;
	{
		auto alreadyLoaded = assets.GetTexture(L"assets/ui.png");
		sdlgf::AssetLoader loader(assets);
		loader.Request({ L"assets/ui.png", L"assets/popup-borders.png", L"assets/does-not-exist.png", L"assets/ui.png" });
		CHECK(loader.GetTotal() == 3); // duplicate request ignored

		const auto deadline = SDL_GetTicks() + 10000;
		while (!loader.Pump(4) && SDL_GetTicks() < deadline) SDL_Delay(1);
		CHECK(loader.IsDone());
		CHECK(loader.GetProgress() == 1.0f);
		CHECK(loader.GetFailures().size() == 1);
		CHECK(!loader.GetFailures().empty() && loader.GetFailures()[0].find("does-not-exist") != std::string::npos);

		auto borders = assets.FindTexture(L"assets/popup-borders.png");
		CHECK(borders && borders->IsLoaded() && borders->GetWidth() > 0);
		CHECK(assets.GetTexture(L"assets/ui.png") == alreadyLoaded); // pooled copy reused, not reloaded
		weakBorders = borders;
	}
	// The loader held the preloaded textures; with it gone and no other user they are freed
	CHECK(weakBorders.expired());

	// Nothing to load is immediately done
	sdlgf::AssetLoader empty(assets);
	CHECK(empty.IsDone() && empty.GetProgress() == 1.0f);
}

static void TestFontCache(sdlgf::GraphicsDevice& gd)
{
	sdlgf::Font::AddFont(L"assets/135openpixel-v2-3.ttf");
	auto font = gd.GetAssets().GetFont(L"135OpenPixel v2.3", 13);
	CHECK(font == gd.GetAssets().GetFont(L"135OpenPixel v2.3", 13)); // fonts are pooled too
	const auto hot = font->Render(L"always drawn").texture;
	CHECK(hot != nullptr);
	// A stream of one-off strings (scores, timers) must not grow the cache without bound, nor push out
	// the string that is drawn all the time
	for (int i = 0; i < 5000; ++i) {
		font->Render(std::to_wstring(i));
		if (i % 100 == 0) font->Render(L"always drawn");
	}
	CHECK(font->GetCachedStringCount() <= 1024);
	CHECK(font->Render(L"always drawn").texture == hot);
	sdlgf::Font::RemoveAllFonts();
}

int main()
{
	try {
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
		if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
		SDL_Window* window = SDL_CreateWindow("assets_test", 64, 64, SDL_WINDOW_HIDDEN);
		if (window == nullptr) throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
		std::filesystem::current_path(SDLGF_ASSET_DIR);
		{
			sdlgf::GraphicsDevice gd(window, "software");
			TestPool(gd);
			TestLoader(gd);
			TestFontCache(gd);
		}
		SDL_DestroyWindow(window);
		SDL_Quit();
		if (g_failures == 0) std::puts("assets_test: all checks passed");
		return g_failures == 0 ? 0 : 1;
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "assets_test failed: %s\n", e.what());
		return 1;
	}
}
