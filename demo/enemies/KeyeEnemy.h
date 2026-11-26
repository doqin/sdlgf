#pragma once
#include "IEnemy.h"

namespace Demo {
    class KeyeEnemy : public EnemyBase<KeyeEnemy> {
    private:
        std::shared_ptr<sdlgf::Texture> texture;
        std::shared_ptr<sdlgf::AnimatedSprite> sprite;
        std::shared_ptr<sdlgf::Texture> projTexture;
        std::vector<RECT> projFrames;
        std::weak_ptr<Player> player;
        int currentCycle = -1;
        int skillTurnThisCycle = -1;

        int GetRandomPattern();
        void PatternRoundCircle(float projDamage);
        void PatternSightline(float projDamage);
    public:
        using EnemyBase<KeyeEnemy>::EnemyBase;
        void Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera);
        void Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime) override;
        sdlgf::ISprite* GetBodySprite() override { return sprite.get(); }
        void OnTurnBegin(std::shared_ptr<Player> player, std::shared_ptr<PopUpMessage> popUpMessage, int currentTurn) override;
        void StartAttack(std::shared_ptr<Player> player, std::vector<std::shared_ptr<IEnemy>>* enemies, std::shared_ptr<PopUpMessage> popUpMessage, sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, int currentTurn) override;
    };
}