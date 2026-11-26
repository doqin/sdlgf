#pragma once
#include "IButton.h"
#include "Font.h"
#include <string>
#include <vector>
namespace Demo
{
	class TextButton : public IButton
	{
	private:
		std::string text;
		std::shared_ptr<sdlgf::FontSprite> fontSprite;

		//save state's color + background
		std::vector<DWORD> textColors;
		std::vector<sdlgf::ColorF> bgColors;
		
		//fix memory allocation
		std::wstring wText;

		// outline
		bool hasOutline;
		float outlineThickness;
		std::vector<sdlgf::ColorF> outlineColors;

		// padding
		float paddingX;
		float paddingY;
	public:
		TextButton(std::shared_ptr<sdlgf::TransformManager> tm,
			float x, float y, float w, float h,
			std::string txtContent, sdlgf::Font* f,
			std::function<void(sdlgf::ITrigger*)> onClick);

		//setters
		TextButton* SetText(std::string newText);
		TextButton* SetBackgroundColors(sdlgf::ColorF idle, sdlgf::ColorF hover, sdlgf::ColorF click, sdlgf::ColorF disabled = sdlgf::ColorF(0.2f, 0.2f, 0.2f, 0.5f));
		TextButton* SetTextColors(DWORD idle, DWORD hover, DWORD click, DWORD disabled = 0xFF888888);
		TextButton* SetOutline(float thickness, sdlgf::ColorF idle, sdlgf::ColorF hover, sdlgf::ColorF click, sdlgf::ColorF disabled = sdlgf::ColorF(0, 0, 0, 1));
		TextButton* SetPadding(float x, float y);

		void Init(sdlgf::Camera* cam) override;
		void Draw(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime) override;
	};
}
