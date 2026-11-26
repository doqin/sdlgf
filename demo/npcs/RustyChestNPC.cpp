#include "pch.h"
#include "RustyChestNPC.h"

namespace Demo {
    RustyChestNPC::RustyChestNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y)
        : INPC(tm, x, y) {
    }

    void RustyChestNPC::Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) {
        INPC::Init(gd, camera, p, cm, font, drawBuffer);

        collider = std::make_shared<sdlgf::RectangleCollider>(transformManager, 32.f, 32.f, this->GetWorldX(), this->GetWorldY());
        collider->SetOriginCenter();
        cm->Add(collider);

        spritesheet = gd->GetAssets().GetTexture(L"assets/chestclosed.png");

        sprite = std::make_shared<sdlgf::AnimatedSprite>(spritesheet, sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 32, 1), 12, true);
        sprite->SetOrigin(16.f, 16.f);
        sprite->SetPosition(this->GetWorldX(), this->GetWorldY());
		openedTexture = gd->GetAssets().GetTexture(L"assets/chestopened.png");
		openedSprite = std::make_shared<sdlgf::AnimatedSprite>(openedTexture, sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 32, 1), 12, true);
		openedSprite->SetOrigin(16.f, 16.f);
		openedSprite->SetPosition(this->GetWorldX(), this->GetWorldY());
    }

    void RustyChestNPC::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) {
        if (isOpened) {
			openedSprite->Begin();
			openedSprite->Draw(camera, deltaTime);
			openedSprite->End();
        }
        else {
            sprite->Begin();
            sprite->Draw(camera, deltaTime);
            sprite->End();
        }
        INPC::Draw(camera, deltaTime);
    }
}