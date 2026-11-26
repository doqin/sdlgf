#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "GraphicsDevice.h"
#include "Text.h"
#include "Texture.h"

namespace sdlgf {
	/// A TrueType font at one size, registered by its family name (see AddFont), with a cache of
	/// the text it has been asked to draw.
	///
	/// `height` is the CELL height in pixels (ascent + descent) and text is drawn without
	/// antialiasing, exactly like the GDI font it replaces; see Text.h.
	class Font {
	private:
		GraphicsDevice* graphicsDevice;
		std::unique_ptr<text::Face> face;

		struct Entry {
			text::Size size;
			bool rendered = false;
			int offsetX = 0, offsetY = 0; // of the mask relative to the text's layout origin
			std::unique_ptr<Texture> texture; // white, alpha = glyph coverage; null for empty text
		};
		std::unordered_map<std::wstring, Entry> cache;
		Entry& Lookup(const std::wstring& text, float maxWidth);
	public:
		/// Throws std::invalid_argument if no registered file has the family name `fontName`.
		Font(GraphicsDevice* graphicsDevice, std::wstring fontName, INT height);
		~Font();
		Font(const Font&) = delete;
		Font& operator=(const Font&) = delete;

		GraphicsDevice* GetGraphicsDevice();
		const text::Face& GetFace() const { return *face; }

		/// Size of `text` laid out with word wrapping at `maxWidth` (0 = no wrapping).
		text::Size Measure(const std::wstring& text, float maxWidth = 0.0f);

		/// The text as a white coverage texture, and where its top-left corner lies relative to
		/// the text's layout origin. `texture` is null when there is nothing to draw. The pointer
		/// stays valid until the next call to Render.
		struct Rendered {
			Texture* texture = nullptr;
			int offsetX = 0, offsetY = 0;
			int width = 0, height = 0;
		};
		Rendered Render(const std::wstring& text, float maxWidth = 0.0f);

		/// Makes the TrueType file at `path` usable by family name (UTF-16 path, relative to the
		/// working directory). Throws std::runtime_error if the file is missing or not a font.
		static void AddFont(std::wstring path);
		static void RemoveAllFonts();
	};
}
