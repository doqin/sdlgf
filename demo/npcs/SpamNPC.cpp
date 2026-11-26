#include "pch.h"
#include "SpamNPC.h"

namespace Demo {
	SpamNPC::SpamNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y)
		: INPC(tm, x, y) {
	}

	void SpamNPC::Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p,
		std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font,
		std::shared_ptr<sdlgf::CommandBuffer> drawBuffer)
	{
		INPC::Init(gd, camera, p, cm, font, drawBuffer);
		this->gd = gd;
		this->colliderManager = cm;

		// 64x64 centred on the object blocks the hill path up to trigger_p.
		collider = std::make_shared<sdlgf::RectangleCollider>(transformManager, 64.f, 64.f, GetWorldX(), GetWorldY());
		collider->SetOriginCenter();
		cm->Add(collider);

		spritesheet = gd->GetAssets().GetTexture(L"assets/spam-Sheet.png");
		sprite = std::make_shared<sdlgf::AnimatedSprite>(spritesheet, sdlgf::Utils::CreateRectsHorizontal(0, 0, 64, 64, 12), 12, true);
		sprite->SetOrigin(32.f, 48.f);
		sprite->SetPosition(GetWorldX(), GetWorldY());

		// A save restored before Init would have set the phase already; re-apply it so a defeated
		// NPC does not get its collider back.
		if (phase == Phase::Defeated) SetPhase(Phase::Defeated);
	}

	void SpamNPC::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime)
	{
		if (phase == Phase::Defeated) return;

		sprite->Begin();
		sprite->Draw(camera, deltaTime);
		sprite->End();

		INPC::Draw(camera, deltaTime);
	}

	void SpamNPC::SetPhase(Phase newPhase)
	{
		phase = newPhase;

		if (phase == Phase::Defeated && collider) {
			if (auto cm = colliderManager.lock()) {
				cm->Remove(collider);
			}
			collider = nullptr;
		}
	}
}
