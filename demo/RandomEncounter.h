#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Player.h"
#include <functional>

namespace Demo {
	std::function<void(const sdlgf::Map::ObjectArea&)> GetRandomEncounterFunc(
		sdlgf::IGame* game,
		std::shared_ptr<Player> player,
		std::vector<std::pair<std::string, int>> possibleEnemies,
		std::shared_ptr<sdlgf::CommandBuffer> drawBuffer,
		std::shared_ptr<sdlgf::CommandBuffer> commandBuffer,
		bool* isGamePaused,
		sdlgf::Camera* uiCamera,
		std::function<void(sdlgf::GraphicsDevice*, unsigned long long)> customBackgroundDraw = nullptr
	);
}