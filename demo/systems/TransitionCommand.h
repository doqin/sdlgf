#pragma once
#include <memory>
#include <vector>
#include "Extras.h"
#include "GraphicsDevice.h"
#include "Application.h"
#include "Camera.h"

namespace Demo {
    class Game;
    class TransitionCommand : public sdlgf::ICommand {
    private:
        Game* game;
        sdlgf::Camera* uiCamera;
        float duration;
        float elapsedTime;
        bool isTransitioningIn;
        int numPillars;
        int screenWidth;
        int screenHeight;
        int frame = 0;
    public:
        TransitionCommand(Game* game, sdlgf::Camera* uiCamera, float duration, bool isTransitioningIn, int numPillars = 10);
        void Execute(unsigned long long deltaTime) override;

        static float EaseInOutQuad(float t) {
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        }
    };
}