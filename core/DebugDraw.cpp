#include "pch.h"
#include "DebugDraw.h"
#include "Utils.h"
#include "AssetManager.h"
#include <algorithm>
#include <cmath>

std::shared_ptr<sdlgf::Font> sdlgf::DebugDraw::debugFont;
std::shared_ptr<sdlgf::FontSprite> sdlgf::DebugDraw::debugSprite;
std::map<int, std::wstring> sdlgf::DebugDraw::coordinateCache;

bool sdlgf::DebugDraw::drawGrid = false;
bool sdlgf::DebugDraw::drawAxis = false;

void sdlgf::DebugDraw::Init(GraphicsDevice* graphicsDevice, const std::wstring& fontName, int fontSize) {
	if (debugFont) return;

	debugFont = graphicsDevice->GetAssets().GetFont(fontName, fontSize);

	debugSprite = std::make_shared<FontSprite>(debugFont);

	for (int i = -2048; i <= 2048; i += 16) {
		coordinateCache[i] = std::to_wstring(i);
	}
}

void sdlgf::DebugDraw::Release() {
	debugSprite.reset();
	debugFont.reset();
	coordinateCache.clear();
}

void sdlgf::DebugDraw::DrawAxis(const Camera& camera, int range, int step, sdlgf::Color color) {

		
	if (!debugSprite) return;

	debugSprite->Begin();
	for (int i = -range + 24; i <= range; i += step) {
		if (coordinateCache.find(i) == coordinateCache.end()) {
			coordinateCache[i] = std::to_wstring(i);
		}

		debugSprite->SetPosition((float)i, 0.0f);
		debugSprite->SetText(coordinateCache[i].c_str());
		debugSprite->Draw(camera, color);

		if (i != 0) {
			debugSprite->SetPosition(0.0f, (float)-i);
			debugSprite->SetText(coordinateCache[i].c_str());
			debugSprite->Draw(camera, color);
		}
	}
	debugSprite->End();
}
//void sdlgf::DebugDraw::DrawAxis(
//	GraphicsDevice* graphicsDevice,
//	const Camera& camera,
//	Application* app,
//	int offsetX,
//	int offsetY,
//	int sreenWidth,
//	int screenHeight,
//	int spacingX,
//	int spacingY,
//	int range, int step,
//	sdlgf::Color color)
//{
//	if (spacingX <= 0 || spacingY <= 0) return;
//
//	auto matCamera = camera.GetTransformMatrix();
//	sdlgf::Mat4 matInv;
//	if (!sdlgf::Mat4::Inverse(&matInv, matCamera)) return;
//
//	auto TransformPoint = [&matInv](float x, float y) {
//		sdlgf::Vec4 v(x, y, 0.0f, 1.0f);
//		v = sdlgf::Transform(v, matInv);
//		if (v.w != 0.0f) {
//			v.x /= v.w;
//			v.y /= v.w;
//		}
//		return sdlgf::Vec2(v.x, v.y);
//		};
//
//	// Convert screen rect to a world-space AABB (works fine even with camera rotation; the AABB just becomes a conservative bound).
//	auto w0 = TransformPoint(0.0f, 0.0f);
//	auto w1 = TransformPoint((float)app->GetScreenWidth(), 0.0f);
//	auto w2 = TransformPoint(0.0f, (float)screenHeight);
//	auto w3 = TransformPoint((float)app->GetScreenWidth(), (float)screenHeight);
//
//	float minX = (std::min)((std::min)(w0.x, w1.x), (std::min)(w2.x, w3.x));
//	float maxX = (std::max)((std::max)(w0.x, w1.x), (std::max)(w2.x, w3.x));
//	float minY = (std::min)((std::min)(w0.y, w1.y), (std::min)(w2.y, w3.y));
//	float maxY = (std::max)((std::max)(w0.y, w1.y), (std::max)(w2.y, w3.y));
//
//	// Expand a bit so lines cover the full screen when the view is rotated.
//	minX -= spacingX;
//	maxX += spacingX;
//	minY -= spacingY;
//	maxY += spacingY;
//
//	float startX = std::floor((minX - offsetX) / (float)spacingX) * spacingX + offsetX;
//	float startY = std::floor((minY - offsetY) / (float)spacingY) * spacingY + offsetY;
//
//	if (!debugSprite) return;
//
//	debugSprite->Begin();
//
//	// X axis (y = 0)
//	for (float x = startX; x <= maxX; x += spacingX) {
//		int ix = (int)x;
//
//		auto& text = coordinateCache[ix];
//		if (text.empty()) {
//			text = std::to_wstring(ix);
//		}
//
//		debugSprite->SetPosition(x, 0.0f);
//		debugSprite->SetText(text.c_str());
//		debugSprite->Draw(camera, color);
//	}
//
//	// Y axis (x = 0)
//	for (float y = startY; y <= maxY; y += spacingY) {
//		int iy = (int)y;
//
//		if (iy == 0) continue;
//
//		auto& text = coordinateCache[iy];
//		if (text.empty()) {
//			text = std::to_wstring(iy);
//		}
//
//		debugSprite->SetPosition(0.0f, y);
//		debugSprite->SetText(text.c_str());
//		debugSprite->Draw(camera, color);
//	}
//
//	debugSprite->End();

	//if (!debugSprite) return;

	//debugSprite->Begin();
	//for (int i = -range + 24; i <= range; i += step) {
	//	if (coordinateCache.find(i) == coordinateCache.end()) {
	//		coordinateCache[i] = std::to_wstring(i);
	//	}

	//	debugSprite->SetPosition((float)i, 0.0f);
	//	debugSprite->SetText(coordinateCache[i].c_str());
	//	debugSprite->Draw(camera, color);

	//	if (i != 0) {
	//		debugSprite->SetPosition(0.0f, (float)-i);
	//		debugSprite->SetText(coordinateCache[i].c_str());
	//		debugSprite->Draw(camera, color);
	//	}
	//}
	//debugSprite->End();
//}

void sdlgf::DebugDraw::DrawGrid(GraphicsDevice* graphicsDevice, int offsetX, int offsetY, int screenWidth, int screenHeight, int spacingX, int spacingY,
	sdlgf::Color lineColor) {
		// Horizontal lines
		for (int i = offsetY % spacingY - spacingY; i < screenHeight; i += spacingY) {
			graphicsDevice->DrawLine(offsetX % spacingX - spacingX, i, screenWidth, i, lineColor);
		}
		// Vertical lines
		for (int i = offsetX % spacingX - spacingX; i < screenWidth; i += spacingX) {
			graphicsDevice->DrawLine(i, offsetY % spacingY - spacingY, i, screenHeight, lineColor);
		}
}

void sdlgf::DebugDraw::DrawGrid(GraphicsDevice* graphicsDevice, const Camera& camera, int offsetX, int offsetY, int screenWidth, int screenHeight, int spacingX, int spacingY, sdlgf::Color lineColor)
{
		if (spacingX <= 0 || spacingY <= 0) return;

		auto matCamera = camera.GetTransformMatrix();
		sdlgf::Mat4 matInv;
		if (!sdlgf::Mat4::Inverse(&matInv, matCamera)) return;

		auto TransformPoint = [&matInv](float x, float y) {
			sdlgf::Vec4 v(x, y, 0.0f, 1.0f);
			v = sdlgf::Transform(v, matInv);
			if (v.w != 0.0f) {
				v.x /= v.w;
				v.y /= v.w;
			}
			return sdlgf::Vec2(v.x, v.y);
			};

		// Convert screen rect to a world-space AABB (works fine even with camera rotation; the AABB just becomes a conservative bound).
		auto w0 = TransformPoint(0.0f, 0.0f);
		auto w1 = TransformPoint((float)screenWidth, 0.0f);
		auto w2 = TransformPoint(0.0f, (float)screenHeight);
		auto w3 = TransformPoint((float)screenWidth, (float)screenHeight);

		float minX = (std::min)((std::min)(w0.x, w1.x), (std::min)(w2.x, w3.x));
		float maxX = (std::max)((std::max)(w0.x, w1.x), (std::max)(w2.x, w3.x));
		float minY = (std::min)((std::min)(w0.y, w1.y), (std::min)(w2.y, w3.y));
		float maxY = (std::max)((std::max)(w0.y, w1.y), (std::max)(w2.y, w3.y));

		// Expand a bit so lines cover the full screen when the view is rotated.
		minX -= spacingX;
		maxX += spacingX;
		minY -= spacingY;
		maxY += spacingY;

		float startX = std::floor((minX - offsetX) / (float)spacingX) * spacingX + offsetX;
		float startY = std::floor((minY - offsetY) / (float)spacingY) * spacingY + offsetY;

		for (float y = startY; y <= maxY; y += spacingY) {
			graphicsDevice->DrawLine(camera, minX, y, maxX, y, lineColor);
		}
		for (float x = startX; x <= maxX; x += spacingX) {
			graphicsDevice->DrawLine(camera, x, minY, x, maxY, lineColor);
		}
}
