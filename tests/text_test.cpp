// Tests the SDL_ttf text module against output captured from the old GDI/D3DXFont renderer:
//   * every size ID3DXFont measured (text_metrics.txt, probe_metrics.txt) must be reproduced exactly
//   * every unscaled text row of the golden sheets is re-rendered on the CPU and compared pixel by pixel
// Rows drawn through a camera zoom or at a fractional position are renderer concerns (sampling rules),
// not text-module ones, and are covered by the renderer tests.
#include "Text.h"
#include "Platform.h"
#include "text_cases.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifndef SDLGF_ASSET_DIR
#	error "SDLGF_ASSET_DIR must point at demo/"
#endif
#ifndef SDLGF_GOLDEN_DIR
#	error "SDLGF_GOLDEN_DIR must point at tests/golden"
#endif

static int g_failures = 0;
#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

using sdlgf::text::Face;
using sdlgf::text::Mask;

static const std::string kAssets = std::string(SDLGF_ASSET_DIR) + "/assets/";
static const std::string kGolden = std::string(SDLGF_GOLDEN_DIR) + "/";

// ---- CPU canvas -----------------------------------------------------------------------------------

struct Canvas {
	int width = 0, height = 0;
	std::vector<std::uint32_t> px; // 0xAARRGGBB, alpha ignored (opaque canvas)

	Canvas(int w, int h, std::uint32_t fill) : width(w), height(h), px(static_cast<size_t>(w) * h, fill) {}

	// Alpha-blends `color` (straight alpha) onto the canvas through a coverage mask.
	void DrawMask(const Mask& mask, int x, int y, std::uint32_t color, int clipTop, int clipBottom)
	{
		const unsigned a = color >> 24;
		for (int my = 0; my < mask.height; ++my) {
			const int py = y + mask.offsetY + my;
			if (py < clipTop || py >= clipBottom || py < 0 || py >= height) continue;
			for (int mx = 0; mx < mask.width; ++mx) {
				if (!mask.alpha[static_cast<size_t>(my) * mask.width + mx]) continue;
				const int pxx = x + mask.offsetX + mx;
				if (pxx < 0 || pxx >= width) continue;
				std::uint32_t& dst = px[static_cast<size_t>(py) * width + pxx];
				if (a == 255) { dst = 0xFF000000u | (color & 0xFFFFFF); continue; }
				std::uint32_t out = 0xFF000000u;
				for (int shift = 0; shift <= 16; shift += 8) {
					const unsigned s = (color >> shift) & 0xFF, d = (dst >> shift) & 0xFF;
					out |= (((s * a + d * (255 - a)) + 127) / 255) << shift;
				}
				dst = out;
			}
		}
	}
};

// The same composition sdlgf::FontSprite::Draw performs: outline copies first, then the text; "bold" is a
// second copy one pixel to the right.
static void ComposeRow(Canvas& canvas, const Face& face, const golden::TextRow& row, int originX, int originY, int clipTop, int clipBottom)
{
	const Mask mask = face.Render(row.text, row.maxWidth);
	const std::uint32_t outlineColor = 0xFF000000u;
	if (row.outline) {
		const float t = row.outlineThickness;
		for (float ox = -t; ox <= t; ox += 1.0f) {
			for (float oy = -t; oy <= t; oy += 1.0f) {
				if (ox * ox + oy * oy > t * t) continue;
				canvas.DrawMask(mask, originX + static_cast<int>(ox), originY + static_cast<int>(oy), outlineColor, clipTop, clipBottom);
				if (row.bold) canvas.DrawMask(mask, originX + static_cast<int>(ox) + 1, originY + static_cast<int>(oy), outlineColor, clipTop, clipBottom);
			}
		}
	}
	canvas.DrawMask(mask, originX, originY, row.color, clipTop, clipBottom);
	if (row.bold) canvas.DrawMask(mask, originX + 1, originY, row.color, clipTop, clipBottom);
}

static bool LoadPng(const std::string& path, Canvas& out)
{
	SDL_Surface* loaded = SDL_LoadSurface(path.c_str());
	if (!loaded) { std::fprintf(stderr, "cannot load %s: %s\n", path.c_str(), SDL_GetError()); return false; }
	SDL_Surface* s = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888);
	SDL_DestroySurface(loaded);
	if (!s) return false;
	out = Canvas(s->w, s->h, 0);
	for (int y = 0; y < s->h; ++y) {
		std::memcpy(&out.px[static_cast<size_t>(y) * s->w], static_cast<const char*>(s->pixels) + static_cast<size_t>(y) * s->pitch, static_cast<size_t>(s->w) * 4);
	}
	SDL_DestroySurface(s);
	return true;
}

static int MaxChannelDiff(std::uint32_t a, std::uint32_t b)
{
	int d = 0;
	for (int shift = 0; shift <= 16; shift += 8) {
		d = std::max(d, std::abs(static_cast<int>((a >> shift) & 0xFF) - static_cast<int>((b >> shift) & 0xFF)));
	}
	return d;
}

// ---- helpers ----------------------------------------------------------------------------------------

static bool AllGlyphsPresent(const Face& face, const wchar_t* text)
{
	for (const char32_t c : sdlgf::Platform::WideToUtf32(text)) {
		if (c >= 0x20 && c != 0x7F && !face.HasGlyph(c)) return false;
	}
	return true;
}

struct Measured { int width, height; };

// Parses lines like "tag id [maxWidth=N] width=W height=H" into key -> size ("tag/id" or "id").
static std::map<std::string, Measured> ReadMetrics(const std::string& path, bool hasTag)
{
	std::map<std::string, Measured> result;
	std::ifstream in(path);
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || line[0] == '#') continue;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		std::istringstream ss(line);
		std::string tag, id, token;
		if (hasTag) ss >> tag;
		ss >> id;
		Measured m{ -1, -1 };
		while (ss >> token) {
			if (token.rfind("width=", 0) == 0) m.width = std::stoi(token.substr(6));
			if (token.rfind("height=", 0) == 0) m.height = std::stoi(token.substr(7));
		}
		result[hasTag ? tag + "/" + id : id] = m;
	}
	return result;
}

// ---- tests ----------------------------------------------------------------------------------------------

static void TestRegistry()
{
	CHECK(sdlgf::text::RegisterFontFile(kAssets + "135openpixel-v2-3.ttf"));
	CHECK(sdlgf::text::RegisterFontFile(kAssets + "cardpixel.ttf"));
	CHECK(sdlgf::text::RegisterFontFile(kAssets + "cardpixel-top.ttf"));
	CHECK(sdlgf::text::RegisterFontFile(kAssets + "cardpixel-bottom.ttf"));
	CHECK(sdlgf::text::RegisterFontFile(kAssets + "arcade-among-2-r46pv.ttf"));
	CHECK(!sdlgf::text::RegisterFontFile(kAssets + "icon.png"));           // not a font
	CHECK(!sdlgf::text::RegisterFontFile(kAssets + "does-not-exist.ttf"));

	bool threw = false;
	try { Face f(L"No Such Family", 13); } catch (const std::invalid_argument&) { threw = true; }
	CHECK(threw);

	// family lookup ignores case, like GDI
	Face lower(L"135openpixel V2.3", 13);
	CHECK(lower.CellHeight() == 13);
	CHECK(lower.Ascent() == 10);   // 1818/2363 of the cell, as GDI reported (10 up, 3 down)
}

static void TestFixedMetrics()
{
	const auto metrics = ReadMetrics(kGolden + "text_metrics.txt", true);
	CHECK(!metrics.empty());
	int compared = 0, skipped = 0;
	for (const auto& font : golden::Fonts()) {
		Face face(font.family, font.size);
		for (const auto& row : golden::TextRows()) {
			const auto it = metrics.find(std::string(font.tag) + "/" + row.id);
			if (it == metrics.end()) { std::fprintf(stderr, "no golden metric for %s/%s\n", font.tag, row.id); ++g_failures; continue; }
			if (!AllGlyphsPresent(face, row.text)) { ++skipped; continue; }   // GDI substituted another font there
			const auto size = face.Measure(row.text, row.maxWidth);
			if (size.width != it->second.width || size.height != it->second.height) {
				std::fprintf(stderr, "metric mismatch %s/%s: golden %dx%d, ours %dx%d\n", font.tag, row.id, it->second.width, it->second.height, size.width, size.height);
				++g_failures;
			}
			++compared;
		}
	}
	std::printf("  text metrics: %d compared exactly, %d skipped (glyphs missing from that font)\n", compared, skipped);
	CHECK(compared >= 100);
}

static void TestProbeMetrics()
{
	const auto metrics = ReadMetrics(kGolden + "probe_metrics.txt", false);
	Face face(L"135OpenPixel v2.3", 13);
	int compared = 0;
	for (const auto& probe : golden::Probes()) {
		const auto it = metrics.find(probe.id);
		if (it == metrics.end()) { std::fprintf(stderr, "no golden probe metric for %s\n", probe.id); ++g_failures; continue; }
		const auto size = face.Measure(probe.text, probe.maxWidth);
		if (size.width != it->second.width || size.height != it->second.height) {
			std::fprintf(stderr, "probe mismatch %s: golden %dx%d, ours %dx%d\n", probe.id, it->second.width, it->second.height, size.width, size.height);
			++g_failures;
		}
		++compared;
	}
	std::printf("  layout probes: %d compared exactly\n", compared);
	CHECK(compared == static_cast<int>(golden::Probes().size()));
}

// Prints the pixels around (cx, cy) in both images ('#' = lit, '.' = background), golden on the left.
static void DumpRegion(const Canvas& expected, const Canvas& golden, int cx, int cy)
{
	std::fprintf(stderr, "    golden (left) vs ours (right) around (%d,%d); '!' marks the differing pixel\n", cx, cy);
	for (int y = cy - 6; y <= cy + 6; ++y) {
		if (y < 0 || y >= expected.height) continue;
		std::string left, right;
		for (int x = cx - 10; x <= cx + 10; ++x) {
			if (x < 0 || x >= expected.width) { left += ' '; right += ' '; continue; }
			const auto g = golden.px[static_cast<size_t>(y) * golden.width + x];
			const auto e = expected.px[static_cast<size_t>(y) * expected.width + x];
			const bool diff = MaxChannelDiff(g, e) > 1;
			left += diff ? '!' : ((g & 0xFFFFFF) != 0x303030 ? '#' : '.');
			right += diff ? '!' : ((e & 0xFFFFFF) != 0x303030 ? '#' : '.');
		}
		std::fprintf(stderr, "    %s   %s\n", left.c_str(), right.c_str());
	}
}

static int ComparePixels(const Canvas& expected, const Canvas& golden, int x0, int y0, int x1, int y1, int tolerance, int* firstX, int* firstY)
{
	int bad = 0;
	for (int y = y0; y < y1; ++y) {
		for (int x = x0; x < x1; ++x) {
			const auto e = expected.px[static_cast<size_t>(y) * expected.width + x];
			const auto g = golden.px[static_cast<size_t>(y) * golden.width + x];
			if (MaxChannelDiff(e, g) > tolerance) {
				if (bad == 0) { *firstX = x; *firstY = y; }
				++bad;
			}
		}
	}
	return bad;
}

static void TestSheets()
{
	int rowsCompared = 0, rowsSkipped = 0;
	for (const auto& font : golden::Fonts()) {
		Canvas golden_png(0, 0, 0);
		CHECK(LoadPng(kGolden + "text_" + font.tag + ".png", golden_png));
		if (golden_png.width == 0) continue;
		Face face(font.family, font.size);
		const float step = golden::RowStep(font.size);

		// Draw every reproducible row in order onto one canvas, exactly like the generator did: a tall
		// row (7 wrapped lines of a 39 px font) overlaps the rows below it, and the golden image
		// contains both.
		Canvas expected(golden_png.width, golden_png.height, 0xFF303030);
		struct Band { const golden::TextRow* row; int top, bottom; };
		std::vector<Band> bands;
		float y = 8.0f;
		for (const auto& row : golden::TextRows()) {
			const float rowY = y;
			y += step;
			if (row.zoom != 1.0f || row.yFraction != 0.0f) { ++rowsSkipped; continue; }
			if (!AllGlyphsPresent(face, row.text)) { ++rowsSkipped; continue; }
			ComposeRow(expected, face, row, static_cast<int>(row.x), static_cast<int>(rowY), 0, expected.height);
			// the band around the row; clamped to the image (the first row of a large font starts above y = 0)
			const int top = std::max(0, static_cast<int>(rowY) - font.size / 2);
			const int bottom = std::min(golden_png.height, static_cast<int>(rowY) - font.size / 2 + static_cast<int>(step));
			bands.push_back({ &row, top, bottom });
		}
		for (const auto& band : bands) {
			int fx = 0, fy = 0;
			const int bad = ComparePixels(expected, golden_png, 0, band.top, golden_png.width, band.bottom, 1, &fx, &fy);
			// Rows where FreeType's scan conversion and GDI's disagree by whole pixels. Each is listed with its
			// exact count so a regression (more) or a change (fewer) is noticed.
			static const std::map<std::string, int> knownDifferences = {
				{ "openpixel39/vietnamese", 1 },   // one extra lit pixel under the circumflex of the 'e' in "vien", GDI only
			};
			const auto known = knownDifferences.find(std::string(font.tag) + "/" + band.row->id);
			const int allowed = known == knownDifferences.end() ? 0 : known->second;
			if (bad != allowed) {
				std::fprintf(stderr, "pixel mismatch in %s/%s: %d pixels differ, first at (%d,%d): golden %08X, ours %08X\n", font.tag, band.row->id, bad, fx, fy,
					(unsigned)golden_png.px[static_cast<size_t>(fy) * golden_png.width + fx], (unsigned)expected.px[static_cast<size_t>(fy) * expected.width + fx]);
				++g_failures;
				DumpRegion(expected, golden_png, fx, fy);
			}
			++rowsCompared;
		}
	}
	std::printf("  pixel rows: %d compared, %d skipped (zoomed/fractional rows or glyphs missing)\n", rowsCompared, rowsSkipped);
	CHECK(rowsCompared >= 80);
}

static void TestProbeImage()
{
	Canvas golden_png(0, 0, 0);
	CHECK(LoadPng(kGolden + "text_probe.png", golden_png));
	if (golden_png.width == 0) return;
	Face face(L"135OpenPixel v2.3", 13);
	const float step = 13.0f * 6.0f;
	Canvas expected(golden_png.width, golden_png.height, 0xFF303030);
	float y = 8.0f;
	int rows = 0;
	for (const auto& probe : golden::Probes()) {
		if (probe.maxWidth > 0.0f) {   // the guide bar the generator drew: opaque 1 px high line
			for (int x = 8; x < 8 + static_cast<int>(probe.maxWidth); ++x) expected.px[static_cast<size_t>(static_cast<int>(y)) * expected.width + x] = 0xFF804040;
		}
		golden::TextRow row{ probe.id, probe.text };
		row.maxWidth = probe.maxWidth;
		ComposeRow(expected, face, row, 8, static_cast<int>(y) + 2, 0, expected.height);
		y += step;
		++rows;
	}
	int fx = 0, fy = 0;
	const int bad = ComparePixels(expected, golden_png, 0, 0, golden_png.width, golden_png.height, 0, &fx, &fy);
	if (bad) {
		const int row = (fy - 8) / static_cast<int>(step);
		std::fprintf(stderr, "probe image: %d pixels differ, first at (%d,%d) in probe '%s'\n", bad, fx, fy,
			row >= 0 && row < static_cast<int>(golden::Probes().size()) ? golden::Probes()[row].id : "?");
		++g_failures;
	}
	std::printf("  probe image: %d probes drawn and compared\n", rows);
}

int main()
{
	std::setvbuf(stdout, nullptr, _IONBF, 0);   // keep progress visible if something crashes
	TestRegistry();
	TestFixedMetrics();
	TestProbeMetrics();
	TestSheets();
	TestProbeImage();
	sdlgf::text::UnregisterAllFontFiles();
	if (g_failures == 0) std::puts("text_test: all tests passed");
	else std::fprintf(stderr, "text_test: %d failure(s)\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
