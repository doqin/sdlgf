#include "Text.h"

#include "Platform.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace sdlgf::text {

namespace {

	// ---- registry of font files, by family name ---------------------------------------------

	struct FontFile {
		std::string path;
		int unitsPerEm = 0;
		int ascent = 0;   // font units; GDI's cell height is ascent + descent
		int descent = 0;
	};

	std::map<std::string, FontFile>& Registry()
	{
		static std::map<std::string, FontFile> files;
		return files;
	}

	std::string LowerAscii(std::string s)
	{
		for (char& c : s) {
			if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
		}
		return s;
	}

	// ---- minimal sfnt (TrueType/OpenType) reader: only what the registry needs ----------------

	struct Reader {
		const std::uint8_t* data;
		size_t size;
		bool Has(size_t off, size_t len) const { return off <= size && len <= size - off; }
		std::uint16_t U16(size_t off) const { return static_cast<std::uint16_t>((data[off] << 8) | data[off + 1]); }
		std::int16_t S16(size_t off) const { return static_cast<std::int16_t>(U16(off)); }
		std::uint32_t U32(size_t off) const { return (static_cast<std::uint32_t>(U16(off)) << 16) | U16(off + 2); }
	};

	bool FindTable(const Reader& r, const char tag[4], size_t& offset, size_t& length)
	{
		if (!r.Has(0, 12)) return false;
		const std::uint32_t version = r.U32(0);
		if (version != 0x00010000u && version != 0x4F54544Fu /* OTTO */ && version != 0x74727565u /* true */) return false;
		const unsigned count = r.U16(4);
		for (unsigned i = 0; i < count; ++i) {
			const size_t rec = 12 + 16u * i;
			if (!r.Has(rec, 16)) return false;
			if (std::memcmp(r.data + rec, tag, 4) == 0) {
				offset = r.U32(rec + 8);
				length = r.U32(rec + 12);
				return r.Has(offset, length);
			}
		}
		return false;
	}

	// Family names (name IDs 1 and 16) as UTF-8.
	std::vector<std::string> FamilyNames(const Reader& r)
	{
		std::vector<std::string> names;
		size_t off = 0, len = 0;
		if (!FindTable(r, "name", off, len) || len < 6) return names;
		const unsigned count = r.U16(off + 2);
		const size_t stringBase = off + r.U16(off + 4);
		for (unsigned i = 0; i < count; ++i) {
			const size_t rec = off + 6 + 12u * i;
			if (!r.Has(rec, 12)) break;
			const unsigned platform = r.U16(rec), nameId = r.U16(rec + 6);
			const size_t strLen = r.U16(rec + 8), strOff = stringBase + r.U16(rec + 10);
			if ((nameId != 1 && nameId != 16) || !r.Has(strOff, strLen)) continue;
			std::u32string cps;
			if (platform == 3 || platform == 0) { // UTF-16BE
				for (size_t k = 0; k + 1 < strLen; k += 2) {
					char32_t u = r.U16(strOff + k);
					if (u >= 0xD800 && u <= 0xDBFF && k + 3 < strLen) {
						const char32_t lo = r.U16(strOff + k + 2);
						if (lo >= 0xDC00 && lo <= 0xDFFF) { u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00); k += 2; }
					}
					cps += u;
				}
			}
			else if (platform == 1) { // Mac Roman: ASCII subset is enough for family names
				for (size_t k = 0; k < strLen; ++k) cps += static_cast<char32_t>(r.data[strOff + k]);
			}
			if (!cps.empty()) names.push_back(LowerAscii(Platform::Utf32ToUtf8(cps)));
		}
		return names;
	}

	bool ReadFontInfo(const Reader& r, FontFile& info)
	{
		size_t off = 0, len = 0;
		if (!FindTable(r, "head", off, len) || len < 20) return false;
		info.unitsPerEm = r.U16(off + 18);
		// GDI's tmAscent/tmDescent come from the OS/2 "win" metrics, falling back to hhea
		bool have = false;
		if (FindTable(r, "OS/2", off, len) && len >= 78) {
			info.ascent = r.U16(off + 74);
			info.descent = r.U16(off + 76);
			have = info.ascent + info.descent > 0;
		}
		if (!have) {
			if (!FindTable(r, "hhea", off, len) || len < 8) return false;
			info.ascent = r.S16(off + 4);
			info.descent = -r.S16(off + 6);
		}
		return info.unitsPerEm > 0 && info.ascent + info.descent > 0;
	}

	bool IsControl(char32_t c) { return c < 0x20 || (c >= 0x7F && c < 0xA0); }

	std::u32string TrimTrailingSpaces(std::u32string s)
	{
		while (!s.empty() && s.back() == U' ') s.pop_back();
		return s;
	}
}

bool RegisterFontFile(const std::string& utf8Path)
{
	size_t size = 0;
	void* raw = SDL_LoadFile(utf8Path.c_str(), &size);
	if (raw == nullptr) return false;
	const Reader reader{ static_cast<const std::uint8_t*>(raw), size };
	FontFile info;
	info.path = utf8Path;
	bool ok = ReadFontInfo(reader, info);
	std::vector<std::string> names;
	if (ok) names = FamilyNames(reader);
	SDL_free(raw);
	if (!ok || names.empty()) return false;
	for (const auto& name : names) Registry()[name] = info;
	return true;
}

void UnregisterAllFontFiles()
{
	Registry().clear();
}

// ---- Face -----------------------------------------------------------------------------------------

struct Face::GlyphCache {
	std::unordered_map<char32_t, Face::Glyph> glyphs;
};

Face::Face(const std::wstring& family, int cellHeight) : cellHeight(cellHeight)
{
	if (cellHeight <= 0) throw std::invalid_argument("font size must be positive");
	const std::string key = LowerAscii(Platform::WideToUtf8(family));
	const auto it = Registry().find(key);
	if (it == Registry().end()) throw std::invalid_argument("font family not registered: " + Platform::WideToUtf8(family));
	const FontFile& file = it->second;

	if (!TTF_Init()) throw std::runtime_error(std::string("TTF_Init failed: ") + SDL_GetError());
	// A cell of `cellHeight` pixels holds (ascent + descent) font units, so the em square is
	// cellHeight * unitsPerEm / (ascent + descent) pixels. SDL_ttf sizes fonts by em (72 dpi).
	const float em = static_cast<float>(cellHeight) * static_cast<float>(file.unitsPerEm)
		/ static_cast<float>(file.ascent + file.descent);
	font = TTF_OpenFont(file.path.c_str(), em);
	// A second face at one pixel per font unit: SDL_ttf reports advances as whole pixels (rounded up),
	// so the unit-sized face gives exact advances that we round the way GDI did.
	unitsFont = font ? TTF_OpenFont(file.path.c_str(), static_cast<float>(file.unitsPerEm)) : nullptr;
	if (font == nullptr || unitsFont == nullptr) {
		if (font) TTF_CloseFont(font);
		if (unitsFont) TTF_CloseFont(unitsFont);
		font = unitsFont = nullptr;
		TTF_Quit();
		throw std::runtime_error("could not open font " + file.path + ": " + SDL_GetError());
	}
	// Pure outline scan conversion: no grid fitting, no kerning (GDI applied neither).
	for (TTF_Font* f : { font, unitsFont }) {
		TTF_SetFontHinting(f, TTF_HINTING_NONE);
		TTF_SetFontKerning(f, false);
	}
	cellUnits = file.ascent + file.descent;
	ascent = static_cast<int>(std::lround(static_cast<double>(file.ascent) * cellHeight / (file.ascent + file.descent)));
	cache = new GlyphCache();
}

Face::~Face()
{
	delete cache;
	if (font != nullptr) {
		TTF_CloseFont(font);
		TTF_CloseFont(unitsFont);
		TTF_Quit();
	}
}

bool Face::HasGlyph(char32_t codePoint) const
{
	return TTF_FontHasGlyph(font, codePoint);
}

const Face::Glyph& Face::GlyphFor(char32_t cp) const
{
	const auto found = cache->glyphs.find(cp);
	if (found != cache->glyphs.end()) return found->second;
	Glyph g;
	if (!IsControl(cp)) {
		int minX = 0, maxX = 0, minY = 0, maxY = 0, advance = 0, unitsAdvance = 0;
		if (TTF_GetGlyphMetrics(font, cp, &minX, &maxX, &minY, &maxY, &advance)) {
			g.minX = minX; g.maxX = maxX; g.minY = minY; g.maxY = maxY;
			g.advance = advance;
			int a = 0, b = 0, c = 0, d = 0;
			if (TTF_GetGlyphMetrics(unitsFont, cp, &a, &b, &c, &d, &unitsAdvance)) {
				g.advance = static_cast<int>(std::lround(static_cast<double>(unitsAdvance) * cellHeight / cellUnits));
			}
		}
		// SDL_ttf starts a rendered string at the leftmost ink when a glyph has a negative left bearing
		const std::string utf8 = Platform::Utf32ToUtf8(std::u32string(1, cp));
		if (SDL_Surface* s = TTF_RenderText_Solid(font, utf8.c_str(), utf8.size(), SDL_Color{ 255, 255, 255, 255 })) {
			g.bitmapX = std::min(0, g.minX);
			g.bitmapY = ascent - TTF_GetFontAscent(font);
			g.bitmapWidth = s->w;
			g.bitmapHeight = s->h;
			g.bitmap.resize(static_cast<size_t>(s->w) * s->h);
			const auto* pixels = static_cast<const std::uint8_t*>(s->pixels);
			for (int y = 0; y < s->h; ++y)
				for (int x = 0; x < s->w; ++x)
					g.bitmap[static_cast<size_t>(y) * s->w + x] = pixels[y * s->pitch + x] ? 255 : 0;
			SDL_DestroySurface(s);
		}
	}
	return cache->glyphs.emplace(cp, std::move(g)).first->second;
}

int Face::StringWidth(std::u32string_view s) const
{
	int w = 0;
	for (const char32_t c : s) w += Advance(c);
	return w;
}

// Line layout, matching what ID3DXFont::DrawText (GDI) produced:
//  - '\n' ends a line; a trailing '\n' does not add an empty one; '\r' is ignored
//  - spaces at the end of a line are not part of its width
//  - with a maximum width, a space is a character like any other: if it fits on the line it stays
//    there (invisible, trailing), if not it moves to the START of the next line, where it is kept
//  - the first word on a line is placed whenever it fits (<= max); every later word also needs
//    room for the space that follows it (GDI breaks "that has to wrap" before "wrap" if the
//    trailing space would not fit either), except the last word of a paragraph
//  - a word wider than the box is broken between characters; a lone character wider than the box
//    gets its own line (when it is the very first character, GDI emitted an empty line before it)
std::vector<Face::Line> Face::Layout(std::u32string_view text, int maxWidth) const
{
	std::vector<Line> lines;
	std::u32string clean;
	for (const char32_t c : text) {
		if (c != U'\r') clean += c;
	}
	if (clean.empty()) return lines;
	if (clean.back() == U'\n') clean.pop_back();

	const auto emit = [&](const std::u32string& s) {
		Line line;
		line.chars = TrimTrailingSpaces(s);
		line.width = StringWidth(line.chars);
		lines.push_back(std::move(line));
	};

	const int spaceAdvance = Advance(U' ');
	bool oddityDone = false; // the "empty first line" quirk applies once, at the start of the text
	size_t start = 0;
	while (start <= clean.size()) {
		size_t end = clean.find(U'\n', start);
		if (end == std::u32string::npos) end = clean.size();
		const std::u32string paragraph = clean.substr(start, end - start);
		start = end + 1;

		if (maxWidth <= 0 || paragraph.empty()) {
			emit(paragraph);
			if (end >= clean.size()) break;
			continue;
		}

		std::u32string line;
		int lineWidth = 0;
		bool lineHasWord = false;
		const auto flush = [&]() { emit(line); line.clear(); lineWidth = 0; lineHasWord = false; };
		// Puts a word that does not follow other words on its line: whole if it fits, else broken
		// between characters.
		const auto placeFirstWord = [&](const std::u32string& word) {
			if (lineWidth + StringWidth(word) <= maxWidth) {
				line += word; lineWidth += StringWidth(word); lineHasWord = true;
				return;
			}
			for (const char32_t ch : word) {
				const int a = Advance(ch);
				if (lineHasWord && lineWidth + a > maxWidth) {
					flush();
				}
				else if (!lineHasWord && lineWidth == 0 && a > maxWidth && lines.empty() && !oddityDone) {
					emit(std::u32string());
					oddityDone = true;
				}
				line += ch; lineWidth += a; lineHasWord = true;
			}
		};

		size_t i = 0;
		while (i < paragraph.size()) {
			if (paragraph[i] == U' ') {
				if (lineWidth + spaceAdvance <= maxWidth) { line += U' '; lineWidth += spaceAdvance; }
				else { flush(); line = U" "; lineWidth = spaceAdvance; } // spills to the next line, kept there
				++i;
				continue;
			}
			const size_t wordStart = i;
			while (i < paragraph.size() && paragraph[i] != U' ') ++i;
			const std::u32string word = paragraph.substr(wordStart, i - wordStart);
			const int followingSpace = (i < paragraph.size()) ? spaceAdvance : 0;
			if (!lineHasWord) {
				placeFirstWord(word);
			}
			else if (lineWidth + StringWidth(word) + followingSpace <= maxWidth) {
				line += word; lineWidth += StringWidth(word); lineHasWord = true;
			}
			else {
				flush();
				placeFirstWord(word);
			}
		}
		flush();
		if (end >= clean.size()) break;
	}
	return lines;
}

Size Face::Measure(std::wstring_view text, float maxWidth) const
{
	const auto lines = Layout(Platform::WideToUtf32(text), static_cast<int>(maxWidth));
	Size size;
	for (const auto& line : lines) size.width = std::max(size.width, line.width);
	size.height = static_cast<int>(lines.size()) * cellHeight;
	return size;
}

Mask Face::Render(std::wstring_view text, float maxWidth) const
{
	const auto lines = Layout(Platform::WideToUtf32(text), static_cast<int>(maxWidth));
	Mask mask;
	if (lines.empty()) return mask;

	// First pass: where does every glyph bitmap go (relative to the layout origin)?
	struct Placed { const Glyph* glyph; int x, y, skipColumns; };
	std::vector<Placed> placed;
	int layoutWidth = 0;
	for (const auto& line : lines) layoutWidth = std::max(layoutWidth, line.width);
	int minX = 0, minY = 0, maxX = layoutWidth, maxY = static_cast<int>(lines.size()) * cellHeight;
	for (size_t row = 0; row < lines.size(); ++row) {
		int pen = 0;
		for (const char32_t c : lines[row].chars) {
			const Glyph& g = GlyphFor(c);
			if (g.bitmapWidth > 0 && g.bitmapHeight > 0) {
				// Ink left of the pen position (negative left bearing, e.g. the bar of a crossed D) is
				// not drawn: ID3DXFont/GDI clipped every glyph to start at its pen position.
				const int skip = std::max(0, -g.bitmapX);
				const int x = pen + g.bitmapX + skip;
				const int y = static_cast<int>(row) * cellHeight + g.bitmapY;
				placed.push_back({ &g, x, y, skip });
				minX = std::min(minX, x);
				minY = std::min(minY, y);
				maxX = std::max(maxX, x + g.bitmapWidth - skip);
				maxY = std::max(maxY, y + g.bitmapHeight);
			}
			pen += g.advance;
		}
	}

	mask.offsetX = minX;
	mask.offsetY = minY;
	mask.width = maxX - minX;
	mask.height = maxY - minY;
	mask.alpha.assign(static_cast<size_t>(mask.width) * mask.height, 0);
	for (const auto& p : placed) {
		const Glyph& g = *p.glyph;
		for (int gy = 0; gy < g.bitmapHeight; ++gy) {
			for (int gx = p.skipColumns; gx < g.bitmapWidth; ++gx) {
				if (g.bitmap[static_cast<size_t>(gy) * g.bitmapWidth + gx]) {
					mask.alpha[static_cast<size_t>(p.y + gy - minY) * mask.width + (p.x + (gx - p.skipColumns) - minX)] = 255;
				}
			}
		}
	}
	return mask;
}

}
