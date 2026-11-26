#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "InputManager.h"
#include "Player.h"
#include "Game.h"
#include "Debug.h"
#include "IShopScene.h"

namespace Demo {
	enum class ShopType { Card, Item };
    struct ShopConfig {
        ShopType type;
        ShopTier tier;
        std::function<sdlgf::IScene* (Game*, Player*, int, int)> factory;
    };
    class ShopPoint : public sdlgf::IGameObject, virtual public Pointable {
    private:
        Game* game;
        sdlgf::GraphicsDevice* gd;
        sdlgf::Camera* worldCamera;
        std::weak_ptr<sdlgf::TransformManager> transformManager;
        std::shared_ptr<sdlgf::Texture> spritesheet;
        std::shared_ptr<sdlgf::AnimatedSprite> sprite;
        std::weak_ptr<Player> player;
        std::shared_ptr<sdlgf::FontSprite> fontSprite;
        std::shared_ptr<sdlgf::RectangleCollider> collider;
        std::weak_ptr<sdlgf::CommandBuffer> drawBuffer;
        ShopConfig config;
		std::function<sdlgf::IScene* (Game*, Player*, int, int)> sceneFactory;

		bool isPlayerNear = false;
		const float INTERACTION_DISTANCE = 50.0f;
		bool isVisible = true;

	public:
		ShopPoint(std::weak_ptr<sdlgf::TransformManager> tm, float x = 0, float y = 0);

        void Init(Game* game, sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer, const ShopConfig& shopConfig);

		std::pair<float, float> GetPoint() const override {
			return { GetWorldX(), GetWorldY() };
		}

		void Update(unsigned long long deltaTime);
		void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime);
        void DrawUI(sdlgf::Camera* uiCamera, unsigned long long deltaTime);
		void SetVisible(bool visible) { isVisible = visible; }
		bool IsVisible() const { return isVisible; }
	};
}