#pragma once
#include <string>
#include <vector>
#include <functional> 
#include "sdlgf.h"

namespace Demo {
	enum class EventType {
		None = 0,
		Gold = 1,
		Energy = 2
	};

	struct BattleEncounter {
		std::string mapEnemyID;
		std::vector<std::string> enemyTypes;
		std::vector<int> enemyHps;
		std::vector<std::string> randomPool;
		bool useGlobalPool = false;
		std::string bgmName = "battle_bgm";
		std::function<void(sdlgf::GraphicsDevice*, unsigned long long)> bgDrawFunc = nullptr;

		std::wstring mapTexturePath;
		int spriteWidth;
		int spriteHeight;

		int frameCount = 12;

		float hitBoxWidth;
		float hitBoxHeight;
		EventType eventType = EventType::None;
	};
}