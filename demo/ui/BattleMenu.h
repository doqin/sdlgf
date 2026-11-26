#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Game.h"
#include "IconButton.h"
#include "KeyboardNavigator.h"
#include "BattleTutorial.h"

namespace Demo {
	class BattleMenu {
	private:
		Game* game;
		std::shared_ptr<sdlgf::TransformManager> transformManager;
		sdlgf::Camera* uiCamera;

		bool isOpen = false;

		// UI Buttons
		std::shared_ptr<IconButton> btnResume;
		std::shared_ptr<IconButton> btnOptions;
		std::shared_ptr<IconButton> btnLeaveGame;

		std::shared_ptr<sdlgf::Texture> uiTex;

		bool pendingLeave = false;

		KeyboardNavigator keyboardNavigator;
		std::vector<KeyboardNavigator::Candidate> CollectKeyboardCandidates();

		std::shared_ptr<IconButton> btnGuide;
		std::shared_ptr<BattleTutorial> tutorial; // built lazily on the first Guide click
		std::shared_ptr<sdlgf::FontSprite> fontSprite;
		std::shared_ptr<sdlgf::Font> font = nullptr;


	public:
		BattleMenu(Game* g, std::shared_ptr<sdlgf::TransformManager> tm, sdlgf::Camera* cam);

		void Init(std::shared_ptr<sdlgf::Font> font);
		void Update(unsigned long long deltaTime);
		void Draw(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime);
		void DrawKeyboardReticle(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime);

		void Toggle();
		bool IsOpen() const { return isOpen; }
		bool IsInKeyboardMode() const { return isOpen && keyboardNavigator.IsInKeyboardMode(); }
		bool IsPendingLeave() const { return pendingLeave; }
		bool IsTutorialActive() const { return tutorial && tutorial->IsPanelVisible(); }
	};
}
