// Golden-image generator for the Direct3D 9 renderer.
//
// Renders a fixed set of scenes (text in every bundled font, sprites, primitives, a tile map)
// with the CURRENT D3D9/D3DX renderer into offscreen render targets, reads the pixels back and
// writes them as PNGs plus a metrics file with the text sizes D3DXFont reports. These files are
// the reference ("golden") output that the SDL renderer is later tested against.
//
// This tool depends on the legacy renderer and is deleted together with it. Rerun it only from
// the commit that tags the last D3D9 build.
//
// Usage: sdlgf_golden_gen <output directory>
#include <SDL3/SDL.h>

#include "DX9GF.h"
#include "DX9GFExtras.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {
	std::filesystem::path g_outDir;
	std::ofstream g_metrics;

	struct Canvas {
		DX9GF::GraphicsDevice* gd;
		std::shared_ptr<DX9GF::Texture> target;
		UINT width, height;
		std::string name;
	};

	Canvas BeginCanvas(DX9GF::GraphicsDevice* gd, const std::string& name, UINT w, UINT h, D3DCOLOR clear)
	{
		Canvas c{ gd, std::make_shared<DX9GF::Texture>(gd), w, h, name };
		c.target->CreateRenderTarget(w, h);
		gd->SetRenderTarget(c.target.get());
		gd->SetViewport(0, 0, w, h, 0.0f, 1.0f);
		gd->ResetVirtualTransform();
		gd->SetAlphaBlending(false);
		gd->Clear(clear);
		gd->BeginDraw();
		return c;
	}

	// Ends the scene, copies the render target to system memory and saves it as an opaque PNG.
	void EndCanvas(Canvas& c)
	{
		c.gd->EndDraw();
		IDirect3DDevice9* dev = c.gd->GetDevice();
		IDirect3DSurface9* rt = c.target->GetSurface();
		IDirect3DSurface9* sys = nullptr;
		if (FAILED(dev->CreateOffscreenPlainSurface(c.width, c.height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, nullptr))
			|| FAILED(dev->GetRenderTargetData(rt, sys))) {
			throw std::runtime_error("readback failed for " + c.name);
		}
		D3DLOCKED_RECT lr{};
		if (FAILED(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))) throw std::runtime_error("lock failed");
		SDL_Surface* surface = SDL_CreateSurfaceFrom(static_cast<int>(c.width), static_cast<int>(c.height),
			SDL_PIXELFORMAT_XRGB8888, lr.pBits, lr.Pitch);
		const std::string path = (g_outDir / (c.name + ".png")).string();
		if (!SDL_SavePNG(surface, path.c_str())) throw std::runtime_error("SDL_SavePNG: " + std::string(SDL_GetError()));
		SDL_DestroySurface(surface);
		sys->UnlockRect();
		sys->Release();
		rt->Release();
		c.gd->RestoreRenderTarget();
		std::printf("wrote %s (%ux%u)\n", path.c_str(), c.width, c.height);
	}

	std::shared_ptr<DX9GF::Texture> Load(DX9GF::GraphicsDevice* gd, const wchar_t* path)
	{
		auto t = std::make_shared<DX9GF::Texture>(gd);
		t->LoadTexture(path);
		return t;
	}

	// Camera that maps world coordinates 1:1 to the canvas (what the engine's defaultCamera does).
	DX9GF::Camera IdentityCamera(UINT w, UINT h)
	{
		DX9GF::Camera cam(static_cast<int>(w), static_cast<int>(h));
		cam.SetPosition(w / 2.0f, h / 2.0f);
		return cam;
	}

	// ---- text -------------------------------------------------------------------------------

	struct TextRow {
		const char* id;
		const wchar_t* text;
		D3DCOLOR color = 0xFFFFFFFF;
		bool bold = false;
		bool outline = false;
		float outlineThickness = 1.0f;
		float maxWidth = 0.0f;
		float zoom = 1.0f;     // camera zoom (the game scales text this way)
		float x = 8.0f;
		float yFraction = 0.0f;
	};

	void RenderTextSheet(DX9GF::GraphicsDevice* gd, const wchar_t* family, int size, const char* fileTag)
	{
		const std::vector<TextRow> rows = {
			{ "latin",       L"The quick brown fox jumps over 0123456789" },
			{ "vietnamese",  L"T\u00F4i, sinh vi\u00EAn UIT n\u0103m 6 b\u1ECB h\u00FAt v\u00E0o cyberspace" },
			{ "diacritics",  L"\u1EAE \u1EB0 \u1EB2 \u1EB4 \u1EB6 \u0110 \u0111 \u01A0 \u01A1 \u01AF \u01B0 \u1EC5 \u1EE3" },
			{ "punctuation", L"!@#$%^*()-_=+[]{};:'\",.<>/?\\|~`" },
			{ "ampersand",   L"UI & Icons: A && B & C" },
			{ "amp_ab",      L"AB" },
			{ "amp_a_b",     L"A&B" },
			{ "amp_a_ampamp_b", L"A&&B" },
			{ "multiline",   L"line one\nline two\r\nline three" },
			{ "wrapped",     L"A long sentence that has to wrap onto several lines because it exceeds the maximum width.", 0xFFFFFFFF, false, false, 1.0f, 300.0f },
			{ "color",       L"Red text", 0xFFFF0000 },
			{ "alpha",       L"Half transparent", 0x80FFFFFF },
			{ "bold",        L"Bold text", 0xFFFFFFFF, true },
			{ "outline1",    L"Outline 1", 0xFFFFFFFF, false, true, 1.0f },
			{ "outline2",    L"Outline 2 bold", 0xFFFFD700, true, true, 2.0f },
			{ "zoom2",       L"Zoom 2x", 0xFFFFFFFF, false, false, 1.0f, 0.0f, 2.0f },
			{ "zoom1_5",     L"Zoom 1.5x", 0xFFFFFFFF, false, false, 1.0f, 0.0f, 1.5f },
			{ "fractional",  L"Fractional position", 0xFFFFFFFF, false, false, 1.0f, 0.0f, 1.0f, 8.5f, 0.5f },
		};

		const UINT width = 960;
		const float step = static_cast<float>(size) * 4.5f;
		const UINT height = static_cast<UINT>(step * rows.size()) + 16;
		auto canvas = BeginCanvas(gd, std::string("text_") + fileTag, width, height, 0xFF303030);

		DX9GF::Font font(gd, family, size);
		float y = 8.0f;
		for (const auto& row : rows) {
			DX9GF::FontSprite sprite(&font);
			sprite.SetText(std::wstring(row.text));
			sprite.SetColor(row.color);
			sprite.SetBold(row.bold);
			if (row.outline) sprite.SetOutline(true, 0xFF000000, row.outlineThickness);
			if (row.maxWidth > 0.0f) sprite.SetMaxWidth(row.maxWidth);

			// Measure unscaled: this is what D3DXFont's DT_CALCRECT reports.
			g_metrics << fileTag << " " << row.id << " width=" << sprite.GetWidth() << " height=" << sprite.GetHeight() << "\n";

			DX9GF::Camera cam = IdentityCamera(width, height);
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
		EndCanvas(canvas);
	}

	// ---- sprites ------------------------------------------------------------------------------

	void RenderSprites(DX9GF::GraphicsDevice* gd)
	{
		const UINT w = 480, h = 320;
		auto canvas = BeginCanvas(gd, "sprites", w, h, 0xFF303030);
		auto token = Load(gd, L"assets/12x12-gold-token.png"); // 264x110 sheet
		auto sheet = Load(gd, L"assets/hkhang-Sheet.png");     // 64x32
		auto borders = Load(gd, L"assets/popup-borders.png");  // 320x160
		DX9GF::Camera cam = IdentityCamera(w, h);
		const RECT tokenRect = { 0, 0, 12, 12 };

		struct Item { float x, y, sx, sy, rot; D3DCOLOR color; bool flipH; };
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
			DX9GF::StaticSprite s(token.get());
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
			DX9GF::StaticSprite s(sheet.get());
			s.SetPosition(8, 70);
			s.Begin(); s.Draw(cam, 0); s.End();
		}
		// Rotation about a centred origin
		{
			DX9GF::StaticSprite s(sheet.get());
			s.SetPosition(120, 90);
			s.SetOrigin(32, 16);
			s.SetRotation(0.7f);
			s.Begin(); s.Draw(cam, 0); s.End();
		}
		// Animated sprite: frames of 16x16 from the sheet
		{
			auto frames = DX9GF::Utils::CreateFrames(64, 32, 16, 16, 8);
			for (unsigned i : { 0u, 3u, 5u }) {
				DX9GF::AnimatedSprite s(sheet.get(), frames, 12, true);
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
			DX9GF::NineSliceSprite s(borders.get(), RECT{ 0, 0, 48, 48 }, 16, 16, 16, 16);
			s.SetTargetSize(n.tw, n.th);
			s.SetScale(n.scale);
			s.SetPosition(n.x, n.y);
			s.Begin(); s.Draw(cam, 0); s.End();
		}
		// Zoomed camera with an offset position
		{
			DX9GF::Camera zoomed = IdentityCamera(w, h);
			zoomed.SetZoom(2.0f);
			zoomed.SetPosition(w / 2.0f + 30.5f, h / 2.0f + 70.25f);
			DX9GF::StaticSprite s(token.get());
			s.SetSrcRect(tokenRect);
			s.SetPosition(w / 2.0f + 10.0f, h / 2.0f + 90.0f);
			s.Begin(); s.Draw(zoomed, 0); s.End();
		}
		EndCanvas(canvas);
	}

	// ---- primitives -----------------------------------------------------------------------------

	void RenderPrimitives(DX9GF::GraphicsDevice* gd)
	{
		const UINT w = 480, h = 320;
		auto canvas = BeginCanvas(gd, "primitives", w, h, 0xFF303030);
		auto token = Load(gd, L"assets/12x12-gold-token.png");
		DX9GF::Camera cam = IdentityCamera(w, h);

		for (int pass = 0; pass < 2; ++pass) {
			const bool blend = pass == 1;
			const float ox = blend ? 240.0f : 0.0f;
			gd->SetAlphaBlending(blend);
			const D3DCOLOR a = blend ? 0x80FF0000 : 0xFFFF0000;
			const D3DCOLOR b = blend ? 0x8000FF00 : 0xFF00FF00;
			const D3DCOLOR c = blend ? 0x800080FF : 0xFF0080FF;

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
			DX9GF::Camera zoomed = IdentityCamera(w, h);
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
		std::vector<D3DXVECTOR2> pts = { {20, 220}, {120, 210}, {150, 290}, {40, 300} };
		std::vector<D3DXVECTOR2> uvs = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
		gd->DrawTexturedPolygon(pts, uvs, token.get());
		std::vector<D3DXVECTOR2> pts2 = { {200, 220}, {300, 220}, {350, 270}, {250, 300}, {190, 270} };
		std::vector<D3DXVECTOR2> uvs2 = { {0.1f, 0}, {0.9f, 0}, {1.4f, 0.5f}, {0.5f, 1.2f}, {-0.2f, 0.6f} };  // outside [0,1]: clamp
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

	void RenderMap(DX9GF::GraphicsDevice* gd, const char* tmx, const char* name, float camX, float camY, float zoom)
	{
		const UINT w = 480, h = 360;
		auto transformManager = std::make_shared<DX9GF::TransformManager>();
		auto colliderManager = std::make_shared<DX9GF::ColliderManager>();
		DX9GF::Map map(gd);
		map.Create(transformManager, colliderManager, tmx);

		auto canvas = BeginCanvas(gd, name, w, h, 0xFF303030);
		DX9GF::Camera cam(static_cast<int>(w), static_cast<int>(h));
		cam.SetZoom(zoom);
		cam.SetPosition(camX, camY);
		map.Draw(cam);
		EndCanvas(canvas);
	}

	class GoldenGame : public DX9GF::IGame {
	public:
		using IGame::IGame;
	};
}

int main(int argc, char** argv)
{
	try {
		g_outDir = (argc > 1) ? argv[1] : "golden";
		std::filesystem::create_directories(g_outDir);
		g_metrics.open(g_outDir / "text_metrics.txt");
		g_metrics << "# Sizes reported by ID3DXFont (DT_CALCRECT) for each text row. Generated by sdlgf_golden_gen.\n";

		auto app = DX9GF::Application::GetInstance();
		app->Init(L"golden", 960, 720, false);
		// Assets are referenced relative to the demo directory
		std::filesystem::current_path(SDLGF_GOLDEN_ASSET_DIR);

		GoldenGame game(app->GetHWnd(), 960, 720);
		game.Init();
		auto gd = game.GetGraphicsDevice();

		const wchar_t* fonts[] = {
			L"assets/135openpixel-v2-3.ttf", L"assets/cardpixel.ttf", L"assets/cardpixel-top.ttf",
			L"assets/cardpixel-bottom.ttf", L"assets/arcade-among-2-r46pv.ttf",
		};
		for (auto f : fonts) DX9GF::Font::AddFont(f);

		RenderTextSheet(gd, L"135OpenPixel v2.3", 13, "openpixel13");
		RenderTextSheet(gd, L"135OpenPixel v2.3", 26, "openpixel26");
		RenderTextSheet(gd, L"135OpenPixel v2.3", 39, "openpixel39");
		RenderTextSheet(gd, L"CardPixel", 18, "cardpixel18");
		RenderTextSheet(gd, L"CardPixelTop", 18, "cardpixeltop18");
		RenderTextSheet(gd, L"CardPixelBottom", 18, "cardpixelbottom18");
		RenderTextSheet(gd, L"Arcade Among 2 R46PV", 24, "arcade24");
		RenderSprites(gd);
		RenderPrimitives(gd);
		RenderMap(gd, "assets/labinside.tmx", "map_labinside_a", 64.0f, 64.0f, 2.0f);
		RenderMap(gd, "assets/labinside.tmx", "map_labinside_b", 50.5f, 70.25f, 3.0f);
		RenderMap(gd, "assets/tutorial.tmx", "map_tutorial", 100.0f, 100.0f, 2.0f);

		g_metrics.close();
		std::puts("done");
		return 0;
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "golden_gen failed: %s\n", e.what());
		return 1;
	}
}
