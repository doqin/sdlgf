#pragma once
#include "IScene.h"
#include "Texture.h"
#include "Sprites.h"
#include "Game.h"

namespace Demo {
    class SplashScene : public sdlgf::IScene {
    private:
        Game* game;
        std::shared_ptr<sdlgf::Texture> logo1Tex, logo2Tex;
        std::shared_ptr<sdlgf::StaticSprite> logo1Sprite, logo2Sprite;

        float timer = 0.0f;
        float alpha = 0.0f;
    public:
        SplashScene(Game* game, int screenW, int screenH) : IScene(screenW, screenH), game(game) {}
        void Init() override;
        void Update(unsigned long long deltaTime) override;
        void DrawWorld(unsigned long long deltaTime) override;
        void DrawUI(unsigned long long deltaTime) override;
    };
}