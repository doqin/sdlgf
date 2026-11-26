#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "InputManager.h"
#include "Player.h"
#include <functional>

namespace Demo {
    class HackTerminal : public sdlgf::IGameObject {
    private:
        sdlgf::GraphicsDevice* gd;
        sdlgf::Camera* worldCamera;
        std::weak_ptr<sdlgf::TransformManager> transformManager;
        std::shared_ptr<sdlgf::Texture> spritesheet;
        std::shared_ptr<sdlgf::StaticSprite> sprite;
        std::weak_ptr<Player> player;
        std::shared_ptr<sdlgf::FontSprite> fontSprite;
        std::shared_ptr<sdlgf::RectangleCollider> collider;

        bool isPlayerNear = false;
        const float INTERACTION_DISTANCE = 50.0f;
        bool isVisible = false;

        bool isHacked = false;
        std::string terminalName;
        int colorID;

        std::string statusMessage = "";
        float messageTimer = 0.0f;

        std::function<void(int)> onHackAttempt;

    public:
        HackTerminal(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y, int cID, std::string name);

        void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::function<void(int)> hackCallback);

        void Update(unsigned long long deltaTime);
        void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime);
        void DrawUI(sdlgf::Camera* uiCamera, unsigned long long deltaTime);

        void SetVisible(bool visible) { isVisible = visible; }
        bool IsVisible() const { return isVisible; }

        void SetHackedStatus(bool status);
        void ShowStatusMessage(const std::string& msg, float time);
        bool IsHacked() const { return isHacked; }

        int GetColorID() { return colorID; }
    };
}