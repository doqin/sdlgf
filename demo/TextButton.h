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
		std::vector<D3DXCOLOR> bgColors;
		
		//fix memory allocation
		std::wstring wText;

		// outline
		bool hasOutline;
		float outlineThickness;
		std::vector<D3DXCOLOR> outlineColors;

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
		TextButton* SetBackgroundColors(D3DXCOLOR idle, D3DXCOLOR hover, D3DXCOLOR click, D3DXCOLOR disabled = D3DXCOLOR(0.2f, 0.2f, 0.2f, 0.5f));
		TextButton* SetTextColors(DWORD idle, DWORD hover, DWORD click, DWORD disabled = 0xFF888888);
		TextButton* SetOutline(float thickness, D3DXCOLOR idle, D3DXCOLOR hover, D3DXCOLOR click, D3DXCOLOR disabled = D3DXCOLOR(0, 0, 0, 1));
		TextButton* SetPadding(float x, float y);

		void Init(sdlgf::Camera* cam) override;
		void Draw(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime) override;
	};
}
