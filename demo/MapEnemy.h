#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Player.h"
#include "BattleEncounter.h"
#include <deque>
#include "Debug.h"

namespace Demo {
    class MapEnemy : public sdlgf::IGameObject, virtual public Pointable {
    private:
        enum class State { Idle, Patrol, Chase, Return };

        State currentState = State::Idle;
        BattleEncounter encounterData;

        float startX, startY;
        float speed = 75.f;
        float aggroRadius = 80.f;
        float returnRadius = 150.f;
        float tetherRadius = 400.f;

        float losTimer = 0.f;
        bool lastLosResult = true;

        std::shared_ptr<sdlgf::Texture> texture;
        std::shared_ptr<sdlgf::AnimatedSprite> sprite;
        std::shared_ptr<sdlgf::RectangleCollider> collider;

        std::weak_ptr<Player> targetPlayer;
        sdlgf::ColliderManager* colliderManager = nullptr;
        Game* game = nullptr;

        bool isDefeated = false;
        float respawnTimer = 0.f;
        float postBattleCooldown = 0.f;

        // queue to store footprints
        std::deque<D3DXVECTOR2> chasePath;
        std::deque<D3DXVECTOR2> returnPath;
        D3DXVECTOR2 lastPlayerPos{ 0, 0 };
        D3DXVECTOR2 lastEnemyPos{ 0, 0 };

        // simulated raycast function for hybrid ai
        bool CheckLineOfSight(float startX, float startY, float targetX, float targetY);
        std::function<void(std::shared_ptr<MapEnemy>)> onEncounterTriggered;

        std::shared_ptr<sdlgf::Texture> particleTex;
        std::unique_ptr<sdlgf::ParticleSystem> wealthyAura;

        std::shared_ptr<sdlgf::Font> hpFont;
        std::shared_ptr<sdlgf::FontSprite> hpFontSprite;
        void DrawHpLabel(sdlgf::Camera* camera, unsigned long long deltaTime, float x, float y);
    public:
        static bool isDisabled;
        MapEnemy(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y, const BattleEncounter& data);
        ~MapEnemy();

		std::pair<float, float> GetPoint() const override {
			return { GetWorldX(), GetWorldY() };
		}
        void Init(Game* game, sdlgf::GraphicsDevice* gd, sdlgf::ColliderManager* colMan, std::shared_ptr<Player> player);
        void Update(unsigned long long deltaTime);
        void Draw(sdlgf::Camera* camera, unsigned long long deltaTime);

        std::string GetEnemyID() const { return encounterData.mapEnemyID; }
        bool IsDefeated() const { return isDefeated; }
        float GetRespawnTimer() const { return respawnTimer; }

        void SetDefeatedState(bool defeated, float timer) {
            isDefeated = defeated;
            respawnTimer = timer;
            if (isDefeated && colliderManager && collider) {
                colliderManager->Remove(collider);
            }
        }
        void SetEventState(EventType type);
        void SetOnEncounterTriggered(std::function<void(std::shared_ptr<MapEnemy>)> callback) { onEncounterTriggered = callback; }
        const BattleEncounter& GetEncounterData() const { return encounterData; }
        // Restores a previously rolled encounter (from a save). Ignored if the data is inconsistent.
        void SetEncounterRoll(const std::vector<std::string>& types, const std::vector<int>& hps);
    };
}