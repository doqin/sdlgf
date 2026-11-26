#pragma once
#include "INPC.h"

namespace Demo {
	// The maze's secret boss, guarding the only rusty key in the level. Talks once, then the
	// scene starts the fight when the conversation ends. Its collider is removed once beaten.
	class CupidNPC : public INPC {
	public:
		enum class Phase {
			Waiting,   // not yet defeated (also covers "lost and can try again")
			Defeated   // beaten - despawned
		};

	private:
		Phase phase = Phase::Waiting;
		std::weak_ptr<sdlgf::ColliderManager> colliderManager;

		void MapCharacterVoices(std::unordered_map<std::wstring, std::string>& voiceMap) override {
			voiceMap[L"???"] = "bleep21";
			voiceMap[L"Pacman"] = "bleep21";
		}
	public:
		CupidNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y);

		void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p,
			std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font,
			std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) override;

		void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) override;

		bool CanInteract() const override { return isPlayerNear && phase != Phase::Defeated; }

		Phase GetPhase() const { return phase; }
		void SetPhase(Phase newPhase);
	};
}
