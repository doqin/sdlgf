#pragma once
#include "IToken.h"

namespace Demo {
	class EnergyToken : public IToken {
	private:
		std::shared_ptr<sdlgf::Texture> texture;
		std::shared_ptr<sdlgf::StaticSprite> sprite;

		float floatOffset = 0.f;
		float floatTime = 0.f;
		int alpha = 255;
		float lifetime = 1500.f; // 1.5s Loot Time
		bool isExpired = false;

	public:
		EnergyToken(std::weak_ptr<sdlgf::TransformManager> tm, sdlgf::GraphicsDevice* gd, float x, float y);

		void Update(unsigned long long deltaTime) override;
		void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) override;
		void OnCollect(Player* player, IBattleScene* scene) override;
		bool IsDone() const override;
	};
}