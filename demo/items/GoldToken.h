#pragma once
#include "IToken.h"

namespace Demo {
	class GoldToken : public IToken {
	private:
		int goldValue;
		std::shared_ptr<sdlgf::Texture> texture;
		std::shared_ptr<sdlgf::AnimatedSprite> sprite;

		float floatOffset = 0.f;
		float floatTime = 0.f;
		int alpha = 255;
		float lifetime = 3000.f;
		bool isExpired = false;

	public:
		GoldToken(std::weak_ptr<sdlgf::TransformManager> tm, sdlgf::GraphicsDevice* gd, float x, float y, int value);

		void Update(unsigned long long deltaTime) override;
		void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) override;
		void OnCollect(Player* player, IBattleScene* scene) override;
		bool IsDone() const override;
	};
}