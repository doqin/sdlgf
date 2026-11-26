#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "IEnemy.h"

namespace Demo {
	class TuitionFeeEnemy : public EnemyBase<TuitionFeeEnemy> {
	private:
		std::shared_ptr<sdlgf::Texture> texture;
		std::shared_ptr<sdlgf::AnimatedSprite> sprite;

		std::shared_ptr<sdlgf::Texture> bulletTexture;
		std::vector<RECT> bulletFrames;

		std::weak_ptr<Player> player;

		bool hasSpawnTurn = false;
		int spawnTurn = 0;
		static constexpr int BUFF_WIPE_INTERVAL = 5;

		static constexpr int VISION_DEBUFF_DURATION = 1;

		static constexpr float ARENA_HALF_SIZE = 128.f;

		void BanRandomItems(std::shared_ptr<Player> player, std::shared_ptr<PopUpMessage> popUpMessage);
		void PatternSideWall(float baseDamage, std::vector<std::shared_ptr<IEnemy>>* enemies);
		void PatternDualSpiral(float baseDamage, std::vector<std::shared_ptr<IEnemy>>* enemies);
		void PatternDebtCollector(float baseDamage, std::vector<std::shared_ptr<IEnemy>>* enemies);

	public:
		using EnemyBase<TuitionFeeEnemy>::EnemyBase;

		void Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera);
		void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) override;
		sdlgf::ISprite* GetBodySprite() override { return sprite.get(); }
		void OnTurnBegin(std::shared_ptr<Player> player, std::shared_ptr<PopUpMessage> popUpMessage, int currentTurn) override;
		void StartAttack(std::shared_ptr<Player> player, std::vector<std::shared_ptr<IEnemy>>* enemies,
			std::shared_ptr<PopUpMessage> popUpMessage, sdlgf::GraphicsDevice* graphicsDevice,
			sdlgf::Camera* camera, int currentTurn) override;
	};
}