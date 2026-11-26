#pragma once
#include "IEnemy.h"
#include "Extras.h"
#include <utility>
#include <vector>

namespace Demo {
	class KernelEnemy : public EnemyBase<KernelEnemy> {
	private:
		std::shared_ptr<sdlgf::Texture> texture;
		std::shared_ptr<sdlgf::AnimatedSprite> sprite;

		std::shared_ptr<sdlgf::Texture> mineTexture;
		std::shared_ptr<sdlgf::Texture> bulletTexture;

		std::weak_ptr<Player> player;

		// The beams themselves live in the projectile system; Bad Sector only keeps the
		// lines it picked so it can work out which cell the player is boxed into, and
		// whether that cell is currently being telegraphed.
		bool telegraphingCell = false;
		float badSectorSpinDirection = 1.f;
		std::vector<float> badSectorArmAngles;
		std::vector<float> laserVerticals;
		std::vector<float> laserHorizontals;

		std::pair<float, float> GetSafeCellCenter(float playerX, float playerY) const;

		//ability vars
		int currentCycle = -1;
		int skillTurnThisCycle = -1;

		// isAlone is false when other enemies share the fight, and the patterns thin out to
		// leave room for their bullets.
		void PatternDefrag(float projDamage, bool isAlone);
		void PatternPing999(float projDamage, bool isAlone);
		void PatternZigzag(float projDamage, bool isAlone);
		void PatternBadSector(float projDamage, bool isAlone);

	public:
		using EnemyBase<KernelEnemy>::EnemyBase;

		void Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera);
		void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) override;
		sdlgf::ISprite* GetBodySprite() override { return sprite.get(); }

		void OnTurnBegin(std::shared_ptr<Player> player, std::shared_ptr<PopUpMessage> popUpMessage, int currentTurn) override;
		void StartAttack(std::shared_ptr<Player> player, std::vector<std::shared_ptr<IEnemy>>* enemies, std::shared_ptr<PopUpMessage> popUpMessage, sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, int currentTurn) override;
	};
}
