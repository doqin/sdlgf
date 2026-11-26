#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Game.h"
#include "Player.h"
#include "IconButton.h"
#include <functional>

namespace Demo {
	/// <summary>
	/// Earthbound-style status HUD anchored at the bottom-center of world scenes.
	/// Shows the player's icon, HP and gold as boxed digit fields, plus a button
	/// that opens the inventory. Panel and borders are drawn with GraphicsDevice.
	/// Reusable by any scene that owns a Player.
	/// </summary>
	class PlayerHUD {
		Game* game;
		std::shared_ptr<Player> player;
		std::shared_ptr<sdlgf::TransformManager> transformManager;
		sdlgf::Camera* uiCamera;
		std::shared_ptr<sdlgf::Font> font;

		std::shared_ptr<sdlgf::FontSprite> fontSprite;
		std::shared_ptr<sdlgf::Texture> playerTex;
		std::shared_ptr<sdlgf::StaticSprite> playerIcon;
		std::shared_ptr<sdlgf::Texture> uiTex;
		std::shared_ptr<IconButton> btnInventory;

		std::function<void()> onInventoryOpen;
		bool visible = true;
		// Fades toward barely-visible while the player is moving, back to full when idle
		float opacity = 1.0f;
		sdlgf::Color ApplyOpacity(sdlgf::Color color) const;

		// Layout cache, recomputed every frame so the panel can grow with its content
		std::wstring hpValue;
		std::wstring goldValue;
		int cellCount = 2;
		float labelW = 0, groupW = 0, statsRowH = 0;
		float panelX = 0, panelY = 0, panelW = 0, panelH = 0;
		float contentX = 0, contentY = 0, contentW = 0, contentH = 0;
		void UpdateLayout();
		void DrawPanel(sdlgf::GraphicsDevice* gd);
		void DrawStatRow(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime, float rowY, const wchar_t* label, const std::wstring& value, sdlgf::Color digitColor);
	public:
		PlayerHUD(Game* game, std::shared_ptr<Player> player, std::shared_ptr<sdlgf::TransformManager> transformManager, sdlgf::Camera* uiCamera, std::shared_ptr<sdlgf::Font> font);

		void Init();
		void Update(unsigned long long deltaTime);
		void Draw(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime);

		void SetOnInventoryOpen(std::function<void()> callback) { onInventoryOpen = callback; }
		void SetVisible(bool value) { visible = value; }
		bool IsVisible() const { return visible; }
	};
}
