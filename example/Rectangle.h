#pragma once
#include "sdlgf.h"
#include "Extras.h"

namespace GO {
	class Rectangle : public sdlgf::IGameObject {
	private:
		sdlgf::GraphicsDevice* graphicsDevice = nullptr;
		sdlgf::Camera* camera = nullptr;
		std::shared_ptr<sdlgf::RectangleTrigger> trigger = nullptr;
		std::shared_ptr<sdlgf::RectangleCollider> collider = nullptr;
		float width, height;
	protected:
	public:
		Rectangle(
			std::weak_ptr<sdlgf::TransformManager> transformManager,
			float width,
			float height,
			float x = 0,
			float y = 0,
			float rotation = 0,
			float scaleX = 1,
			float scaleY = 1
		) : IGameObject(transformManager, x, y, rotation, scaleX, scaleY),
			width(width),
			height(height) {}
		Rectangle(
			std::weak_ptr<sdlgf::TransformManager> transformManager,
			std::weak_ptr<IGameObject> parent,
			float width,
			float height,
			float x = 0,
			float y = 0,
			float rotation = 0,
			float scaleX = 1,
			float scaleY = 1
		) : IGameObject(transformManager, parent, x, y, rotation, scaleX, scaleY),
			width(width),
			height(height) {}
		std::weak_ptr<sdlgf::RectangleCollider> GetCollider();
		void Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, std::shared_ptr<sdlgf::ColliderManager> colliderManager);
		void Update(unsigned long long deltaTime);
		void Draw(unsigned long long deltaTime);
		void Dispose();
	};
}