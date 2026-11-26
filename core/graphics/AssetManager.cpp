#include "pch.h"
#include "AssetManager.h"
#include <stdexcept>

std::wstring sdlgf::AssetManager::FontKey(const std::wstring& family, INT height)
{
	return family + L'\n' + std::to_wstring(height);
}

std::shared_ptr<sdlgf::Texture> sdlgf::AssetManager::FindTexture(const std::wstring& path) const
{
	auto it = textures.find(path);
	return it != textures.end() ? it->second.lock() : nullptr;
}

std::shared_ptr<sdlgf::Texture> sdlgf::AssetManager::GetTexture(const std::wstring& path)
{
	if (recording && recordedSet.insert(path).second) recordedPaths.push_back(path);
	if (auto existing = FindTexture(path)) return existing;
	auto texture = std::make_shared<Texture>(graphicsDevice);
	texture->LoadTexture(path);
	textures[path] = texture;
	return texture;
}

std::shared_ptr<sdlgf::Texture> sdlgf::AssetManager::AdoptTexture(const std::wstring& path, std::shared_ptr<Texture> texture)
{
	if (auto existing = FindTexture(path)) return existing;
	textures[path] = texture;
	return texture;
}

std::shared_ptr<sdlgf::Font> sdlgf::AssetManager::GetFont(const std::wstring& family, INT height)
{
	const std::wstring key = FontKey(family, height);
	auto it = fonts.find(key);
	if (it != fonts.end()) {
		if (auto existing = it->second.lock()) return existing;
	}
	auto font = std::make_shared<Font>(graphicsDevice, family, height);
	fonts[key] = font;
	return font;
}

void sdlgf::AssetManager::Prune()
{
	for (auto it = textures.begin(); it != textures.end();) it = it->second.expired() ? textures.erase(it) : std::next(it);
	for (auto it = fonts.begin(); it != fonts.end();) it = it->second.expired() ? fonts.erase(it) : std::next(it);
}

size_t sdlgf::AssetManager::LiveTextureCount() const
{
	size_t n = 0;
	for (const auto& entry : textures) if (!entry.second.expired()) ++n;
	return n;
}

void sdlgf::AssetManager::StartRecording()
{
	recording = true;
	recordedPaths.clear();
	recordedSet.clear();
}

std::vector<std::wstring> sdlgf::AssetManager::StopRecording()
{
	recording = false;
	recordedSet.clear();
	return std::move(recordedPaths);
}
