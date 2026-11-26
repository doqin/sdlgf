#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Game.h"
#include "Player.h"

namespace Demo {
	class World1Scene : public sdlgf::IScene {
	Game* game;
	std::shared_ptr<Player> player;
	sdlgf::ColliderManager colliderManager;
	std::shared_ptr<sdlgf::TransformManager> transformManager;
	public:
		World1Scene(Game* game, int screenWidth, int screenHeight) : IScene(screenWidth, screenHeight), game(game) {}
		void Init() override;
		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;
	};
}