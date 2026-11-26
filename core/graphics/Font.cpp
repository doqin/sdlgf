#include "pch.h"
#include "Font.h"
#include <stdexcept>
#include <string>

namespace {
	// Text is cached by content; dynamic strings (scores, timers) would otherwise grow the cache
	// without bound, so it is simply emptied when it gets large.
	constexpr size_t kMaxCachedStrings = 1024;
}

sdlgf::Font::Font(GraphicsDevice* graphicsDevice, std::wstring fontName, INT height) : graphicsDevice(graphicsDevice)
{
	face = std::make_unique<text::Face>(fontName, height);
}

sdlgf::Font::~Font() = default;

sdlgf::GraphicsDevice* sdlgf::Font::GetGraphicsDevice()
{
	return graphicsDevice;
}

sdlgf::Font::Entry& sdlgf::Font::Lookup(const std::wstring& text, float maxWidth)
{
	// Layout only sees whole pixels of the wrap width
	const int wrap = maxWidth > 0.0f ? static_cast<int>(maxWidth) : 0;
	std::wstring key = std::to_wstring(wrap);
	key += L'|';
	key += text;
	auto it = cache.find(key);
	if (it != cache.end()) return it->second;

	if (cache.size() >= kMaxCachedStrings) cache.clear();
	Entry entry;
	entry.size = face->Measure(text, static_cast<float>(wrap));
	return cache.emplace(std::move(key), std::move(entry)).first->second;
}

sdlgf::text::Size sdlgf::Font::Measure(const std::wstring& text, float maxWidth)
{
	return Lookup(text, maxWidth).size;
}

sdlgf::Font::Rendered sdlgf::Font::Render(const std::wstring& text, float maxWidth)
{
	Entry& entry = Lookup(text, maxWidth);
	if (!entry.rendered) {
		entry.rendered = true;
		const text::Mask mask = face->Render(text, maxWidth > 0.0f ? static_cast<float>(static_cast<int>(maxWidth)) : 0.0f);
		if (mask.width > 0 && mask.height > 0) {
			std::vector<std::uint32_t> pixels(mask.alpha.size());
			for (size_t i = 0; i < pixels.size(); ++i) {
				pixels[i] = (static_cast<std::uint32_t>(mask.alpha[i]) << 24) | 0x00FFFFFFu;
			}
			entry.texture = std::make_unique<Texture>(graphicsDevice);
			entry.texture->CreateFromPixels(pixels.data(), static_cast<UINT>(mask.width), static_cast<UINT>(mask.height));
			entry.offsetX = mask.offsetX;
			entry.offsetY = mask.offsetY;
		}
	}
	Rendered result;
	result.texture = entry.texture.get();
	result.offsetX = entry.offsetX;
	result.offsetY = entry.offsetY;
	if (entry.texture) {
		result.width = static_cast<int>(entry.texture->GetWidth());
		result.height = static_cast<int>(entry.texture->GetHeight());
	}
	return result;
}

void sdlgf::Font::AddFont(std::wstring path)
{
	if (!text::RegisterFontFile(sdlgf::Platform::WideToUtf8(path))) {
		throw std::runtime_error("Could not load font file \"" + sdlgf::Platform::WideToUtf8(path) + "\"");
	}
}

void sdlgf::Font::RemoveAllFonts()
{
	text::UnregisterAllFontFiles();
}
