#pragma once
#include "Extras.h"
#include "sdlgf.h"

namespace GO {
	class Player : public sdlgf::IGameObject, sdlgf::ISaveable {
		inline static size_t playerCount = 0;
		sdlgf::GraphicsDevice* graphicsDevice = nullptr;
		sdlgf::Camera* camera = nullptr;
		sdlgf::CommandBuffer* commandBuffer = nullptr;
		std::shared_ptr<sdlgf::Texture> marioTexture = nullptr;
		std::shared_ptr<sdlgf::AnimatedSprite> mario = nullptr;
		std::shared_ptr<sdlgf::RectangleCollider> collider = nullptr;
		std::shared_ptr<sdlgf::ColliderManager> colliderManager = nullptr;
	public:
		Player(std::weak_ptr<sdlgf::TransformManager> transformManager) : IGameObject(transformManager) { playerCount++; }
		Player(std::weak_ptr<sdlgf::TransformManager> transformManager, float x, float y, float rotation = 0, float scaleX = 1, float scaleY = 1) : IGameObject(transformManager, x, y, rotation, scaleX, scaleY) { playerCount++; }
		~Player();
		void Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::CommandBuffer* commandBuffer, sdlgf::Camera* camera, std::shared_ptr<sdlgf::ColliderManager> colliderManager);
		void Update(unsigned long long deltaTime);
		void Draw(unsigned long long deltaTime);

		// Inherited via ISaveable
		std::string GetSaveID() const override;
		void GenerateSaveData(nlohmann::json& outData) override;
		void RestoreSaveData(const nlohmann::json& inData) override;
	};
}