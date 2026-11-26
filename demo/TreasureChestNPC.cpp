#include "pch.h"
#include "TreasureChestNPC.h"
#include "GameItems.h"
namespace Demo {
    TreasureChestNPC::TreasureChestNPC(
        std::weak_ptr<sdlgf::TransformManager> tm, float x, float y,
        std::vector<ChestReward> rewards, bool randomPick)
        : INPC(tm, x, y), rewards(std::move(rewards)), randomPick(randomPick) {
    }
    void TreasureChestNPC::Init(
        sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p,
        std::shared_ptr<sdlgf::ColliderManager> cm,
        std::shared_ptr<sdlgf::Font> font,
        std::shared_ptr<sdlgf::CommandBuffer> drawBuffer)
    {
        INPC::Init(gd, camera, p, cm, font, drawBuffer);

        collider = std::make_shared<sdlgf::RectangleCollider>(
            transformManager, 32.f, 8.f, this->GetWorldX(), this->GetWorldY() + 12.f);
        collider->SetOriginCenter();
        cm->Add(collider);
        spritesheet = std::make_shared<sdlgf::Texture>(gd);
        spritesheet->LoadTexture(L"assets/chestclosed.png");
        sprite = std::make_shared<sdlgf::AnimatedSprite>(
            spritesheet.get(),
            sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 32, 1),
            12, true);
        sprite->SetOrigin(16.f, 16.f);
        sprite->SetPosition(this->GetWorldX(), this->GetWorldY());
		openedTexture = std::make_shared<sdlgf::Texture>(gd);
		openedTexture->LoadTexture(L"assets/chestopened.png");
		openedSprite = std::make_shared<sdlgf::AnimatedSprite>(
			openedTexture.get(),
			sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 32, 1),
			12, true);
		openedSprite->SetOrigin(16.f, 16.f);
		openedSprite->SetPosition(this->GetWorldX(), this->GetWorldY());

    }
    std::vector<ChestReward> TreasureChestNPC::Open(Player* p) {
        if (isOpened) return {};
        isOpened = true;

        std::vector<ChestReward> given;
        if (randomPick && !rewards.empty()) {
            given.push_back(rewards[rand() % rewards.size()]);
        }
        else {
            given = rewards;
        }

        for (auto& r : given) {
            if (r.type == ChestRewardType::ITEM) {
                p->GetInventoryItems().AddItem(r.itemID, r.quantity);
            }
            else if (r.type == ChestRewardType::CARD) {
                p->AddCardToInventory(r.cardSaveID);
            }
        }
        return given;
    }
    void TreasureChestNPC::Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) {
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