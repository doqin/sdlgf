#include "pch.h"
#include "NPC1.h"

namespace Demo {
    NPC1::NPC1(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y)
        : INPC(tm, x, y) {
    }

    void NPC1::Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) {
        INPC::Init(gd, camera, p, cm, font, drawBuffer);

        collider = std::make_shared<sdlgf::RectangleCollider>(transformManager, 32.f, 8.f, this->GetWorldX(), this->GetWorldY() + 12.f);
        collider->SetOriginCenter();
        cm->Add(collider);

        spritesheet = gd->GetAssets().GetTexture(L"assets/NPC1.png");
        sprite = std::make_shared<sdlgf::AnimatedSprite>(spritesheet, sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 32, 4), 8, true);
        sprite->SetOrigin(16.f, 16.f);
        sprite->SetPosition(this->GetWorldX(), this->GetWorldY());
    }

    void NPC1::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) {
        sprite->Begin();
        sprite->Draw(camera, deltaTime);
        sprite->End();

        INPC::Draw(camera, deltaTime);
    }
}