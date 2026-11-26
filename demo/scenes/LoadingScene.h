#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "IScene.h"
#include "Sprites.h"
#include "AssetLoader.h"
#include "Game.h"

namespace Demo {
	/// One piece of main-thread work a loading screen performs between two drawn frames
	/// (e.g. constructing one scene). Keep steps small: the screen cannot repaint while one runs.
	struct LoadStep {
		std::wstring label;
		std::function<void()> run;
	};

	/// A full-screen loading scene shown by SceneManager::BeginLoading, outside the scene stack.
	///
	/// Phase 1 streams `assets` into the AssetManager (decoding on a worker thread, uploading a few
	/// ms per frame), phase 2 runs `steps` one per frame, then `onDone` runs once and the scene ends
	/// itself. Every step is preceded by at least one drawn frame, so progress is always on screen
	/// before a blocking step starts.
	class LoadingScene : public sdlgf::IScene {
	private:
		Game* game;
		std::vector<std::wstring> assets;
		std::vector<LoadStep> steps;
		std::function<void()> onDone;

		std::unique_ptr<sdlgf::AssetLoader> loader;
		std::shared_ptr<sdlgf::Font> font;
		std::shared_ptr<sdlgf::FontSprite> text;
		size_t nextStep = 0;
		bool drawnSinceLastWork = false;
		float elapsedSeconds = 0.0f;
		float waitMs = 0.0f;        // artificial pause left before the next piece of work
		std::wstring statusLabel;   // what is being done now
		std::wstring shownLabel;    // what the fade-in currently applies to
		float labelFade = 0.0f;     // 0..1 opacity of the step text
		float throbberFade = 0.0f;  // 0..1 opacity of the throbber
	public:
		LoadingScene(Game* game, int screenW, int screenH, std::vector<std::wstring> assets,
			std::vector<LoadStep> steps, std::function<void()> onDone);
		void Init() override;
		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;
	};
}
