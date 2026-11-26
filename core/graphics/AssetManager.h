#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Font.h"
#include "Texture.h"

namespace sdlgf {
	class GraphicsDevice;

	/// A shared pool of the heavy, immutable assets (textures loaded from files, fonts), owned by
	/// the GraphicsDevice they belong to (GraphicsDevice::GetAssets()).
	///
	/// Objects never own a private copy of an asset: they ask the pool and hold the returned
	/// shared_ptr. The pool itself only keeps weak references, so an asset is freed as soon as
	/// the last object (or AssetSet, see below) using it goes away - no explicit unload calls.
	///
	/// Textures from GetTexture are SHARED: do not change their pixels (SetColor, ReleaseTexture).
	/// For a texture you intend to modify or render into, make your own with
	/// std::make_shared<Texture>(graphicsDevice).
	///
	/// Main thread only (it creates GPU textures).
	class AssetManager final {
	private:
		GraphicsDevice* graphicsDevice;
		std::unordered_map<std::wstring, std::weak_ptr<Texture>> textures;
		std::unordered_map<std::wstring, std::weak_ptr<Font>> fonts;
		bool recording = false;
		std::vector<std::wstring> recordedPaths;
		std::unordered_set<std::wstring> recordedSet;

		static std::wstring FontKey(const std::wstring& family, INT height);
	public:
		explicit AssetManager(GraphicsDevice* graphicsDevice) : graphicsDevice(graphicsDevice) {}
		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;

		GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice; }

		/// The image at `path`, loading it if no one is using it right now.
		/// Throws std::runtime_error naming the file if it cannot be loaded.
		std::shared_ptr<Texture> GetTexture(const std::wstring& path);
		/// The image at `path` if it is already in memory, otherwise null (never loads).
		std::shared_ptr<Texture> FindTexture(const std::wstring& path) const;
		/// Puts an already-created texture in the pool under `path` (used by the async loader).
		/// If another texture is already pooled under that path, that one is returned instead.
		std::shared_ptr<Texture> AdoptTexture(const std::wstring& path, std::shared_ptr<Texture> texture);

		/// The font `family` at cell height `height`, creating it if no one is using it right now.
		/// Throws what Font's constructor throws.
		std::shared_ptr<Font> GetFont(const std::wstring& family, INT height);

		/// Starts logging every path GetTexture is asked for (pool hits included), in first-use
		/// order. StopRecording returns the log. Used to generate a scene's asset manifest.
		void StartRecording();
		std::vector<std::wstring> StopRecording();

		/// Forgets pool entries whose asset is already gone (keeps the maps from growing).
		void Prune();
		/// How many pooled textures are still alive; for the debug overlay and tests.
		size_t LiveTextureCount() const;
	};

	/// A bundle of strong references that keeps a group of assets (e.g. everything one scene
	/// needs) alive together. Hold one per scene; dropping it lets the pool free whatever nothing
	/// else uses.
	class AssetSet final {
	private:
		std::vector<std::shared_ptr<Texture>> textures;
		std::vector<std::shared_ptr<Font>> fonts;
	public:
		void Add(std::shared_ptr<Texture> texture) { if (texture) textures.push_back(std::move(texture)); }
		void Add(std::shared_ptr<Font> font) { if (font) fonts.push_back(std::move(font)); }
		size_t Size() const { return textures.size() + fonts.size(); }
		void Clear() { textures.clear(); fonts.clear(); }
	};
}
