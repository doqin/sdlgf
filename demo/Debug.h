#pragma once
#include <memory>
#include "Player.h"
#include <sdlgf.h>
#include <Extras.h>

namespace Demo {
	class Pointable {
		static std::shared_ptr<sdlgf::Font> font;
		static std::shared_ptr<sdlgf::FontSprite> fontSprite;
	public:
		static bool isDrawing;
		virtual std::pair<float, float> GetPoint() const = 0;
		void DrawPosition(unsigned long long deltaTime, sdlgf::GraphicsDevice* gd, const sdlgf::Camera& camera) const;
	};

	void CreateImGuiDebugFrame(std::shared_ptr<Player> player, Game* game);
	// Debug window for battles. onKillAllEnemies is invoked when the "Kill All Enemies" button is pressed.
	void CreateImGuiBattleDebugFrame(const std::function<void()>& onKillAllEnemies);
}
