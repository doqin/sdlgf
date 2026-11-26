#pragma once
#include <cstdint>
#include <string>
#include <tuple>
#include "GraphicsDevice.h"

struct SDL_Texture;

namespace sdlgf {
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
