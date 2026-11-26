#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Game.h"
#include "CardCatalog.h"
#include "CardFrame.h"
#include <string>
#include <vector>

namespace Demo {
	// Overlay scene that plays the card-pack purchase spectacle: the pack-opening animation, an
	// explosion particle burst, then the pulled cards revealed one at a time (click to fade in the
	// next). The cards are already in the player's inventory before this scene is pushed, so the
	// reveal is pure presentation - quitting mid-reveal loses nothing.
	class PackOpeningScene : public sdlgf::IScene {
	public:
		PackOpeningScene(Game* game, int screenWidth, int screenHeight, std::vector<std::string> cardIds);

		bool IsOverlay() const override { return true; }
		void Init() override;
		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;

	private:
		enum class State { Opening, Revealing, Done };

		Game* game;
		std::vector<std::string> cardIds;

		std::shared_ptr<sdlgf::TransformManager> transformManager;
		std::shared_ptr<sdlgf::Texture> packTex;
		std::shared_ptr<sdlgf::Texture> particleTex;
		std::shared_ptr<sdlgf::AnimatedSprite> packSprite;
		std::shared_ptr<CardFrame> cardFrame;
		std::shared_ptr<sdlgf::Font> font;
		std::shared_ptr<sdlgf::FontSprite> fontSprite;
		std::unique_ptr<sdlgf::ParticleSystem> explosionEmitter;

		// What CardFrame needs to draw each revealed card, in the same order as cardIds.
		struct CardFace {
			bool valid = false;
			CardTemplate cardTemplate = CardTemplate::Red;
			std::wstring name;
			std::wstring inputs;
			size_t cost = 0;
		};
		std::vector<CardFace> faces;
		std::vector<CardCatalog::Rarity> rarities;

		State state = State::Opening;
		int revealIndex = 0;
		float cardAlpha = 0.f;
		float holdTimer = 0.f;      // brief pause between the burst and the first card
		bool burstFired = false;
		bool pendingCardSound = false; // play the card-draw sfx when the next card starts appearing
		bool shouldLeave = false;
	};
}
