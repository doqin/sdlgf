#include "pch.h"
#include "GraphicsDevice.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include "Texture.h"
#include "AssetManager.h"

namespace {
	// The game was written against Direct3D 9, whose pixel CENTRES lie on integer coordinates;
	// SDL (like Direct3D 10+) puts them at +0.5. Shapes that were authored in D3D9 screen space
	// are shifted by half a pixel so that they cover the same pixels (and sample textures at the
	// same points) as before. Lines go through SDL_RenderLine(s), which applies the same
	// convention itself.
	constexpr float kD3D9PixelShift = 0.5f;

	SDL_FColor ToFColor(sdlgf::Color c)
	{
		return SDL_FColor{
			static_cast<float>((c >> 16) & 0xFF) * (1.0f / 255.0f),
			static_cast<float>((c >> 8) & 0xFF) * (1.0f / 255.0f),
			static_cast<float>(c & 0xFF) * (1.0f / 255.0f),
			static_cast<float>((c >> 24) & 0xFF) * (1.0f / 255.0f)
		};
	}

	sdlgf::Vec2 TransformPoint(const sdlgf::Mat4& matrix, float x, float y)
	{
		sdlgf::Vec4 v(x, y, 0.0f, 1.0f);
		v = sdlgf::Transform(v, matrix);
		if (v.w != 0.0f) {
			v.x /= v.w;
			v.y /= v.w;
		}
		return sdlgf::Vec2(v.x, v.y);
	}

	sdlgf::Vec2 TransformRectanglePoint(
		float x,
		float y,
		float localX,
		float localY,
		float rotation,
		float scaleX,
		float scaleY,
		float offsetX,
		float offsetY
	)
	{
		float px = (localX - offsetX) * scaleX;
		float py = (localY - offsetY) * scaleY;
		float c = std::cos(rotation);
		float s = std::sin(rotation);
		float rx = px * c - py * s;
		float ry = px * s + py * c;
		return sdlgf::Vec2(x + rx, y + ry);
	}

	constexpr int kEllipseSamples = 36;

}

sdlgf::GraphicsDevice::GraphicsDevice(SDL_Window* window, const char* rendererName) : window(window)
{
	// Thin lines: use the driver's own line rasteriser. SDL's default (Bresenham on rounded
	// endpoints) moves lines with fractional endpoints by up to a pixel; the hardware rule is the
	// one Direct3D 9 used, so lines keep the pixels they always had. (The software renderer has
	// its own line drawing and ignores this.)
	SDL_SetHintWithPriority(SDL_HINT_RENDER_LINE_METHOD, "2", SDL_HINT_DEFAULT);
	renderer = SDL_CreateRenderer(window, rendererName);
	if (renderer == nullptr) {
		throw std::runtime_error(std::string("Error creating renderer: ") + SDL_GetError());
	}
	// Texture coordinates outside [0,1] hold the edge pixel (SDL's default would repeat the
	// texture); callers pass such coordinates on purpose, e.g. for a cropped screen-space window.
	SDL_SetRenderTextureAddressMode(renderer, SDL_TEXTURE_ADDRESS_CLAMP, SDL_TEXTURE_ADDRESS_CLAMP);
	// Not every driver can vsync (software, dummy video); that is fine.
	SDL_SetRenderVSync(renderer, 1);
	alphaBlendMode = SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
	// The software renderer only knows the standard blend modes; its BLEND differs from the
	// custom mode only in the alpha channel written to render targets.
	if (!SDL_SetRenderDrawBlendMode(renderer, alphaBlendMode)) {
		alphaBlendMode = SDL_BLENDMODE_BLEND;
	}
	assets = std::make_unique<AssetManager>(this);
	sdlgf::Platform::DebugLog(std::string("Renderer: ") + SDL_GetRendererName(renderer) + "\n");
}

sdlgf::GraphicsDevice::~GraphicsDevice()
{
	// SDL_DestroyRenderer frees every texture created from it; let the Texture objects that
	// outlive the device know, so they neither free their texture again nor call back into us.
	for (Texture* texture : textures) texture->OnDeviceDestroyed();
	textures.clear();
	if (renderer != nullptr) SDL_DestroyRenderer(renderer);
}

void sdlgf::GraphicsDevice::RegisterTexture(Texture* texture)
{
	textures.push_back(texture);
}

void sdlgf::GraphicsDevice::UnregisterTexture(Texture* texture)
{
	auto it = std::find(textures.begin(), textures.end(), texture);
	if (it != textures.end()) {
		*it = textures.back();
		textures.pop_back();
	}
}

void sdlgf::GraphicsDevice::GetOutputSize(int& width, int& height) const
{
	width = 0;
	height = 0;
	SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
}

void sdlgf::GraphicsDevice::SetViewport(int x, int y, int width, int height)
{
	const SDL_Rect rect{ x, y, width, height };
	SDL_SetRenderViewport(renderer, &rect);
}

void sdlgf::GraphicsDevice::Clear(sdlgf::ColorF color)
{
	SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(renderer);
}

void sdlgf::GraphicsDevice::Clear()
{
	Clear(sdlgf::ColorF(0.0f, 0.0f, 0.0f, 1.0f));
}

void sdlgf::GraphicsDevice::Present()
{
	SDL_RenderPresent(renderer);
}

bool sdlgf::GraphicsDevice::SetRenderTarget(Texture* target)
{
	if (target == nullptr || target->GetSDLTexture() == nullptr) return false;
	if (!SDL_SetRenderTarget(renderer, target->GetSDLTexture())) return false;
	renderTarget = target;
	ApplyScissor();
	return true;
}

void sdlgf::GraphicsDevice::RestoreRenderTarget()
{
	SDL_SetRenderTarget(renderer, nullptr);
	renderTarget = nullptr;
	ApplyScissor();
}

void sdlgf::GraphicsDevice::ApplyBlend()
{
	SDL_SetRenderDrawBlendMode(renderer, alphaBlending ? alphaBlendMode : SDL_BLENDMODE_NONE);
}

void sdlgf::GraphicsDevice::SetAlphaBlending(bool enabled)
{
	alphaBlending = enabled;
}

void sdlgf::GraphicsDevice::ApplyScissor()
{
	if (scissorEnabled) {
		const SDL_Rect rect{ scissorRect.left, scissorRect.top, scissorRect.right - scissorRect.left, scissorRect.bottom - scissorRect.top };
		SDL_SetRenderClipRect(renderer, &rect);
	}
	else {
		SDL_SetRenderClipRect(renderer, nullptr);
	}
}

void sdlgf::GraphicsDevice::SetScissorTest(bool enabled)
{
	scissorEnabled = enabled;
	ApplyScissor();
}

void sdlgf::GraphicsDevice::SetScissorRect(const RECT& rect)
{
	scissorRect = rect;
	if (scissorEnabled) ApplyScissor();
}

void sdlgf::GraphicsDevice::SetVirtualTransform(float scale, float offsetX, float offsetY)
{
	virtualScale = scale;
	virtualOffsetX = offsetX;
	virtualOffsetY = offsetY;
}

void sdlgf::GraphicsDevice::ResetVirtualTransform()
{
	virtualScale = 1.0f;
	virtualOffsetX = 0.0f;
	virtualOffsetY = 0.0f;
}

// ---- shape helpers -------------------------------------------------------------------------------

void sdlgf::GraphicsDevice::FillFan(const std::vector<sdlgf::Vec2>& points, sdlgf::Color color)
{
	if (points.size() < 3) return;
	ApplyBlend();
	const SDL_FColor c = ToFColor(color);
	std::vector<SDL_Vertex> vertices(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		vertices[i].position = SDL_FPoint{ points[i].x + kD3D9PixelShift, points[i].y + kD3D9PixelShift };
		vertices[i].color = c;
		vertices[i].tex_coord = SDL_FPoint{ 0.0f, 0.0f };
	}
	std::vector<int> indices;
	indices.reserve((points.size() - 2) * 3);
	for (size_t i = 1; i + 1 < points.size(); ++i) {
		indices.push_back(0);
		indices.push_back(static_cast<int>(i));
		indices.push_back(static_cast<int>(i + 1));
	}
	SDL_RenderGeometry(renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size()));
}

void sdlgf::GraphicsDevice::StrokeStrip(const std::vector<sdlgf::Vec2>& points, sdlgf::Color color)
{
	if (points.size() < 2) return;
	ApplyBlend();
	const SDL_FColor c = ToFColor(color);
	SDL_SetRenderDrawColorFloat(renderer, c.r, c.g, c.b, c.a);
	std::vector<SDL_FPoint> pts(points.size());
	for (size_t i = 0; i < points.size(); ++i) pts[i] = SDL_FPoint{ points[i].x, points[i].y };
	SDL_RenderLines(renderer, pts.data(), static_cast<int>(pts.size()));
}

void sdlgf::GraphicsDevice::DrawLine(float x1, float y1, float x2, float y2, sdlgf::Color color, float thickness)
{
	DrawLineInternal(
		x1 * virtualScale + virtualOffsetX,
		y1 * virtualScale + virtualOffsetY,
		x2 * virtualScale + virtualOffsetX,
		y2 * virtualScale + virtualOffsetY,
		color,
		thickness > 1.0f ? thickness * virtualScale : thickness
	);
}

void sdlgf::GraphicsDevice::DrawLineInternal(float x1, float y1, float x2, float y2, sdlgf::Color color, float thickness)
{
	if (thickness <= 1.0f) {
		StrokeStrip({ sdlgf::Vec2(x1, y1), sdlgf::Vec2(x2, y2) }, color);
		return;
	}

	float dx = x2 - x1;
	float dy = y2 - y1;
	float len = std::sqrt(dx * dx + dy * dy);
	if (len < 0.00001f) return;

	float nx = -dy / len;
	float ny = dx / len;
	float halfThickness = thickness * 0.5f;

	// Triangle strip order of the original: (p1+n, p2+n, p1-n, p2-n)
	const sdlgf::Vec2 a(x1 + nx * halfThickness, y1 + ny * halfThickness);
	const sdlgf::Vec2 b(x2 + nx * halfThickness, y2 + ny * halfThickness);
	const sdlgf::Vec2 c(x1 - nx * halfThickness, y1 - ny * halfThickness);
	const sdlgf::Vec2 d(x2 - nx * halfThickness, y2 - ny * halfThickness);
	ApplyBlend();
	const SDL_FColor col = ToFColor(color);
	const SDL_Vertex v[4] = {
		{ SDL_FPoint{ a.x + kD3D9PixelShift, a.y + kD3D9PixelShift }, col, SDL_FPoint{ 0, 0 } },
		{ SDL_FPoint{ b.x + kD3D9PixelShift, b.y + kD3D9PixelShift }, col, SDL_FPoint{ 0, 0 } },
		{ SDL_FPoint{ c.x + kD3D9PixelShift, c.y + kD3D9PixelShift }, col, SDL_FPoint{ 0, 0 } },
		{ SDL_FPoint{ d.x + kD3D9PixelShift, d.y + kD3D9PixelShift }, col, SDL_FPoint{ 0, 0 } },
	};
	const int idx[6] = { 0, 1, 2, 2, 1, 3 };
	SDL_RenderGeometry(renderer, nullptr, v, 4, idx, 6);
}

void sdlgf::GraphicsDevice::DrawLine(const sdlgf::Camera& camera, float x1, float y1, float x2, float y2, sdlgf::Color color, float thickness)
{
	auto matCamera = camera.GetTransformMatrix();
	auto p1 = TransformPoint(matCamera, x1, y1);
	auto p2 = TransformPoint(matCamera, x2, y2);
	DrawLineInternal(p1.x, p1.y, p2.x, p2.y, color, thickness);
}

void sdlgf::GraphicsDevice::DrawRectangle(float x, float y, float width, float height, sdlgf::Color color, bool isFilled)
{
	DrawRectangle(x, y, width, height, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, color, isFilled);
}

void sdlgf::GraphicsDevice::DrawRectangle(const sdlgf::Camera& camera, float x, float y, float width, float height, sdlgf::Color color, bool isFilled)
{
	DrawRectangle(camera, x, y, width, height, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, color, isFilled);
}

void sdlgf::GraphicsDevice::DrawRectangle(float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled)
{
	std::vector<sdlgf::Vec2> points = {
		TransformRectanglePoint(x, y, 0.0f, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY),
		TransformRectanglePoint(x, y, width, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY),
		TransformRectanglePoint(x, y, width, height, rotation, scaleX, scaleY, offsetX, offsetY),
		TransformRectanglePoint(x, y, 0.0f, height, rotation, scaleX, scaleY, offsetX, offsetY),
	};
	for (auto& p : points) {
		p.x = p.x * virtualScale + virtualOffsetX;
		p.y = p.y * virtualScale + virtualOffsetY;
	}
	if (isFilled) {
		FillFan(points, color);
	}
	else {
		points.push_back(points[0]);
		StrokeStrip(points, color);
	}
}

void sdlgf::GraphicsDevice::DrawRectangle(const sdlgf::Camera& camera, float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled)
{
	auto matCamera = camera.GetTransformMatrix();

	auto wp0 = TransformRectanglePoint(x, y, 0.0f, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY);
	auto wp1 = TransformRectanglePoint(x, y, width, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY);
	auto wp2 = TransformRectanglePoint(x, y, width, height, rotation, scaleX, scaleY, offsetX, offsetY);
	auto wp3 = TransformRectanglePoint(x, y, 0.0f, height, rotation, scaleX, scaleY, offsetX, offsetY);

	std::vector<sdlgf::Vec2> points = {
		TransformPoint(matCamera, wp0.x, wp0.y),
		TransformPoint(matCamera, wp1.x, wp1.y),
		TransformPoint(matCamera, wp2.x, wp2.y),
		TransformPoint(matCamera, wp3.x, wp3.y),
	};
	if (isFilled) {
		FillFan(points, color);
	}
	else {
		points.push_back(points[0]);
		StrokeStrip(points, color);
	}
}

void sdlgf::GraphicsDevice::DrawEllipse(float centerX, float centerY, float width, float height, sdlgf::Color color, bool isFilled)
{
	DrawEllipse(centerX, centerY, width, height, 0.0f, 1.0f, 1.0f, width * 0.5f, height * 0.5f, color, isFilled);
}

void sdlgf::GraphicsDevice::DrawEllipse(const sdlgf::Camera& camera, float centerX, float centerY, float width, float height, sdlgf::Color color, bool isFilled)
{
	DrawEllipse(camera, centerX, centerY, width, height, 0.0f, 1.0f, 1.0f, width * 0.5f, height * 0.5f, color, isFilled);
}

void sdlgf::GraphicsDevice::DrawEllipse(float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled)
{
	std::vector<sdlgf::Vec2> points;
	points.reserve(kEllipseSamples + 2);

	float rx = width * 0.5f;
	float ry = height * 0.5f;
	float cx = rx;
	float cy = ry;

	if (isFilled) {
		auto c = TransformRectanglePoint(x, y, cx, cy, rotation, scaleX, scaleY, offsetX, offsetY);
		points.emplace_back(c.x * virtualScale + virtualOffsetX, c.y * virtualScale + virtualOffsetY);
	}
	for (int i = 0; i <= kEllipseSamples; ++i) {
		float theta = i * (2.0f * sdlgf::Pi / kEllipseSamples);
		float lx = cx + std::cos(theta) * rx;
		float ly = cy + std::sin(theta) * ry;
		auto p = TransformRectanglePoint(x, y, lx, ly, rotation, scaleX, scaleY, offsetX, offsetY);
		points.emplace_back(p.x * virtualScale + virtualOffsetX, p.y * virtualScale + virtualOffsetY);
	}
	if (isFilled) FillFan(points, color);
	else StrokeStrip(points, color);
}

void sdlgf::GraphicsDevice::DrawEllipse(const sdlgf::Camera& camera, float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled)
{
	auto matCamera = camera.GetTransformMatrix();

	std::vector<sdlgf::Vec2> points;
	points.reserve(kEllipseSamples + 2);

	float rx = width * 0.5f;
	float ry = height * 0.5f;
	float cx = rx;
	float cy = ry;

	if (isFilled) {
		auto wc = TransformRectanglePoint(x, y, cx, cy, rotation, scaleX, scaleY, offsetX, offsetY);
		points.push_back(TransformPoint(matCamera, wc.x, wc.y));
	}
	for (int i = 0; i <= kEllipseSamples; ++i) {
		float theta = i * (2.0f * sdlgf::Pi / kEllipseSamples);
		float lx = cx + std::cos(theta) * rx;
		float ly = cy + std::sin(theta) * ry;
		auto wp = TransformRectanglePoint(x, y, lx, ly, rotation, scaleX, scaleY, offsetX, offsetY);
		points.push_back(TransformPoint(matCamera, wp.x, wp.y));
	}
	if (isFilled) FillFan(points, color);
	else StrokeStrip(points, color);
}

void sdlgf::GraphicsDevice::DrawTriangle(float centerX, float topY, float width, float height, sdlgf::Color color, bool isFilled)
{
	std::vector<sdlgf::Vec2> points = {
		{ (centerX - width * 0.5f) * virtualScale + virtualOffsetX, topY * virtualScale + virtualOffsetY },
		{ (centerX + width * 0.5f) * virtualScale + virtualOffsetX, topY * virtualScale + virtualOffsetY },
		{ centerX * virtualScale + virtualOffsetX, (topY + height) * virtualScale + virtualOffsetY },
	};
	if (isFilled) {
		FillFan(points, color);
	}
	else {
		points.push_back(points[0]);
		StrokeStrip(points, color);
	}
}

void sdlgf::GraphicsDevice::DrawTriangle(const sdlgf::Camera& camera, float centerX, float topY, float width, float height, sdlgf::Color color, bool isFilled)
{
	auto matCamera = camera.GetTransformMatrix();

	std::vector<sdlgf::Vec2> points = {
		TransformPoint(matCamera, centerX - width * 0.5f, topY),
		TransformPoint(matCamera, centerX + width * 0.5f, topY),
		TransformPoint(matCamera, centerX, topY + height),
	};
	if (isFilled) {
		FillFan(points, color);
	}
	else {
		points.push_back(points[0]);
		StrokeStrip(points, color);
	}
}

void sdlgf::GraphicsDevice::DrawTriangleScreenSpace(float x1, float y1, float x2, float y2, float x3, float y3, sdlgf::Color color)
{
	FillFan({ sdlgf::Vec2(x1, y1), sdlgf::Vec2(x2, y2), sdlgf::Vec2(x3, y3) }, color);
}

// ---- textured drawing ------------------------------------------------------------------------------

void sdlgf::GraphicsDevice::DrawTexturedPolygon(const std::vector<sdlgf::Vec2>& points, const std::vector<sdlgf::Vec2>& uvs, sdlgf::Texture* texture, sdlgf::Color color)
{
	if (points.size() < 3 || points.size() != uvs.size() || texture == nullptr || texture->GetSDLTexture() == nullptr) return;

	const SDL_FColor c = ToFColor(color);
	std::vector<SDL_Vertex> vertices(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		vertices[i].position = SDL_FPoint{
			points[i].x * virtualScale + virtualOffsetX + kD3D9PixelShift,
			points[i].y * virtualScale + virtualOffsetY + kD3D9PixelShift
		};
		vertices[i].color = c;
		vertices[i].tex_coord = SDL_FPoint{ uvs[i].x, uvs[i].y };
	}
	std::vector<int> indices;
	indices.reserve((points.size() - 2) * 3);
	for (size_t i = 1; i + 1 < points.size(); ++i) {
		indices.push_back(0);
		indices.push_back(static_cast<int>(i));
		indices.push_back(static_cast<int>(i + 1));
	}

	// UVs outside [0,1] are clamped to the edge pixel (SDL's default texture addressing), which is
	// what callers rely on for screen-space-projected crop windows. Unlike sprites, this honours
	// SetAlphaBlending.
	texture->SetBlending(alphaBlending);
	SDL_RenderGeometry(renderer, texture->GetSDLTexture(), vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size()));
	texture->SetBlending(true);
}

void sdlgf::GraphicsDevice::DrawTexturedQuad(Texture* texture, const sdlgf::Mat4& transform, float x, float y, float width, float height,
	float u0, float v0, float u1, float v1, sdlgf::Color color)
{
	if (texture == nullptr || texture->GetSDLTexture() == nullptr) return;

	const sdlgf::Vec2 p0 = TransformPoint(transform, x, y);
	const sdlgf::Vec2 p1 = TransformPoint(transform, x + width, y);
	const sdlgf::Vec2 p2 = TransformPoint(transform, x + width, y + height);
	const sdlgf::Vec2 p3 = TransformPoint(transform, x, y + height);
	const SDL_FColor c = ToFColor(color);
	const SDL_Vertex v[4] = {
		{ SDL_FPoint{ p0.x, p0.y }, c, SDL_FPoint{ u0, v0 } },
		{ SDL_FPoint{ p1.x, p1.y }, c, SDL_FPoint{ u1, v0 } },
		{ SDL_FPoint{ p2.x, p2.y }, c, SDL_FPoint{ u1, v1 } },
		{ SDL_FPoint{ p3.x, p3.y }, c, SDL_FPoint{ u0, v1 } },
	};
	const int idx[6] = { 0, 1, 2, 0, 2, 3 };
	texture->SetBlending(true);
	SDL_RenderGeometry(renderer, texture->GetSDLTexture(), v, 4, idx, 6);
}

void sdlgf::GraphicsDevice::DrawTexturedTriangles(Texture* texture, const Vertex* vertices, int vertexCount)
{
	if (texture == nullptr || texture->GetSDLTexture() == nullptr || vertexCount < 3) return;

	scratchVertices.resize(static_cast<size_t>(vertexCount));
	for (int i = 0; i < vertexCount; ++i) {
		const Vertex& in = vertices[i];
		scratchVertices[i] = SDL_Vertex{ SDL_FPoint{ in.x, in.y }, ToFColor(in.color), SDL_FPoint{ in.u, in.v } };
	}
	texture->SetBlending(true);
	SDL_RenderGeometry(renderer, texture->GetSDLTexture(), scratchVertices.data(), vertexCount, nullptr, 0);
}
