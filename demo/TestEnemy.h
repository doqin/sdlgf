#pragma once
#include "IEnemy.h"

namespace Demo {
	class TestEnemy : public EnemyBase<TestEnemy> {
	private:
		std::shared_ptr<sdlgf::Texture> texture;
		std::shared_ptr<sdlgf::StaticSprite> sprite;
		std::shared_ptr<sdlgf::Texture> roundProjectileTexture;
		std::weak_ptr<Player> player;
		int currentCycle = -1;
		int skillTurnThisCycle = -1;

	public:
		using EnemyBase<TestEnemy>::EnemyBase;
		// Unscaled 64px sprite.
		float GetBodyWidth() const override { return 64.f; }
		float GetBodyHeight() const override { return 64.f; }
		void Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera);
		void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) override;
		sdlgf::ISprite* GetBodySprite() override { return sprite.get(); }

		void OnTurnBegin(std::shared_ptr<Player> player, std::shared_ptr<PopUpMessage> popUpMessage, int currentTurn) override;
		void StartAttack(std::shared_ptr<Player> player, std::vector<std::shared_ptr<IEnemy>>* enemies, std::shared_ptr<PopUpMessage> popUpMessage, sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, int currentTurn) override;
	};
}