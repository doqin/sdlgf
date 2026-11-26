#include "pch.h"
#include "SettingsManager.h"
#include "HealingPoint.h"
#include <cmath>

namespace Demo {
    HealingPoint::HealingPoint(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y)
        : IGameObject(tm, x, y), transformManager(tm) {
    }

    void HealingPoint::Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) {
        player = p;
        this->worldCamera = camera;
        fontSprite = std::make_shared<sdlgf::FontSprite>(font.get());
        this->drawBuffer = drawBuffer;
        this->gd = gd;
        collider = std::make_shared<sdlgf::RectangleCollider>(transformManager, 32.f, 8.f, GetWorldX(), GetWorldY() + 12.f);
        collider->SetOriginCenter();
        cm->Add(collider);

        spritesheet = std::make_shared<sdlgf::Texture>(gd);
        spritesheet->LoadTexture(L"assets/healingpoint-Sheet.png");

        sprite = std::make_shared<sdlgf::AnimatedSprite>(spritesheet.get(), sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 32, 5), 12);
        sprite->SetOrigin(16.f, 16.f);
        sprite->SetPosition(GetWorldX(), GetWorldY());
    }

    void HealingPoint::Update(unsigned long long deltaTime) {
        if (!isVisible) return;

        if (messageTimer > 0) {
            messageTimer -= deltaTime / 1000.0f;
            if (messageTimer <= 0) statusMessage = "";
        }

        auto pLock = player.lock();
        if (!pLock) return;

        auto [px, py] = pLock->GetWorldPosition();
        auto [sx, sy] = GetWorldPosition();

        float distance = std::sqrt((px - sx) * (px - sx) + (py - sy) * (py - sy));
        isPlayerNear = (distance <= INTERACTION_DISTANCE);

        auto inpMan = sdlgf::InputManager::GetInstance();

        if (isPlayerNear && inpMan->KeyDown(SettingsManager::GetInstance()->GetKeybind("INTERACT"))) {
            if (pLock->GetHealth() < pLock->GetMaxHealth()) {
                pLock->SetHealth(pLock->GetMaxHealth());
                statusMessage = "Healed to FULL HP!";
                sdlgf::AudioManager::GetInstance()->PlayRandom("power_up", 0.2f);
                messageTimer = 1.5f;
            }
            else {
                statusMessage = "HP is already full!";
                sdlgf::AudioManager::GetInstance()->Play("error", false, 0.3f);
                messageTimer = 1.0f;
            }
        }
    }

    void HealingPoint::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) {
        if (!isVisible) return;
        sprite->Begin();
        sprite->Draw(camera, deltaTime);
        sprite->End();
		DrawPosition(deltaTime, gd, camera);
    }

    void HealingPoint::DrawUI(sdlgf::Camera* uiCamera, unsigned long long deltaTime) {
        if (!isVisible || !uiCamera || !fontSprite || !worldCamera) return;

        auto [x, y] = GetWorldPosition();
        float zoom = worldCamera->GetZoom();

        float uiX = (x - worldCamera->GetPosition().x) * zoom;
        float uiY = (y - worldCamera->GetPosition().y) * zoom;

        if (!statusMessage.empty()) {
            float scale = 0.8f * zoom;

            fontSprite->Begin();
            fontSprite->SetText(sdlgf::Utils::Utf8ToWide(statusMessage));
            fontSprite->SetScale(scale);
            fontSprite->SetColor(0xFF55FF55);

            float textW = fontSprite->GetWidth() * scale;
            float textH = fontSprite->GetHeight() * scale;

            fontSprite->SetPosition(uiX - textW / 2.f, uiY - 45.f * zoom - textH / 2.f);
            fontSprite->SetOutline(true, 0xFF000000);
            fontSprite->Draw(*uiCamera, deltaTime);
            fontSprite->End();
        }
        else if (isPlayerNear) {
            float scale = 1.0f * zoom;

            fontSprite->Begin();
            fontSprite->SetText(SettingsManager::GetInstance()->GetKeybindDisplayName("INTERACT"));
            fontSprite->SetScale(scale);
            fontSprite->SetColor(0xFFFFFFFF);

            float textW = fontSprite->GetWidth() * scale;
            float textH = fontSprite->GetHeight() * scale;

            fontSprite->SetPosition(uiX - textW / 2.f, uiY - 30.f * zoom - textH / 2.f);
            fontSprite->SetOutline(true, 0xFF000000);
            fontSprite->Draw(*uiCamera, deltaTime);
            fontSprite->End();
        }
    }
}