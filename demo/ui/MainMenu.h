#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "IScene.h"
#include "Game.h"
#include "TextButton.h"
#include "IconButton.h"
#include "TextIconButton.h"
#include "KeyboardNavigator.h"
#include <functional>

namespace Demo
{
   class SaveGameState;
	class MainMenu : public sdlgf::IScene
	{
	private:
		Game* game;
		std::shared_ptr<sdlgf::TransformManager> transformManager;
		std::shared_ptr<sdlgf::SaveManager> saveManager;
		std::shared_ptr<sdlgf::Font> font;
		std::shared_ptr<sdlgf::FontSprite> fontSprite;
		std::shared_ptr<sdlgf::StaticSprite> bgSprite;
		std::shared_ptr<sdlgf::Texture> bgTex;
		std::shared_ptr<sdlgf::Texture> titleTex;

		//button component
		std::shared_ptr<sdlgf::Texture> buttonSheetTex;
		std::vector<std::shared_ptr<Demo::IButton>> uiButtons;

		//temporarily comment, only uncomment when needed. Ex: That button changes affect another component in the scene.

		std::shared_ptr<TextIconButton> continueButton;
		std::shared_ptr<TextIconButton> newGameButton;
		std::shared_ptr<TextIconButton> optionsButton;
		std::shared_ptr<TextIconButton> creditsButton;
		std::shared_ptr<TextIconButton> quitButton;
		std::shared_ptr<sdlgf::CommandBuffer> drawBuffer;
		std::shared_ptr<sdlgf::CommandBuffer> commandBuffer;

		//use to scale or relocate the sprite/object
		int lastScreenWidth;
		int lastScreenHeight;

		bool isTransitioning = false;

		std::function<void()> doContinueGame;

		KeyboardNavigator keyboardNavigator;
		std::vector<KeyboardNavigator::Candidate> CollectKeyboardCandidates();

		//map preview pentagons: one cropped snapshot of each featured map's world
		static const int MAP_PREVIEW_COUNT = 4;
		std::shared_ptr<sdlgf::Texture> mapPreviews[MAP_PREVIEW_COUNT];
		void RenderMapPreviews();

	public:
		static std::shared_ptr<SaveGameState> gameSaveState;

		MainMenu(Game* game, int screenWidth, int screenHeight)
			: IScene(screenWidth, screenHeight),
			game(game) {
		}
		void Init() override;
		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;
		// The preview textures are render targets holding a one-off snapshot, not a loaded
		// asset - if the driver loses render-target contents they come back blank, so they
		// need to be redrawn.
		void OnDeviceReset() override { RenderMapPreviews(); }
		void UpdateLayout(int width, int height);
		void DrawBackground(unsigned long long deltaTime);
	};
}

