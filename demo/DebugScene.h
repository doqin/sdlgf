#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Game.h"
#include "IDraggable.h"
#include "IContainer.h"
#include "Player.h"
#include "Font.h"
#include "TextButton.h"
#include "IconButton.h"
#include "TextInputManager.h"
#include "IConversation.h"
#include "CardShop.h"

namespace Demo {
	class DebugScene : public sdlgf::IScene {
		Game* game;
		sdlgf::CommandBuffer commandBuffer;
		std::shared_ptr<sdlgf::FontSprite> myFontSprite;
		std::string typedText = "Go chu vao day: ";
		// Managers
		std::shared_ptr<DraggableManager> draggableManager;
		std::shared_ptr<sdlgf::TransformManager> transformManager;
		sdlgf::ColliderManager colliderManager;

		// Objects
		std::vector<std::shared_ptr<IContainer>> containers;
		std::vector<std::shared_ptr<IDraggable>> draggables;
		std::shared_ptr<IConversation> activeConversation;
		std::shared_ptr<Player> player;

		//UI component
		std::vector<std::shared_ptr<Demo::IButton>> uiButtons;
		std::shared_ptr<sdlgf::Texture> uiSheetTex;
		std::shared_ptr<sdlgf::Font> myFont;

	public:
		DebugScene(Game* game, int screenWidth, int screenHeight);
		void Init() override;
		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;
	};
}