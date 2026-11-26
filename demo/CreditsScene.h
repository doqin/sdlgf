#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "IScene.h"
#include "Game.h"
#include "IconButton.h"
#include "KeyboardNavigator.h"
#include <vector>
#include <string>

namespace Demo
{
	class CreditsScene : public sdlgf::IScene
	{
	private:
		Game* game;
		std::shared_ptr<sdlgf::TransformManager> transformManager;

		std::shared_ptr<sdlgf::StaticSprite> bgSprite;
		std::shared_ptr<sdlgf::Texture> bgTex;
		std::shared_ptr<sdlgf::Texture> uiSheetTex;
		std::shared_ptr<sdlgf::Texture> placeholderTex;

		std::shared_ptr<sdlgf::Font> font;
		std::shared_ptr<sdlgf::FontSprite> fontSprite;

		std::vector<std::shared_ptr<Demo::IButton>> uiButtons;
		std::shared_ptr<IconButton> backButton;

		// Re-use volume buttons for Next/Prev page
		std::shared_ptr<IconButton> btnPrevPage;
		std::shared_ptr<IconButton> btnNextPage;

		bool isGoingBack = false;
		int lastScreenWidth;
		int lastScreenHeight;

		//pagination
		int currentPage = 0;
		std::vector<std::vector<std::wstring>> creditsPages;

		KeyboardNavigator keyboardNavigator;
		std::vector<KeyboardNavigator::Candidate> CollectKeyboardCandidates();

	public:
		CreditsScene(Game* game, int screenWidth, int screenHeight)
			: IScene(screenWidth, screenHeight), game(game) {
		}

		void Init() override;
		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;
		void DrawBackground(unsigned long long deltaTime);
		void DrawOverlay(unsigned long long deltaTime);
		void DrawCreditsText(unsigned long long deltaTime);
		void DrawPagination(unsigned long long deltaTime);
		void UpdateLayout();
	};
}