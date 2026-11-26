#pragma once
#include "GraphicsDevice.h"
#include "Font.h"
#include "Sprites.h"
#include <map>
#include <string>
#include <vector>

namespace sdlgf {
	class DebugDraw final {
	private:
		DebugDraw() {}
		~DebugDraw() {}

		static Font* debugFont;
		static FontSprite* debugSprite;
		static std::map<int, std::wstring> coordinateCache;
	public:

		/// `fontName` must be a family name registered with Font::AddFont.
		static void Init(GraphicsDevice* graphicsDevice, const std::wstring& fontName, int fontSize = 14);
		static void Release();

		static void DrawGrid(GraphicsDevice* graphicsDevice, int offsetX, int offsetY, int screenWidth, int screenHeight, int spacingX, int spacingY, sdlgf::Color lineColor);
		static void DrawGrid(GraphicsDevice* graphicsDevice, const Camera& camera, int offsetX, int offsetY, int screenWidth, int screenHeight, int spacingX, int spacingY, sdlgf::Color lineColor);

		//static void DrawAxis(GraphicsDevice* graphicsDevice, const Camera& camera, Application* app, int offsetX, int offsetY, int sreenWidth, int screenHeight, int spacingX, int spacingY,int range, int step, sdlgf::Color color = 0xFFFFFFFF);
		static void DrawAxis(const Camera& camera, int range, int step, sdlgf::Color color = 0xFFFFFFFF);

		static bool drawGrid;
		static bool drawAxis;
	};
}