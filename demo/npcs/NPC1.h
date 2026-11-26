#pragma once
#include "pch.h"
#include "sdlgf.h"      
#include "Extras.h"  
#include "Player.h"
#include "DialogueLine.h"
#include "IConversation.h"
#include "INPC.h"

namespace Demo {
    class NPC1 : public INPC {
    public:
        NPC1(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y);

        void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) override;

        void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) override;

        bool CanInteract() const override { return isPlayerNear; }
    };
}