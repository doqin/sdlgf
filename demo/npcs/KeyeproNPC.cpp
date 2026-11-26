#include "pch.h"
#include "KeyeproNPC.h"

namespace Demo {
	KeyeproNPC::KeyeproNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y)
		: INPC(tm, x, y) {
	}

	void KeyeproNPC::Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p,
		std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font,
		std::shared_ptr<sdlgf::CommandBuffer> drawBuffer)
	{
		INPC::Init(gd, camera, p, cm, font, drawBuffer);
		this->gd = gd;
		this->colliderManager = cm;

		// 32x32 centred on the object covers the tile it is standing on.
		collider = std::make_shared<sdlgf::RectangleCollider>(transformManager, 32.f, 32.f, GetWorldX(), GetWorldY());
		collider->SetOriginCenter();
		cm->Add(collider);

		spritesheet = gd->GetAssets().GetTexture(L"assets/boss-Sheet.png");
		sprite = std::make_shared<sdlgf::AnimatedSprite>(spritesheet, sdlgf::Utils::CreateRectsHorizontal(0, 0, 128, 128, 6), 6, true);
		sprite->SetOrigin(64.f, 80.f);
		sprite->SetPosition(GetWorldX(), GetWorldY());

		// A save restored before Init would have set the phase already; re-apply it so a defeated
		// NPC does not get its collider back.
		if (phase == Phase::Defeated) SetPhase(Phase::Defeated);
	}

	void KeyeproNPC::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime)
	{
		if (phase == Phase::Defeated) return;

		sprite->Begin();
		sprite->Draw(camera, deltaTime);
		sprite->End();

		INPC::Draw(camera, deltaTime);
	}

	void KeyeproNPC::SetPhase(Phase newPhase)
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
