#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "IScene.h"
#include "Game.h"
#include "IConversation.h"

namespace Demo {
    class OutroScene : public sdlgf::IScene {
    private:
        Game* game;
        std::shared_ptr<sdlgf::Font> font;
        std::shared_ptr<sdlgf::FontSprite> fontSprite;
        std::shared_ptr<IConversation> conversation;
        bool hasTransitioned = false;
    public:
        OutroScene(Game* game, int screenWidth, int screenHeight)
            : IScene(screenWidth, screenHeight), game(game){}
        void Init() override;
        void Update(unsigned long long deltaTime) override;
        void DrawWorld(unsigned long long deltaTime) override;
        void DrawUI(unsigned long long deltaTime) override;
    };
}
