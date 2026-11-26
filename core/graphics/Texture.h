#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include "GraphicsDevice.h"

struct SDL_Texture;
struct SDL_Surface;

namespace sdlgf {
	struct SurfaceDeleter { void operator()(SDL_Surface* surface) const; };
	/// A CPU-side image (pixels in RAM, no GPU involved), always 0xAARRGGBB.
	using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

	/// An image in video memory: loaded from a PNG/BMP/JPG file, filled with a colour, built from
	/// pixels, or a render target the GraphicsDevice can draw into. Filtering is always
	/// nearest-neighbour (the game is pixel art).
	///
	/// A Texture belongs to the GraphicsDevice it was made for. If the device is destroyed first the
	/// texture simply becomes empty (IsLoaded() is false).
	class Texture {
	private:
		GraphicsDevice* graphicsDevice;
		SDL_Texture* texture = nullptr;
		UINT width = 0, height = 0;
		bool blending = true;
		void Release();
		void Adopt(SDL_Texture* created, UINT w, UINT h);
		friend class GraphicsDevice;
		void OnDeviceDestroyed();
	public:
		Texture(GraphicsDevice* graphicsDevice);
		~Texture();
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		void CreatePlainTexture(sdlgf::Color color, UINT width, UINT height);
		/// Fills the whole texture with one colour.
		void SetColor(sdlgf::Color color);
		/// Loads an image file (paths are relative to the working directory, which the
		/// Application sets to the executable's directory). Throws std::runtime_error naming
		/// the file if it cannot be loaded.
		void LoadTexture(const std::wstring& filePath);
		/// Reads and decodes an image file into RAM without touching the GPU, so it is safe to
		/// call from a worker thread (see AssetLoader). Throws std::runtime_error naming the file.
		static SurfacePtr DecodeFile(const std::wstring& filePath);
		/// Uploads a decoded image (from DecodeFile) to the GPU. Main thread only.
		/// `name` is only used in error messages.
		void LoadFromSurface(SDL_Surface* argbSurface, const std::string& name);
		/// Creates the texture from `width` x `height` pixels of 0xAARRGGBB, row-major.
		void CreateFromPixels(const std::uint32_t* argbPixels, UINT width, UINT height);
		/// Copies what the window currently shows into this texture.
		void CaptureCurrentBackBuffer();
		/// A texture the GraphicsDevice can be redirected into with SetRenderTarget.
		void CreateRenderTarget(UINT width, UINT height);

		SDL_Texture* GetSDLTexture() const { return texture; }
		bool IsLoaded() const { return texture != nullptr; }
		UINT GetWidth() const;
		UINT GetHeight() const;
		std::tuple<UINT, UINT> GetSize() const;
		GraphicsDevice* GetGraphicsDevice();
		void ReleaseTexture() { Release(); width = 0; height = 0; }

		/// Whether drawing this texture blends (see GraphicsDevice::GetAlphaBlendMode) or
		/// overwrites. Sprites always blend; GraphicsDevice flips this around textured polygons.
		void SetBlending(bool enabled);
	};
}
