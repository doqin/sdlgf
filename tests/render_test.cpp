// Renders the golden scenes (text in every bundled font, sprites, primitives, tile maps) with the
// SDL renderer and compares the result with the images captured from the old Direct3D 9 renderer
// (tests/golden/*.png, see the README there for what each one contains).
//
// Runs on a hidden window with the Direct3D 11 driver (WARP when there is no GPU). Scenes whose
// geometry is not aligned to pixels may differ from D3D9 in a few edge pixels; those differences
// are listed in g_allowed with their exact size so that a regression (more or larger differences)
// and an improvement (fewer) both show up. Everything else must be bit-identical. Other render
// drivers (SDLGF_TEST_RENDERER=software) only report.
//
// Usage: sdlgf_render_test [output directory for the rendered images]
#include <SDL3/SDL.h>

#include "sdlgf.h"
#include "Extras.h"
#include "text_cases.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifndef SDLGF_ASSET_DIR
#	error "SDLGF_ASSET_DIR must point at demo/"
#endif
#ifndef SDLGF_GOLDEN_DIR
#	error "SDLGF_GOLDEN_DIR must point at tests/golden"
#endif

namespace {
	int g_failures = 0;
	std::filesystem::path g_outDir;

	struct Image {
		int width = 0, height = 0;
		std::vector<std::uint32_t> px; // 0xAARRGGBB
	};

	Image FromSurface(SDL_Surface* surface)
	{
		Image image;
		SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);
		if (converted == nullptr) return image;
		image.width = converted->w;
		image.height = converted->h;
		image.px.resize(static_cast<size_t>(image.width) * image.height);
		for (int y = 0; y < image.height; ++y) {
			std::memcpy(&image.px[static_cast<size_t>(y) * image.width],
				static_cast<const unsigned char*>(converted->pixels) + static_cast<size_t>(y) * converted->pitch,
				static_cast<size_t>(image.width) * 4);
		}
		SDL_DestroySurface(converted);
		return image;
	}

	Image LoadGolden(const std::string& name)
	{
		const std::string path = std::string(SDLGF_GOLDEN_DIR) + "/" + name + ".png";
		SDL_Surface* surface = SDL_LoadSurface(path.c_str());
		if (surface == nullptr) {
			std::fprintf(stderr, "FAIL cannot load %s: %s\n", path.c_str(), SDL_GetError());
			++g_failures;
			return {};
		}
		Image image = FromSurface(surface);
		SDL_DestroySurface(surface);
		return image;
	}

	void SavePng(const Image& image, const std::string& name)
	{
		if (g_outDir.empty()) return;
		std::vector<std::uint32_t> copy = image.px;
		SDL_Surface* surface = SDL_CreateSurfaceFrom(image.width, image.height, SDL_PIXELFORMAT_ARGB8888, copy.data(), image.width * 4);
		if (surface == nullptr) return;
		SDL_SavePNG(surface, (g_outDir / (name + ".png")).string().c_str());
		SDL_DestroySurface(surface);
	}

	int MaxChannelDiff(std::uint32_t a, std::uint32_t b)
	{
		int diff = 0;
		for (int shift = 0; shift <= 16; shift += 8) {
			diff = std::max(diff, std::abs(static_cast<int>((a >> shift) & 0xFF) - static_cast<int>((b >> shift) & 0xFF)));
		}
		return diff;
	}

	/// How much of a scene may differ from the D3D9 image: at most `pixels` differing pixels, none by
	/// more than `maxChannel` in any colour channel. Scenes not listed must be identical.
	struct Allowed {
		int pixels;
		int maxChannel;
		const char* why;
	};
	// Direct3D 9 and Direct3D 11 pick the texel for a pixel centre that falls exactly on a texel
	// boundary differently, and round sub-pixel positions differently, so scenes with fractional
	// positions/scales differ in single texel picks and edge pixels. The limits are the largest
	// differences seen on the direct3d11 driver, on a GPU and on WARP (the GitHub runners, whose
	// rasteriser differs from the GPU's in a few edge pixels: primitives 24 vs 71, map_labinside_b
	// 364 vs 362), so a regression beyond them fails.
	std::map<std::string, Allowed> g_allowed = {
		{ "text_openpixel39", { 1, 207, "one lit pixel under a circumflex: FreeType vs GDI rasteriser" } },
		{ "sprites", { 343, 234, "fractional positions/scales, rotation, 1.5x nine-slice: texel picks at boundaries" } },
		{ "primitives", { 71, 207, "diagonal 1 px lines and rotated outlines: line rasterisation rule" } },
		{ "map_labinside_b", { 364, 207, "3x zoom at a fractional camera position: tile seams" } },
	};
	// Only the direct3d11 driver is compared strictly; other drivers rasterise differently and just report.
	bool g_strict = true;

	/// Rows [first, second) of an image that are not compared (text the font cannot draw).
	using Bands = std::vector<std::pair<int, int>>;

	void Compare(const std::string& name, const Image& actual, const Bands& ignore = {})
	{
		SavePng(actual, name);
		const Image golden = LoadGolden(name);
		if (golden.width == 0) return;
		if (golden.width != actual.width || golden.height != actual.height) {
			std::fprintf(stderr, "FAIL %s: size %dx%d, golden %dx%d\n", name.c_str(), actual.width, actual.height, golden.width, golden.height);
			++g_failures;
			return;
		}
		int differing = 0, maxDiff = 0, firstX = -1, firstY = -1;
		Image diffImage;
		diffImage.width = actual.width;
		diffImage.height = actual.height;
		diffImage.px.resize(actual.px.size());
		for (int y = 0; y < actual.height; ++y) {
			const bool ignored = std::any_of(ignore.begin(), ignore.end(), [y](const auto& band) { return y >= band.first && y < band.second; });
			for (int x = 0; x < actual.width; ++x) {
				const size_t index = static_cast<size_t>(y) * actual.width + x;
				const std::uint32_t a = actual.px[index] & 0xFFFFFF;
				const std::uint32_t g = golden.px[index] & 0xFFFFFF;
				// diff image: the golden image dimmed, differing pixels red (ours brighter) or blue (golden brighter)
				const unsigned dim = ((g >> 16 & 0xFF) + (g >> 8 & 0xFF) + (g & 0xFF)) / 3 / 3;
				diffImage.px[index] = 0xFF000000u | (dim << 16) | (dim << 8) | dim;
				if (a == g || ignored) continue;
				diffImage.px[index] = (a & 0xFF) + (a >> 8 & 0xFF) + (a >> 16 & 0xFF) > (g & 0xFF) + (g >> 8 & 0xFF) + (g >> 16 & 0xFF) ? 0xFFFF2020u : 0xFF3030FFu;
				if (differing++ == 0) { firstX = x; firstY = y; }
				maxDiff = std::max(maxDiff, MaxChannelDiff(a, g));
			}
		}
		if (differing > 0) SavePng(diffImage, name + ".diff");
		const auto allowed = g_allowed.find(name);
		const int allowedPixels = allowed == g_allowed.end() ? 0 : allowed->second.pixels;
		const int allowedChannel = allowed == g_allowed.end() ? 0 : allowed->second.maxChannel;
		const bool ok = !g_strict || (differing <= allowedPixels && maxDiff <= allowedChannel);
		std::printf("  %-22s %s  %d differing pixels (max channel diff %d)%s\n", name.c_str(), ok ? "ok  " : "FAIL", differing, maxDiff,
			allowed == g_allowed.end() ? "" : "  [known]");
		if (!ok) {
			std::fprintf(stderr, "FAIL %s: %d differing pixels (allowed %d), max channel diff %d (allowed %d); first at (%d,%d): golden %08X, ours %08X\n",
				name.c_str(), differing, allowedPixels, maxDiff, allowedChannel, firstX, firstY,
				(unsigned)golden.px[static_cast<size_t>(firstY) * actual.width + firstX],
				(unsigned)actual.px[static_cast<size_t>(firstY) * actual.width + firstX]);
			++g_failures;
		}
	}

	// ---- canvas ---------------------------------------------------------------------------------

	struct Canvas {
		sdlgf::GraphicsDevice* gd;
		std::shared_ptr<sdlgf::Texture> target;
		UINT width, height;
		std::string name;
	};

	Canvas BeginCanvas(sdlgf::GraphicsDevice* gd, const std::string& name, UINT w, UINT h, sdlgf::Color clear)
	{
		Canvas c{ gd, std::make_shared<sdlgf::Texture>(gd), w, h, name };
		c.target->CreateRenderTarget(w, h);
		gd->SetRenderTarget(c.target.get());
		gd->SetViewport(0, 0, static_cast<int>(w), static_cast<int>(h));
		gd->ResetVirtualTransform();
		gd->SetAlphaBlending(false);
		gd->SetScissorTest(false);
		gd->Clear(clear);
		gd->BeginDraw();
		return c;
	}

	// Ends the scene, reads the render target back and compares it with the golden image.
	void EndCanvas(Canvas& c, const Bands& ignore = {})
	{
		c.gd->EndDraw();
		SDL_Surface* pixels = SDL_RenderReadPixels(c.gd->GetRenderer(), nullptr);
		if (pixels == nullptr) {
			std::fprintf(stderr, "FAIL %s: SDL_RenderReadPixels: %s\n", c.name.c_str(), SDL_GetError());
			++g_failures;
		}
		else {
			Image image = FromSurface(pixels);
			SDL_DestroySurface(pixels);
			Compare(c.name, image, ignore);
		}
		c.gd->RestoreRenderTarget();
	}

	std::shared_ptr<sdlgf::Texture> Load(sdlgf::GraphicsDevice* gd, const wchar_t* path)
	{
		auto t = std::make_shared<sdlgf::Texture>(gd);
		t->LoadTexture(path);
		return t;
	}

	// Camera that maps world coordinates 1:1 to the canvas (what the engine's defaultCamera does).
	sdlgf::Camera IdentityCamera(UINT w, UINT h)
	{
		sdlgf::Camera cam(static_cast<int>(w), static_cast<int>(h));
		cam.SetPosition(w / 2.0f, h / 2.0f);
		return cam;
	}

	// ---- text ---------------------------------------------------------------------------------

	void RenderTextSheet(sdlgf::GraphicsDevice* gd, const golden::FontCase& fc)
	{
		const auto& rows = golden::TextRows();

		const UINT width = 960;
		const float step = golden::RowStep(fc.size);
		const UINT height = static_cast<UINT>(step * rows.size()) + 16;
		auto canvas = BeginCanvas(gd, std::string("text_") + fc.tag, width, height, 0xFF303030);

		sdlgf::Font font(gd, fc.family, fc.size);
		// GDI drew characters the font lacks with a substitute system font; there is nothing to
		// compare those rows against (they would need that font), so they are skipped.
		Bands ignore;
		float y = 8.0f;
		for (const auto& row : rows) {
			const std::u32string codePoints = sdlgf::Platform::WideToUtf32(row.text);
			if (!std::all_of(codePoints.begin(), codePoints.end(), [&](char32_t c) { return c < 0x20 || font.GetFace().HasGlyph(c); })) {
				const int top = std::max(0, static_cast<int>(y) - fc.size / 2);
				ignore.emplace_back(top, top + static_cast<int>(step));
			}
			sdlgf::FontSprite sprite(&font);
			sprite.SetText(std::wstring(row.text));
			sprite.SetColor(row.color);
			sprite.SetBold(row.bold);
			if (row.outline) sprite.SetOutline(true, 0xFF000000, row.outlineThickness);
			if (row.maxWidth > 0.0f) sprite.SetMaxWidth(row.maxWidth);

			sdlgf::Camera cam = IdentityCamera(width, height);
			cam.SetZoom(row.zoom);
			// With zoom the camera pivots on its position; keep the sprite's top-left where it would be at 1x.
			const float cx = width / 2.0f, cy = height / 2.0f;
			cam.SetPosition(cx - (row.x * (row.zoom - 1.0f)) / row.zoom, cy - ((y + row.yFraction) * (row.zoom - 1.0f)) / row.zoom);
			sprite.SetPosition(row.x, y + row.yFraction);
			sprite.Begin();
			sprite.Draw(cam, 0);
			sprite.End();
			y += step;
		}
		EndCanvas(canvas, ignore);
	}

	void RenderTextProbes(sdlgf::GraphicsDevice* gd)
	{
		const auto& probes = golden::Probes();

		sdlgf::Font font(gd, L"135OpenPixel v2.3", 13);
		const float step = 13.0f * 6.0f;
		const UINT width = 400;
		const UINT height = static_cast<UINT>(step * probes.size()) + 16;
		auto canvas = BeginCanvas(gd, "text_probe", width, height, 0xFF303030);
		sdlgf::Camera cam = IdentityCamera(width, height);
		float y = 8.0f;
		for (const auto& p : probes) {
			sdlgf::FontSprite sprite(&font);
			sprite.SetText(std::wstring(p.text));
			if (p.maxWidth > 0.0f) sprite.SetMaxWidth(p.maxWidth);
			// guide rectangle showing the wrap width
			if (p.maxWidth > 0.0f) gd->DrawRectangle(8.0f, y, p.maxWidth, 1.0f, 0xFF804040, true);
			sprite.SetPosition(8.0f, y + 2.0f);
			sprite.Begin();
			sprite.Draw(cam, 0);
			sprite.End();
			y += step;
		}
		EndCanvas(canvas);
	}

	// ---- sprites ------------------------------------------------------------------------------

	void RenderSprites(sdlgf::GraphicsDevice* gd)
	{
		const UINT w = 480, h = 320;
		auto canvas = BeginCanvas(gd, "sprites", w, h, 0xFF303030);
		auto token = Load(gd, L"assets/12x12-gold-token.png"); // 264x110 sheet
		auto sheet = Load(gd, L"assets/hkhang-Sheet.png");     // 64x32
		auto borders = Load(gd, L"assets/popup-borders.png");  // 320x160
		sdlgf::Camera cam = IdentityCamera(w, h);
		const RECT tokenRect = { 0, 0, 12, 12 };

		struct Item { float x, y, sx, sy, rot; sdlgf::Color color; bool flipH; };
		const std::vector<Item> items = {
			{ 8,    8,    1,    1,   0.0f,  0xFFFFFFFF, false },  // integer position
			{ 28.5f, 8.25f, 1,  1,   0.0f,  0xFFFFFFFF, false },  // fractional position
			{ 56,   8,    2,    2,   0.0f,  0xFFFFFFFF, false },  // 2x
			{ 96,   8,    2.5f, 2.5f, 0.0f, 0xFFFFFFFF, false },  // fractional scale
			{ 144,  8,    3,    3,   0.0f,  0xFFFFFFFF, true },   // flipped
			{ 200,  8,    2,    2,   0.5f,  0xFFFFFFFF, false },  // rotated
			{ 248,  8,    2,    2,   0.0f,  0x80FFFFFF, false },  // half alpha
			{ 288,  8,    2,    2,   0.0f,  0xFFFF4040, false },  // tinted
			{ 328,  8,    3,    1,   0.0f,  0xFFFFFFFF, false },  // stretched
		};
		for (const auto& it : items) {
			sdlgf::StaticSprite s(token.get());
			s.SetSrcRect(tokenRect);
			s.SetPosition(it.x, it.y);
			s.SetScale(it.sx, it.sy);
			s.SetRotation(it.rot);
			s.SetColor(it.color);
			if (it.flipH) s.FlipHorizontal();
			s.Begin(); s.Draw(cam, 0); s.End();
		}

		// Whole texture, no source rect
		{
			sdlgf::StaticSprite s(sheet.get());
			s.SetPosition(8, 70);
			s.Begin(); s.Draw(cam, 0); s.End();
		}
		// Rotation about a centred origin
		{
			sdlgf::StaticSprite s(sheet.get());
			s.SetPosition(120, 90);
			s.SetOrigin(32, 16);
			s.SetRotation(0.7f);
			s.Begin(); s.Draw(cam, 0); s.End();
		}
		// Animated sprite: frames of 16x16 from the sheet
		{
			auto frames = sdlgf::Utils::CreateFrames(64, 32, 16, 16, 8);
			for (unsigned i : { 0u, 3u, 5u }) {
				sdlgf::AnimatedSprite s(sheet.get(), frames, 12, true);
				s.SetFrame(i);
				s.SetPosition(180.0f + i * 40.0f, 70);
				s.SetScale(2.0f, 2.0f);
				s.Begin(); s.Draw(cam, 0); s.End();
			}
		}
		// Nine-slice at several target sizes and scales
		struct Nine { float x, y, tw, th, scale; };
		const std::vector<Nine> nines = {
			{ 8, 140, 48, 48, 1 }, { 70, 140, 100, 40, 1 }, { 190, 140, 120, 80, 2 }, { 340, 140, 90, 60, 1.5f },
		};
		for (const auto& n : nines) {
			sdlgf::NineSliceSprite s(borders.get(), RECT{ 0, 0, 48, 48 }, 16, 16, 16, 16);
			s.SetTargetSize(n.tw, n.th);
			s.SetScale(n.scale);
			s.SetPosition(n.x, n.y);
			s.Begin(); s.Draw(cam, 0); s.End();
		}
		// Zoomed camera with an offset position
		{
			sdlgf::Camera zoomed = IdentityCamera(w, h);
			zoomed.SetZoom(2.0f);
			zoomed.SetPosition(w / 2.0f + 30.5f, h / 2.0f + 70.25f);
			sdlgf::StaticSprite s(token.get());
			s.SetSrcRect(tokenRect);
			s.SetPosition(w / 2.0f + 10.0f, h / 2.0f + 90.0f);
			s.Begin(); s.Draw(zoomed, 0); s.End();
		}
		EndCanvas(canvas);
	}

	// ---- primitives -----------------------------------------------------------------------------

	void RenderPrimitives(sdlgf::GraphicsDevice* gd)
	{
		const UINT w = 480, h = 320;
		auto canvas = BeginCanvas(gd, "primitives", w, h, 0xFF303030);
		auto token = Load(gd, L"assets/12x12-gold-token.png");
		sdlgf::Camera cam = IdentityCamera(w, h);

		for (int pass = 0; pass < 2; ++pass) {
			const bool blend = pass == 1;
			const float ox = blend ? 240.0f : 0.0f;
			gd->SetAlphaBlending(blend);
			const sdlgf::Color a = blend ? 0x80FF0000 : 0xFFFF0000;
			const sdlgf::Color b = blend ? 0x8000FF00 : 0xFF00FF00;
			const sdlgf::Color c = blend ? 0x800080FF : 0xFF0080FF;

			// instant (screen-space) draws
			gd->DrawLine(ox + 8, 8, ox + 100, 8, a);                    // horizontal 1px
			gd->DrawLine(ox + 8, 12, ox + 8, 60, a);                    // vertical 1px
			gd->DrawLine(ox + 14, 12, ox + 100, 60, b);                 // diagonal 1px
			gd->DrawLine(ox + 14, 70, ox + 100, 100, c, 3.0f);          // thick diagonal
			gd->DrawLine(ox + 14, 110, ox + 100, 110, a, 5.0f);         // thick horizontal
			gd->DrawRectangle(ox + 110, 12, 40, 30, a, true);           // filled
			gd->DrawRectangle(ox + 160, 12, 40, 30, b, false);          // outline
			gd->DrawRectangle(ox + 110, 56, 40, 30, 0.5f, 1.0f, 1.0f, 20.0f, 15.0f, c, true);  // rotated filled
			gd->DrawRectangle(ox + 170, 56, 40, 30, 0.5f, 1.0f, 1.0f, 20.0f, 15.0f, a, false); // rotated outline
			gd->DrawEllipse(ox + 130, 120, 40, 24, b, true);
			gd->DrawEllipse(ox + 180, 120, 40, 24, c, false);
			gd->DrawTriangle(ox + 40, 130, 40, 24, a, true);
			gd->DrawTriangle(ox + 90, 130, 40, 24, b, false);

			// camera-space draws at zoom 2 with a fractional camera position
			sdlgf::Camera zoomed = IdentityCamera(w, h);
			zoomed.SetZoom(2.0f);
			zoomed.SetPosition(w / 2.0f + 0.5f, h / 2.0f + 0.25f);
			gd->DrawLine(zoomed, ox + 10, 90, ox + 60, 110, c);
			gd->DrawRectangle(zoomed, ox + 70, 90, 20, 14, a, true);
			gd->DrawRectangle(zoomed, ox + 95, 90, 20, 14, b, false);
			gd->DrawEllipse(zoomed, ox + 10, 118, 30, 20, a, true);
			gd->DrawTriangle(zoomed, ox + 70, 118, 24, 18, c, true);
		}

		// textured polygon (as used for the map previews), alpha blending left in the last state
		gd->SetAlphaBlending(true);
		std::vector<sdlgf::Vec2> pts = { {20, 220}, {120, 210}, {150, 290}, {40, 300} };
		std::vector<sdlgf::Vec2> uvs = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
		gd->DrawTexturedPolygon(pts, uvs, token.get());
		std::vector<sdlgf::Vec2> pts2 = { {200, 220}, {300, 220}, {350, 270}, {250, 300}, {190, 270} };
		std::vector<sdlgf::Vec2> uvs2 = { {0.1f, 0}, {0.9f, 0}, {1.4f, 0.5f}, {0.5f, 1.2f}, {-0.2f, 0.6f} };  // outside [0,1]: clamp
		gd->DrawTexturedPolygon(pts2, uvs2, token.get(), 0xFFFF8080);

		// scissor test
		gd->SetAlphaBlending(false);
		RECT scissor = { 360, 220, 440, 280 };
		gd->SetScissorRect(scissor);
		gd->SetScissorTest(true);
		gd->DrawRectangle(350, 210, 120, 100, 0xFFFFFF00, true);
		gd->SetScissorTest(false);
		gd->DrawRectangle(350, 300, 30, 10, 0xFFFFFFFF, false);
		EndCanvas(canvas);
	}

	// ---- tile map -------------------------------------------------------------------------------

	void RenderMap(sdlgf::GraphicsDevice* gd, const char* tmx, const char* name, float camX, float camY, float zoom)
	{
		const UINT w = 480, h = 360;
		auto transformManager = std::make_shared<sdlgf::TransformManager>();
		auto colliderManager = std::make_shared<sdlgf::ColliderManager>();
		sdlgf::Map map(gd);
		map.Create(transformManager, colliderManager, tmx);

		auto canvas = BeginCanvas(gd, name, w, h, 0xFF303030);
		sdlgf::Camera cam(static_cast<int>(w), static_cast<int>(h));
		cam.SetZoom(zoom);
		cam.SetPosition(camX, camY);
		map.Draw(cam);
		EndCanvas(canvas);
	}
}

int main(int argc, char** argv)
{
	try {
		g_outDir = (argc > 1) ? std::filesystem::absolute(argv[1]) : std::filesystem::path();
		if (!g_outDir.empty()) std::filesystem::create_directories(g_outDir);

		// Defaults: headless and deterministic. SDLGF_TEST_VIDEO / SDLGF_TEST_RENDERER choose another
		// video/render driver (e.g. "windows" + "direct3d11" to try the GPU path on a real display).
#ifdef _WIN32
		// Direct3D 11 needs a real (hidden) window; on machines without a GPU it runs on WARP.
		const char* defaultVideo = "windows";
		const char* defaultRenderer = "direct3d11";
#else
		const char* defaultVideo = "dummy";
		const char* defaultRenderer = "software";
#endif
		const char* video = std::getenv("SDLGF_TEST_VIDEO");
		const char* rendererName = std::getenv("SDLGF_TEST_RENDERER");
		if (rendererName == nullptr) rendererName = defaultRenderer;
		g_strict = std::strcmp(rendererName, "direct3d11") == 0;
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, video ? video : defaultVideo);
		if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
		SDL_Window* window = SDL_CreateWindow("render_test", 960, 720, SDL_WINDOW_HIDDEN);
		if (window == nullptr) throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
		// Assets are referenced relative to the demo directory
		std::filesystem::current_path(SDLGF_ASSET_DIR);

		{
			// Everything drawn with the device is released before the window goes away
			sdlgf::GraphicsDevice gd(window, rendererName);

			for (const auto& f : golden::Fonts()) {
				sdlgf::Font::AddFont(std::wstring(L"assets/") + std::wstring(f.file, f.file + std::strlen(f.file)));
			}

			std::printf("render test (%s renderer%s), compared with tests/golden:\n", rendererName, g_strict ? "" : ", report only");
			for (const auto& f : golden::Fonts()) RenderTextSheet(&gd, f);
			RenderTextProbes(&gd);
			RenderSprites(&gd);
			RenderPrimitives(&gd);
			RenderMap(&gd, "assets/labinside.tmx", "map_labinside_a", 64.0f, 64.0f, 2.0f);
			RenderMap(&gd, "assets/labinside.tmx", "map_labinside_b", 50.5f, 70.25f, 3.0f);
			RenderMap(&gd, "assets/tutorial.tmx", "map_tutorial", 100.0f, 100.0f, 2.0f);

			sdlgf::Font::RemoveAllFonts();
		}
		SDL_DestroyWindow(window);
		SDL_Quit();
		if (g_failures == 0) std::puts("render_test: all scenes match");
		else std::fprintf(stderr, "render_test: %d scene(s) failed\n", g_failures);
		return g_failures == 0 ? 0 : 1;
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "render_test failed: %s\n", e.what());
		return 1;
	}
}
