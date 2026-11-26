#pragma once
// Text layout and rasterisation on top of SDL_ttf, independent of any renderer.
//
// The game's text was written against GDI/ID3DXFont, so this reproduces what that did:
//   * a font "size" is the CELL HEIGHT in pixels (ascent + descent), not the em size
//   * glyphs are rasterised without antialiasing (1 bit per pixel), so pixel fonts stay crisp
//   * line layout follows DrawText: '\n' breaks lines (a trailing '\n' adds none), '\r' and other
//     control characters have no width, optional word wrapping to a maximum width
// The output is an 8-bit coverage mask; a renderer uploads it as a texture and draws it tinted.
// Every behaviour above was measured on the old renderer and is tested against those measurements
// (tests/golden/text_metrics.txt, probe_metrics.txt and the golden images).
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct TTF_Font;

namespace sdlgf::text {

	struct Size {
		int width = 0;
		int height = 0;
	};

	/// 8-bit coverage mask (0 or 255) covering the text's ink and layout box.
	struct Mask {
		int width = 0;
		int height = 0;
		/// Where the mask's top-left corner lies relative to the text's layout origin
		/// (<= 0; negative when a glyph overhangs the left or top of the layout box).
		int offsetX = 0;
		int offsetY = 0;
		std::vector<std::uint8_t> alpha; // width * height, row-major
	};

	/// Makes a TrueType file available to Face by its family name (the replacement for
	/// AddFontResourceEx). Returns false if the file cannot be read or is not TrueType/OpenType.
	bool RegisterFontFile(const std::string& utf8Path);
	/// Forgets every registered file. Existing Face objects keep working.
	void UnregisterAllFontFiles();

	/// One registered font at one size.
	class Face {
	public:
		/// Throws std::invalid_argument if `family` was never registered, std::runtime_error if
		/// the file cannot be opened.
		Face(const std::wstring& family, int cellHeight);
		~Face();
		Face(const Face&) = delete;
		Face& operator=(const Face&) = delete;

		int CellHeight() const { return cellHeight; }
		/// Pixels from the top of a line to its baseline.
		int Ascent() const { return ascent; }
		bool HasGlyph(char32_t codePoint) const;

		/// Size of the laid-out text. `maxWidth` <= 0 disables word wrapping.
		/// Empty text measures 0 x 0.
		Size Measure(std::wstring_view text, float maxWidth = 0.0f) const;
		/// Rasterises the laid-out text.
		Mask Render(std::wstring_view text, float maxWidth = 0.0f) const;

	private:
		struct Line {
			std::u32string chars; // the line's text: leading spaces kept, trailing spaces removed
			int width = 0;        // advance of `chars`
		};
		struct Glyph {
			int advance = 0;                 // pixels, rounded the way GDI rounded font-unit advances
			int minX = 0, maxX = 0, minY = 0, maxY = 0;
			// 1-bit coverage of the glyph (0 or 255), placed relative to the pen position / line top
			int bitmapX = 0, bitmapY = 0, bitmapWidth = 0, bitmapHeight = 0;
			std::vector<std::uint8_t> bitmap;
		};
		const Glyph& GlyphFor(char32_t codePoint) const;
		int Advance(char32_t codePoint) const { return GlyphFor(codePoint).advance; }
		int StringWidth(std::u32string_view s) const;
		std::vector<Line> Layout(std::u32string_view text, int maxWidth) const;

		TTF_Font* font = nullptr;
		TTF_Font* unitsFont = nullptr; // sized so that 1 pixel = 1 font unit: gives exact advances
		int cellHeight = 0;
		int ascent = 0;
		int cellUnits = 0;             // ascent + descent in font units
		struct GlyphCache;
		GlyphCache* cache = nullptr;
	};
}
