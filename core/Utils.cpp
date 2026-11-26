#include "pch.h"
#include "Utils.h"
#include "Application.h"
#include <SDL3/SDL_mouse.h>

std::vector<RECT> sdlgf::Utils::CreateFrames(UINT sheetWidth, UINT sheetHeight, UINT frameWidth, UINT frameHeight, UINT frameCount, UINT offset)
{
	std::vector<RECT> frames;
	frames.reserve(frameCount);

	UINT framesPerRow = sheetWidth / frameWidth;

	for (UINT i = 0; i < frameCount; ++i)
	{
		UINT frameIndex = offset + i;
		UINT row = frameIndex / framesPerRow;
		UINT col = frameIndex % framesPerRow;

		RECT rect;
		rect.left = col * frameWidth;
		rect.top = row * frameHeight;
		rect.right = rect.left + frameWidth;
		rect.bottom = rect.top + frameHeight;

		frames.push_back(rect);
	}

	return frames;
}

std::vector<RECT> sdlgf::Utils::CreateRectsVertical(long x, long y, long width, long height, long count)
{
	std::vector<RECT> rects(count);
	for (int i = 0; i < count; i++) {
		rects[i] = {
			.left = x,
			.top = y + i * height,
			.right = x + width,
			.bottom = y + (i + 1) * height
		};
	}
	return rects;
}

std::vector<RECT> sdlgf::Utils::CreateRectsHorizontal(long x, long y, long width, long height, long count)
{
	std::vector<RECT> rects(count);
	for (int i = 0; i < count; i++) {
		rects[i] = {
			.left = x + i * width,
			.top = y,
			.right = x + (i + 1) * width,
			.bottom = y + height
		};
	}
	return rects;
}

bool sdlgf::Utils::IsWithinRectangle(float pointX, float pointY, float rectX, float rectY, float rectWidth, float rectHeight)
{
	return pointX > rectX 
		&& pointX < rectX + rectWidth
		&& pointY > rectY
		&& pointY < rectY + rectHeight;
}

void sdlgf::Utils::SetMousePos(int x, int y)
{
	// x and y are client-area pixels; SDL warps in window coordinates
	auto app = sdlgf::Application::GetInstance();
	if (app->GetWindow() == nullptr) return;
	const float density = app->GetPixelDensity();
	SDL_WarpMouseInWindow(app->GetWindow(), static_cast<float>(x) / density, static_cast<float>(y) / density);
}

std::tuple<float, float> sdlgf::Utils::WindowToWorldCoords(const sdlgf::Camera& camera, float windowX, float windowY)
{
	D3DXMATRIX cameraTransform = camera.GetTransformMatrix();
	D3DXMATRIX invTransform;
	D3DXMatrixInverse(&invTransform, nullptr, &cameraTransform);

	D3DXVECTOR3 windowPos(windowX, windowY, 0.0f);
	D3DXVECTOR3 worldPos;
	D3DXVec3TransformCoord(&worldPos, &windowPos, &invTransform);
	return std::make_tuple(worldPos.x, worldPos.y);
}

std::tuple<float, float> sdlgf::Utils::WorldToWindowCoords(const sdlgf::Camera& camera, float worldX, float worldY)
{
	D3DXMATRIX cameraTransform = camera.GetTransformMatrix();
	D3DXVECTOR3 worldPos(worldX, worldY, 0.0f);
	D3DXVECTOR3 windowPos;
	D3DXVec3TransformCoord(&windowPos, &worldPos, &cameraTransform);
	return std::make_tuple(windowPos.x, windowPos.y);
}

std::wstring sdlgf::Utils::Utf8ToWide(const std::string& utf8)
{
	return sdlgf::Platform::Utf8ToWide(utf8);
}

std::string sdlgf::Utils::WideToUtf8(const std::wstring& wide)
{
	return sdlgf::Platform::WideToUtf8(wide);
}
