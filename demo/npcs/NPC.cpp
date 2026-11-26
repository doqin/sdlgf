#include "pch.h"
#include "NPC.h"

namespace Demo {
	NPC::NPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y, const NPCConfig& config)
		: INPC(tm, x, y), config(config) {
	}

	void NPC::Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) {
		INPC::Init(gd, camera, p, cm, font, drawBuffer);

		collider = std::make_shared<sdlgf::RectangleCollider>(transformManager, config.colW, config.colH, this->GetWorldX(), this->GetWorldY() + config.colOffsetY);
		collider->SetOriginCenter();
		cm->Add(collider);

		spritesheet = std::make_shared<sdlgf::Texture>(gd);
		spritesheet->LoadTexture(config.texturePath.c_str());
		sprite = std::make_shared<sdlgf::AnimatedSprite>(spritesheet.get(), sdlgf::Utils::CreateRectsHorizontal(0, 0, config.frameW, config.frameH, config.frameCount), config.animSpeed, true);
		sprite->SetOrigin(config.frameW / 2.0f, config.frameH / 2.0f);
		sprite->SetPosition(this->GetWorldX(), this->GetWorldY());
	}

	void NPC::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) {
		sprite->Begin();
		sprite->Draw(camera, deltaTime);
		sprite->End();

		INPC::Draw(camera, deltaTime);
	}
}