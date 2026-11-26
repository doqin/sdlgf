#pragma once
#include "sdlgf.h"
#include "Extras.h"

namespace Demo {
	class IBattleScene;
	class Player;

	class IToken : public sdlgf::IGameObject {
	protected:
		bool isCollected = false;
		std::shared_ptr<sdlgf::RectangleCollider> collider;

	public:
		IToken(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y)
			: sdlgf::IGameObject(tm, x, y) {}
		virtual ~IToken() = default;

		virtual void Update(unsigned long long deltaTime) = 0;
		virtual void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) = 0;
		virtual void OnCollect(Player* player, IBattleScene* scene) = 0;
		virtual bool IsDone() const = 0;

		bool IsCollected() const { return isCollected; }
		std::weak_ptr<sdlgf::RectangleCollider> GetCollider() { return collider; }
	};
}