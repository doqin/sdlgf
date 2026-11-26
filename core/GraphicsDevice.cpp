#include "pch.h"
#include "GraphicsDevice.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <d3dx9math.h>
#include "Application.h"
#include "Utils.h"
#include "Texture.h"

struct Vertex {
	float x, y, z, rhw; // rhw is reciprocal of homogenous w
	DWORD color;
};

struct TexVertex {
	float x, y, z, rhw; // rhw is reciprocal of homogenous w
	DWORD color;
	float u, v;
};
#define D3DFVF_TEXVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static sdlgf::Vec2 TransformPoint(const sdlgf::Mat4& matrix, float x, float y)
{
	sdlgf::Vec4 v(x, y, 0.0f, 1.0f);
	v = sdlgf::Transform(v, matrix);
	if (v.w != 0.0f) {
		v.x /= v.w;
		v.y /= v.w;
	}
	return sdlgf::Vec2(v.x, v.y);
}

static sdlgf::Vec2 TransformRectanglePoint(
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

sdlgf::GraphicsDevice::GraphicsDevice(IDirect3DDevice9* d3ddev, IDirect3DSurface9* backbuffer) : d3ddev(d3ddev), backbuffer(backbuffer)
{
	d3ddev->GetViewport(&viewport);
}

sdlgf::GraphicsDevice::~GraphicsDevice() {
	if (d3ddev != NULL) d3ddev->Release();
}

IDirect3DDevice9*& sdlgf::GraphicsDevice::GetDevice() {
	return d3ddev;
}

LPDIRECT3DSURFACE9& sdlgf::GraphicsDevice::GetBackBuffer() {
	return backbuffer;
}

HRESULT sdlgf::GraphicsDevice::SetViewport(D3DVIEWPORT9 viewport)
{
	return d3ddev->SetViewport(&viewport);
}

HRESULT sdlgf::GraphicsDevice::SetViewport(DWORD x, DWORD y, DWORD width, DWORD height, float minZ, float maxZ)
{
	D3DVIEWPORT9 vp = { x, y, width, height, minZ, maxZ };
	return d3ddev->SetViewport(&vp);
}

HRESULT sdlgf::GraphicsDevice::BeginDraw()
{
	return d3ddev->BeginScene();
}

HRESULT sdlgf::GraphicsDevice::Clear(sdlgf::ColorF color)
{
	return d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0);
}

HRESULT sdlgf::GraphicsDevice::Clear()
{
	return d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, COLOR_XRGB(0, 0, 0), 1.0f, 0);
}

void sdlgf::GraphicsDevice::RegisterVolatileResource(IDeviceLostAware* resource)
{
	volatileResources.push_back(resource);
}

void sdlgf::GraphicsDevice::UnregisterVolatileResource(IDeviceLostAware* resource)
{
	auto it = std::find(volatileResources.begin(), volatileResources.end(), resource);
	if (it != volatileResources.end()) volatileResources.erase(it);
}

void sdlgf::GraphicsDevice::NotifyLostDevice()
{
	for (auto* resource : volatileResources) resource->OnLostDevice();
}

void sdlgf::GraphicsDevice::NotifyResetDevice()
{
	for (auto* resource : volatileResources) resource->OnResetDevice();
}

HRESULT sdlgf::GraphicsDevice::EndDraw()
{
	return d3ddev->EndScene();
}

HRESULT sdlgf::GraphicsDevice::Present()
{
	return d3ddev->Present(NULL, NULL, NULL, NULL);
}

HRESULT sdlgf::GraphicsDevice::IsValid()
{
	if (d3ddev == NULL)
		return E_FAIL;

	return d3ddev->TestCooperativeLevel();
}

void sdlgf::GraphicsDevice::SetAlphaBlending(bool enabled)
{
	if (enabled) {
		d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else {
		d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}
}

void sdlgf::GraphicsDevice::SetScissorTest(bool enabled)
{
	d3ddev->SetRenderState(D3DRS_SCISSORTESTENABLE, enabled);
}

void sdlgf::GraphicsDevice::SetScissorRect(const RECT& rect)
{
	d3ddev->SetScissorRect(&rect);
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
		Vertex vertices[] = {
			{ .x=x1, .y=y1, .z=0.0f, .rhw=1.0f, .color=color },
			{ .x=x2, .y=y2, .z=0.0f, .rhw=1.0f, .color=color }
		};
		d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		d3ddev->DrawPrimitiveUP(D3DPT_LINELIST, 1, vertices, sizeof(Vertex));
	}
	else {
		float dx = x2 - x1;
		float dy = y2 - y1;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len < 0.00001f) return;

		float nx = -dy / len;
		float ny = dx / len;

		float halfThickness = thickness * 0.5f;

		Vertex vertices[] = {
			{ .x = x1 + nx * halfThickness, .y = y1 + ny * halfThickness, .z = 0.0f, .rhw = 1.0f, .color = color },
			{ .x = x2 + nx * halfThickness, .y = y2 + ny * halfThickness, .z = 0.0f, .rhw = 1.0f, .color = color },
			{ .x = x1 - nx * halfThickness, .y = y1 - ny * halfThickness, .z = 0.0f, .rhw = 1.0f, .color = color },
			{ .x = x2 - nx * halfThickness, .y = y2 - ny * halfThickness, .z = 0.0f, .rhw = 1.0f, .color = color }
		};

		DWORD prevCullMode;
		d3ddev->GetRenderState(D3DRS_CULLMODE, &prevCullMode);
		d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));

		d3ddev->SetRenderState(D3DRS_CULLMODE, prevCullMode);
	}
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
	auto p0 = TransformRectanglePoint(x, y, 0.0f, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY);
	auto p1 = TransformRectanglePoint(x, y, width, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY);
	auto p2 = TransformRectanglePoint(x, y, width, height, rotation, scaleX, scaleY, offsetX, offsetY);
	auto p3 = TransformRectanglePoint(x, y, 0.0f, height, rotation, scaleX, scaleY, offsetX, offsetY);

	for (auto* p : { &p0, &p1, &p2, &p3 }) {
		p->x = p->x * virtualScale + virtualOffsetX;
		p->y = p->y * virtualScale + virtualOffsetY;
	}

	std::vector<Vertex> vertices = {
		{.x = p0.x, .y = p0.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p1.x, .y = p1.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p2.x, .y = p2.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p3.x, .y = p3.y, .z = 0.0f, .rhw = 1.0f, .color = color }
	};

	if (!isFilled) {
		vertices.push_back(vertices[0]);
	}

	d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DPRIMITIVETYPE primitiveType = isFilled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP;
	d3ddev->DrawPrimitiveUP(primitiveType, isFilled ? 2 : 4, vertices.data(), sizeof(Vertex));
}

void sdlgf::GraphicsDevice::DrawRectangle(const sdlgf::Camera& camera, float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled)
{
	auto matCamera = camera.GetTransformMatrix();

	auto wp0 = TransformRectanglePoint(x, y, 0.0f, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY);
	auto wp1 = TransformRectanglePoint(x, y, width, 0.0f, rotation, scaleX, scaleY, offsetX, offsetY);
	auto wp2 = TransformRectanglePoint(x, y, width, height, rotation, scaleX, scaleY, offsetX, offsetY);
	auto wp3 = TransformRectanglePoint(x, y, 0.0f, height, rotation, scaleX, scaleY, offsetX, offsetY);

	auto p0 = TransformPoint(matCamera, wp0.x, wp0.y);
	auto p1 = TransformPoint(matCamera, wp1.x, wp1.y);
	auto p2 = TransformPoint(matCamera, wp2.x, wp2.y);
	auto p3 = TransformPoint(matCamera, wp3.x, wp3.y);

	std::vector<Vertex> vertices = {
		{.x = p0.x, .y = p0.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p1.x, .y = p1.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p2.x, .y = p2.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p3.x, .y = p3.y, .z = 0.0f, .rhw = 1.0f, .color = color }
	};

	if (!isFilled) {
		vertices.push_back(vertices[0]);
	}
	D3DPRIMITIVETYPE primitiveType = isFilled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP;
	d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	d3ddev->DrawPrimitiveUP(primitiveType, isFilled ? 2 : 4, vertices.data(), sizeof(Vertex));
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
	const int SAMPLES = 36;
	std::vector<Vertex> vertices;

	float rx = width * 0.5f;
	float ry = height * 0.5f;
	float cx = rx;
	float cy = ry;

	if (isFilled) {
		auto c = TransformRectanglePoint(x, y, cx, cy, rotation, scaleX, scaleY, offsetX, offsetY);
		vertices.push_back({ .x = c.x * virtualScale + virtualOffsetX, .y = c.y * virtualScale + virtualOffsetY, .z = 0.0f, .rhw = 1.0f, .color = color });
	}
	for (int i = 0; i <= SAMPLES; ++i) {
		float theta = i * (2.0f * sdlgf::Pi / SAMPLES);
		float lx = cx + std::cos(theta) * rx;
		float ly = cy + std::sin(theta) * ry;
		auto p = TransformRectanglePoint(x, y, lx, ly, rotation, scaleX, scaleY, offsetX, offsetY);
		vertices.push_back({
			.x = p.x * virtualScale + virtualOffsetX,
			.y = p.y * virtualScale + virtualOffsetY,
			.z = 0.0f,
			.rhw = 1.0f,
			.color = color
		});
	}
	d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DPRIMITIVETYPE primitiveType = isFilled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP;
	d3ddev->DrawPrimitiveUP(primitiveType, SAMPLES, vertices.data(), sizeof(Vertex));
}

void sdlgf::GraphicsDevice::DrawEllipse(const sdlgf::Camera& camera, float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled)
{
	const int SAMPLES = 36;
	auto matCamera = camera.GetTransformMatrix();

	std::vector<Vertex> vertices;

	float rx = width * 0.5f;
	float ry = height * 0.5f;
	float cx = rx;
	float cy = ry;

	if (isFilled) {
		auto wc = TransformRectanglePoint(x, y, cx, cy, rotation, scaleX, scaleY, offsetX, offsetY);
		auto c = TransformPoint(matCamera, wc.x, wc.y);
		vertices.push_back({ .x = c.x, .y = c.y, .z = 0.0f, .rhw = 1.0f, .color = color });
	}
	for (int i = 0; i <= SAMPLES; ++i) {
		float theta = i * (2.0f * sdlgf::Pi / SAMPLES);
		float lx = cx + std::cos(theta) * rx;
		float ly = cy + std::sin(theta) * ry;
		auto wp = TransformRectanglePoint(x, y, lx, ly, rotation, scaleX, scaleY, offsetX, offsetY);
		auto p = TransformPoint(matCamera, wp.x, wp.y);
		vertices.push_back({
			.x = p.x,
			.y = p.y,
			.z = 0.0f,
			.rhw = 1.0f,
			.color = color
		});
	}

	d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DPRIMITIVETYPE primitiveType = isFilled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP;
	d3ddev->DrawPrimitiveUP(primitiveType, SAMPLES, vertices.data(), sizeof(Vertex));
}

void sdlgf::GraphicsDevice::DrawTriangle(float centerX, float topY, float width, float height, sdlgf::Color color, bool isFilled)
{
	std::vector<Vertex> vertices = {
		{.x = (centerX - width * 0.5f) * virtualScale + virtualOffsetX, .y = topY * virtualScale + virtualOffsetY, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = (centerX + width * 0.5f) * virtualScale + virtualOffsetX, .y = topY * virtualScale + virtualOffsetY, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = centerX * virtualScale + virtualOffsetX, .y = (topY + height) * virtualScale + virtualOffsetY, .z = 0.0f, .rhw = 1.0f, .color = color }
	};

	if (!isFilled) {
		vertices.push_back(vertices[0]);
	}

	d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DPRIMITIVETYPE primitiveType = isFilled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP;
	d3ddev->DrawPrimitiveUP(primitiveType, isFilled ? 1 : 3, vertices.data(), sizeof(Vertex));
}

void sdlgf::GraphicsDevice::DrawTriangle(const sdlgf::Camera& camera, float centerX, float topY, float width, float height, sdlgf::Color color, bool isFilled)
{
	auto matCamera = camera.GetTransformMatrix();

	auto p0 = TransformPoint(matCamera, centerX - width * 0.5f, topY);
	auto p1 = TransformPoint(matCamera, centerX + width * 0.5f, topY);
	auto p2 = TransformPoint(matCamera, centerX, topY + height);

	std::vector<Vertex> vertices = {
		{.x = p0.x, .y = p0.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p1.x, .y = p1.y, .z = 0.0f, .rhw = 1.0f, .color = color },
		{.x = p2.x, .y = p2.y, .z = 0.0f, .rhw = 1.0f, .color = color }
	};

	if (!isFilled) {
		vertices.push_back(vertices[0]);
	}

	d3ddev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DPRIMITIVETYPE primitiveType = isFilled ? D3DPT_TRIANGLEFAN : D3DPT_LINESTRIP;
	d3ddev->DrawPrimitiveUP(primitiveType, isFilled ? 1 : 3, vertices.data(), sizeof(Vertex));
}
void sdlgf::GraphicsDevice::DrawTexturedPolygon(const std::vector<sdlgf::Vec2>& points, const std::vector<sdlgf::Vec2>& uvs, sdlgf::Texture* texture, sdlgf::Color color)
{
	if (points.size() < 3 || points.size() != uvs.size() || texture == nullptr) return;

	std::vector<TexVertex> vertices;
	vertices.reserve(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		vertices.push_back({
			.x = points[i].x * virtualScale + virtualOffsetX,
			.y = points[i].y * virtualScale + virtualOffsetY,
			.z = 0.0f,
			.rhw = 1.0f,
			.color = color,
			.u = uvs[i].x,
			.v = uvs[i].y
			});
	}

	// Clamp rather than wrap/tile: callers may pass UVs outside [0,1] on purpose (e.g. a
	// screen-space-projected crop window), and clamping just holds the edge pixel there
	// instead of repeating the texture.
	d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	d3ddev->SetTexture(0, texture->GetRawTexture());
	d3ddev->SetFVF(D3DFVF_TEXVERTEX);
	d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, static_cast<UINT>(points.size() - 2), vertices.data(), sizeof(TexVertex));
	d3ddev->SetTexture(0, nullptr);
	d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	d3ddev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
}

HRESULT sdlgf::GraphicsDevice::SetRenderTarget(Texture* renderTarget)
{
	if (renderTarget == nullptr) return E_POINTER;
	IDirect3DSurface9* surface = renderTarget->GetSurface();
	HRESULT hr = d3ddev->SetRenderTarget(0, surface);

	// COM object automatically increments its reference count when calling GetSurfaceLevel; must Release after use
	if (surface != nullptr) surface->Release();
	return hr;
}

HRESULT sdlgf::GraphicsDevice::RestoreRenderTarget()
{
	// Restore the render target back to the actual screen
	return d3ddev->SetRenderTarget(0, backbuffer);
}