#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "InputManager.h"
#include "Player.h"
#include "Debug.h"

namespace Demo {
    class SavePoint : public sdlgf::IGameObject, virtual public Pointable {
    private:
        sdlgf::GraphicsDevice* gd;
        sdlgf::Camera* worldCamera;
        std::weak_ptr<sdlgf::TransformManager> transformManager;
        std::shared_ptr<sdlgf::Texture> spritesheet;
        std::shared_ptr<sdlgf::AnimatedSprite> sprite;
        std::weak_ptr<Player> player;
        std::weak_ptr<sdlgf::SaveManager> saveManager;
        std::shared_ptr<sdlgf::FontSprite> fontSprite;
        std::shared_ptr<sdlgf::RectangleCollider> collider;
        std::weak_ptr<sdlgf::CommandBuffer> drawBuffer;

        bool isPlayerNear = false;
        const float INTERACTION_DISTANCE = 50.0f;
        bool isVisible = false;

        // Fast travel: a save point unlocks the first time it is used, and the unlock is saved
        // in PlayerGlobalData under this id ("<SceneSaveID>#<index>").
        std::string id;
        std::function<void()> onOpenMap;
        float indicatorTime = 0.f;
        // Size of the visible (virtual) screen in UI units; the UI camera can draw past it into the letterbox bars
        float screenW = 0.f, screenH = 0.f;

    public:
        SavePoint(std::weak_ptr<sdlgf::TransformManager> tm, float x = 0, float y = 0);

		std::pair<float, float> GetPoint() const override {
			return { GetWorldX(), GetWorldY() };
		}
        void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* worldCamera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::SaveManager> sm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer);

        void Update(unsigned long long deltaTime);
        void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime);
        void DrawUI(sdlgf::Camera* uiCamera, unsigned long long deltaTime);

        void SetVisible(bool visible) { isVisible = visible; }
        bool IsVisible() const { return isVisible; }

        // Called by the owning scene once all of its save points exist.
        void SetScreenSize(float w, float h) { screenW = w; screenH = h; }
        void ConfigureTravel(std::string pointId, std::function<void()> openMap) {
            id = std::move(pointId);
            onOpenMap = std::move(openMap);
        }
        const std::string& GetId() const { return id; }
        bool IsUnlocked() const;
        // Where the player appears when travelling here: just in front of the point, clear of its collider.
        // The scene refines this to a walkable spot (WorldSceneBase::FindSpawnNear); this is only the anchor.
        std::pair<float, float> GetSpawnPosition() const { return { GetWorldX(), GetWorldY() + 32.f }; }
    };
}