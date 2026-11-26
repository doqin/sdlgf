#include "pch.h"
#include "Texture.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdexcept>
#include <vector>
#include "Application.h"

sdlgf::Texture::Texture(GraphicsDevice* graphicsDevice) : graphicsDevice(graphicsDevice)
{
	if (graphicsDevice != nullptr) graphicsDevice->RegisterTexture(this);
}

sdlgf::Texture::~Texture()
{
	Release();
	if (graphicsDevice != nullptr) graphicsDevice->UnregisterTexture(this);
}

void sdlgf::Texture::Release()
{
	if (texture != nullptr) {
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
}

void sdlgf::Texture::OnDeviceDestroyed()
{
	// The renderer (and with it the SDL texture) is already gone.
	texture = nullptr;
	width = 0;
	height = 0;
	graphicsDevice = nullptr;
}

void sdlgf::Texture::Adopt(SDL_Texture* created, UINT w, UINT h)
{
	Release();
	texture = created;
	width = w;
	height = h;
	blending = true;
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	SDL_SetTextureBlendMode(texture, graphicsDevice->GetAlphaBlendMode());
}

void sdlgf::Texture::SetBlending(bool enabled)
{
	if (texture == nullptr || blending == enabled) return;
	blending = enabled;
	SDL_SetTextureBlendMode(texture, enabled ? graphicsDevice->GetAlphaBlendMode() : SDL_BLENDMODE_NONE);
}

void sdlgf::Texture::CreateFromPixels(const std::uint32_t* argbPixels, UINT w, UINT h)
{
	if (graphicsDevice == nullptr) throw std::runtime_error("Graphics device is null");
	SDL_Texture* created = SDL_CreateTexture(graphicsDevice->GetRenderer(), SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STATIC, static_cast<int>(w), static_cast<int>(h));
	if (created == nullptr) {
		throw std::runtime_error(std::string("Could not create texture: ") + SDL_GetError());
	}
	if (!SDL_UpdateTexture(created, nullptr, argbPixels, static_cast<int>(w * sizeof(std::uint32_t)))) {
		const std::string error = SDL_GetError();
		SDL_DestroyTexture(created);
		throw std::runtime_error("Could not upload texture: " + error);
	}
	Adopt(created, w, h);
}

void sdlgf::Texture::CreatePlainTexture(sdlgf::Color color, UINT w, UINT h)
{
	std::vector<std::uint32_t> pixels(static_cast<size_t>(w) * h, color);
	CreateFromPixels(pixels.data(), w, h);
}

void sdlgf::Texture::SetColor(sdlgf::Color color)
{
	if (texture == nullptr) {
		throw std::runtime_error("Texture is null");
	}
	std::vector<std::uint32_t> pixels(static_cast<size_t>(width) * height, color);
	if (!SDL_UpdateTexture(texture, nullptr, pixels.data(), static_cast<int>(width * sizeof(std::uint32_t)))) {
		throw std::runtime_error(std::string("Could not update texture: ") + SDL_GetError());
	}
}

void sdlgf::SurfaceDeleter::operator()(SDL_Surface* surface) const
{
	SDL_DestroySurface(surface);
}

sdlgf::SurfacePtr sdlgf::Texture::DecodeFile(const std::wstring& filePath)
{
	const std::string path = sdlgf::Platform::WideToUtf8(filePath);
	SDL_Surface* loaded = IMG_Load(path.c_str());
	if (loaded == nullptr) {
		// SDL's message alone ("Couldn't open file") doesn't say WHICH asset died - with dozens of
		// textures loading at startup that is not diagnosable, so name the file
		throw std::runtime_error("Could not load texture \"" + path + "\": " + SDL_GetError());
	}
	SDL_Surface* converted = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888);
	SDL_DestroySurface(loaded);
	if (converted == nullptr) {
		throw std::runtime_error("Could not convert texture \"" + path + "\": " + SDL_GetError());
	}
	return SurfacePtr(converted);
}

void sdlgf::Texture::LoadFromSurface(SDL_Surface* converted, const std::string& name)
{
	if (graphicsDevice == nullptr) {
		throw std::runtime_error("Graphics device is null");
	}
	SDL_Texture* created = SDL_CreateTextureFromSurface(graphicsDevice->GetRenderer(), converted);
	if (created == nullptr) {
		throw std::runtime_error("Could not create texture \"" + name + "\": " + SDL_GetError());
	}
	Adopt(created, static_cast<UINT>(converted->w), static_cast<UINT>(converted->h));
}

void sdlgf::Texture::LoadTexture(const std::wstring& filePath)
{
	if (graphicsDevice == nullptr) {
		throw std::runtime_error("Graphics device is null");
	}
	SurfacePtr decoded = DecodeFile(filePath);
	LoadFromSurface(decoded.get(), sdlgf::Platform::WideToUtf8(filePath));
}

void sdlgf::Texture::CaptureCurrentBackBuffer()
{
	if (graphicsDevice == nullptr) {
		throw std::runtime_error("Graphics device is null");
	}
	SDL_Surface* pixels = SDL_RenderReadPixels(graphicsDevice->GetRenderer(), nullptr);
	if (pixels == nullptr) {
		throw std::runtime_error(std::string("Could not read back the screen: ") + SDL_GetError());
	}
	SDL_Surface* converted = SDL_ConvertSurface(pixels, SDL_PIXELFORMAT_ARGB8888);
	SDL_DestroySurface(pixels);
	if (converted == nullptr) {
		throw std::runtime_error(std::string("Could not convert the screen capture: ") + SDL_GetError());
	}
	SDL_Texture* created = SDL_CreateTextureFromSurface(graphicsDevice->GetRenderer(), converted);
	const UINT w = static_cast<UINT>(converted->w), h = static_cast<UINT>(converted->h);
	SDL_DestroySurface(converted);
	if (created == nullptr) {
		throw std::runtime_error(std::string("Could not create texture from the screen: ") + SDL_GetError());
	}
	Adopt(created, w, h);
}

void sdlgf::Texture::CreateRenderTarget(UINT w, UINT h)
{
	if (graphicsDevice == nullptr) throw std::runtime_error("Graphics device is null");
	SDL_Texture* created = SDL_CreateTexture(graphicsDevice->GetRenderer(), SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET, static_cast<int>(w), static_cast<int>(h));
	if (created == nullptr) {
		throw std::runtime_error(std::string("Could not create render target: ") + SDL_GetError());
	}
	Adopt(created, w, h);
}

UINT sdlgf::Texture::GetWidth() const
{
	return width;
}

UINT sdlgf::Texture::GetHeight() const
{
	return height;
}

std::tuple<UINT, UINT> sdlgf::Texture::GetSize() const
{
	return std::tuple<UINT, UINT>(width, height);
}

sdlgf::GraphicsDevice* sdlgf::Texture::GetGraphicsDevice()
{
	return graphicsDevice;
}
