#pragma once
#include "IExpressionCard.h"

namespace Demo {
	class EnemyCard : public IExpressionCard {
	private:
		std::shared_ptr<IEnemy> enemy;
		std::shared_ptr<sdlgf::Texture> arrowTexture;
		std::shared_ptr<sdlgf::StaticSprite> arrowSprite;
		std::shared_ptr<sdlgf::Texture> uiTexture;
		std::shared_ptr<sdlgf::StaticSprite> targetSprite;
		std::shared_ptr<sdlgf::StaticSprite> untargetedSprite;
		std::shared_ptr<sdlgf::StaticSprite> targetedSprite;
	public:
		EnemyCard(std::weak_ptr<sdlgf::TransformManager> tm, std::shared_ptr<IEnemy> enemy, float x = 0, float y = 0)
			: IGameObject(tm, x, y), IExpressionCard(tm, 96, 32, x, y), enemy(enemy) {}
		std::shared_ptr<IEnemy> GetValue() override;
		void Draw(unsigned long long deltaTime) override;
		bool OnDrop(std::shared_ptr<IDraggable> draggable) override;
		void OnDropMissed(std::weak_ptr<sdlgf::IGameObject> oldParent, float oldWorldX, float oldWorldY) override;
		bool CanBeStoredInContainer() const override { return false; }
	};
}
