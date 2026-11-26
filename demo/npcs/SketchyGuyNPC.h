#pragma once
#include "INPC.h"

namespace Demo {
	// The card-pack peddler: a shifty little process that sells sealed packs of "acquired" cards.
	// First meeting is a short conversation (he's sketchy, the player goes along with it); every
	// meeting after that opens the buy prompt. All of his flavour text is baked into the class so
	// any scene can drop him in with just a position - see SketchyGuyGlobalData for the shared
	// met / first-pack state.
	class SketchyGuyNPC : public INPC {
	private:
		std::weak_ptr<sdlgf::ColliderManager> colliderManager;

		void MapCharacterVoices(std::unordered_map<std::wstring, std::string>& voiceMap) override {
			voiceMap[L"???"] = "bleep21";
			voiceMap[L"Peddler"] = "bleep21";
		}
	public:
		SketchyGuyNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y);

		void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p,
			std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font,
			std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) override;

		void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) override;

		bool CanInteract() const override { return isPlayerNear; }

		// Buy-prompt wording, kept here so the popup the scene shows stays in his voice.
		std::wstring GetBuyPromptTitle() const;
		std::wstring GetBuyPromptText(int cost) const;
		std::wstring GetBuyConfirmLabel(int cost) const;
	};
}
