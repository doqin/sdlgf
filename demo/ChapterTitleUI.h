#pragma once
#include "sdlgf.h"
#include <string>
#include <memory>

namespace Demo {
	class ChapterTitleUI {
	private:
		std::wstring mainTitle;
		std::wstring subTitle;
		float timer;
		float maxTime;
		float alpha;

		enum class State { Hidden, FadeIn, Visible, FadeOut };
		State state;

		std::shared_ptr<sdlgf::FontSprite> fontSprite;

		sdlgf::Color titleColor;
		sdlgf::Color subColor;
		sdlgf::Color shadowColor;

	public:
		ChapterTitleUI(std::shared_ptr<sdlgf::Font> font);

		void Show(const std::wstring& title, const std::wstring& sub, float duration, sdlgf::Color tColor, sdlgf::Color sColor, sdlgf::Color shColor);
		void Update(unsigned long long deltaTime);

		void Draw(sdlgf::Camera* uiCam, unsigned long long deltaTime);

		bool IsActive() const { return state != State::Hidden; }
	};
}