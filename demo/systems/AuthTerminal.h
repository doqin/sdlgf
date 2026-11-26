#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "InputManager.h"
#include "Player.h"
#include "PasswordMenu.h"
#include "Debug.h"
#include <functional>

namespace Demo {
	// A terminal that guards a credential behind a 4-digit code. Interacting opens a PasswordMenu;
	// cracking the code fires onSolved once and flips the sprite to the OK frame for good.
	class AuthTerminal : public sdlgf::IGameObject, virtual public Pointable {
	private:
		sdlgf::GraphicsDevice* gd = nullptr;
		sdlgf::Camera* worldCamera = nullptr;
		std::weak_ptr<sdlgf::TransformManager> transformManager;
		std::shared_ptr<sdlgf::Texture> spritesheet;
		std::shared_ptr<sdlgf::StaticSprite> sprite;
		std::weak_ptr<Player> player;
		std::shared_ptr<sdlgf::FontSprite> fontSprite;
		std::shared_ptr<sdlgf::RectangleCollider> collider;

		PasswordMenu menu;

		bool isPlayerNear = false;
		const float INTERACTION_DISTANCE = 50.0f;
		bool isVisible = false;
		bool isSolved = false;

		std::wstring password;
		std::function<void()> onSolved;

	public:
		AuthTerminal(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y);

		std::pair<float, float> GetPoint() const override { return { GetWorldX(), GetWorldY() }; }

		void Init(Game* game, sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, sdlgf::Camera* uiCamera,
			std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm,
			std::shared_ptr<sdlgf::Font> font);

		void Update(unsigned long long deltaTime);
		void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime);
		void DrawUI(sdlgf::Camera* uiCamera, unsigned long long deltaTime);

		void SetVisible(bool visible) { isVisible = visible; }
		bool IsVisible() const { return isVisible; }

		// The code the player has to crack. Four characters, each '0'-'9'.
		void SetPassword(const std::wstring& pw) { password = pw; }
		const std::wstring& GetPassword() const { return password; }

		void SetSolved(bool solved);
		bool IsSolved() const { return isSolved; }

		void SetOnSolved(std::function<void()> cb) { onSolved = std::move(cb); }

		bool IsMenuOpen() const { return menu.IsOpen(); }
	};
}
