#include "pch.h"
#include "LoadingScene.h"
#include "MainFont.h"
#include "LocalizationManager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Demo {
	namespace {
		constexpr unsigned long long kUploadBudgetMs = 4;
		constexpr float kLabelFadeMs = 300.0f;

		// Developer knob: SDLGF_LOADING_STEP_MS=<ms> pauses that long before every step (and before
		// finishing), so the loading screen can be looked at even when everything is cached.
		float ArtificialStepDelayMs() {
			static const float delay = [] {
				const char* value = std::getenv("SDLGF_LOADING_STEP_MS");
				return value ? static_cast<float>((std::max)(0, std::atoi(value))) : 0.0f;
			}();
			return delay;
		}
		// Same colour as TransitionCommand's pillars, so the screen stays seamless around loading
		constexpr sdlgf::Color kBackground = 0xFF000000;
		constexpr float kMargin = 28.0f;       // throbber centre's distance from the screen corner
		constexpr float kThrobberRadius = 8.0f;
		constexpr float kThrobberDot = 4.0f;
		constexpr int kThrobberDots = 8;
		constexpr float kThrobberSpeed = 10.0f; // steps per second
		constexpr float kLabelGap = 18.0f;     // between the text and the throbber
	}

	LoadingScene::LoadingScene(Game* game, int screenW, int screenH, std::vector<std::wstring> assets,
		std::vector<LoadStep> steps, std::function<void()> onDone)
		: IScene(screenW, screenH), game(game), assets(std::move(assets)), steps(std::move(steps)), onDone(std::move(onDone)) {
	}

	void LoadingScene::Init() {
		auto gd = game->GetGraphicsDevice();
		font = gd->GetAssets().GetFont(kMainFontName, kMainFontSize);
		text = std::make_shared<sdlgf::FontSprite>(font);
		loader = std::make_unique<sdlgf::AssetLoader>(gd->GetAssets());
		loader->Request(assets);
		waitMs = ArtificialStepDelayMs();
		statusLabel = Tr(L"Loading");
	}

	void LoadingScene::Update(unsigned long long deltaTime) {
		elapsedSeconds += static_cast<float>(deltaTime) / 1000.0f;
		camera.Update();

		// Fade the step text in again whenever it changes
		if (statusLabel != shownLabel) {
			shownLabel = statusLabel;
			labelFade = 0.0f;
		}
		labelFade = (std::min)(1.0f, labelFade + static_cast<float>(deltaTime) / kLabelFadeMs);
		throbberFade = (std::min)(1.0f, throbberFade + static_cast<float>(deltaTime) / kLabelFadeMs);

		if (waitMs > 0.0f) {
			waitMs -= static_cast<float>(deltaTime);
			return;
		}
		if (!drawnSinceLastWork) return; // show the current state before doing more work
		drawnSinceLastWork = false;

		if (!loader->Pump(kUploadBudgetMs)) {
			statusLabel = Tr(L"Loading assets");
			return;
		}
		if (nextStep < steps.size()) {
			const LoadStep& step = steps[nextStep];
			statusLabel = step.label;
			++nextStep;
			step.run();
			waitMs = ArtificialStepDelayMs();
			return;
		}
		// Everything is built. Release the loader's references only after the objects that needed the
		// textures hold their own.
		for (const auto& failure : loader->GetFailures()) sdlgf::Platform::DebugLog("Loading screen: " + failure + "\n");
		sdlgf::Platform::DebugLog("Loading finished in " + std::to_string(static_cast<int>(elapsedSeconds * 1000.0f)) + " ms ("
			+ std::to_string(loader->GetCompleted()) + " textures preloaded, " + std::to_string(steps.size()) + " steps)\n");
		loader.reset();
		if (onDone) onDone();
		game->GetSceneManager()->EndLoading();
	}

	void LoadingScene::DrawWorld(unsigned long long deltaTime) {
		auto gd = game->GetGraphicsDevice();
		if (!gd->BeginDraw()) return;

		const float w = static_cast<float>(game->GetVirtualWidth());
		const float h = static_cast<float>(game->GetVirtualHeight());
		gd->DrawRectangle(camera, -w / 2.0f, -h / 2.0f, w, h, kBackground, true);
		gd->SetAlphaBlending(true);

		// White throbber in the bottom-right corner: a ring of squares with a fading trail
		const float cx = w / 2.0f - kMargin, cy = h / 2.0f - kMargin;
		const int head = static_cast<int>(elapsedSeconds * kThrobberSpeed) % kThrobberDots;
		for (int i = 0; i < kThrobberDots; ++i) {
			const float angle = 6.2831853f * static_cast<float>(i) / kThrobberDots - 1.5707963f;
			const int behind = (head - i + kThrobberDots) % kThrobberDots; // 0 = the leading square
			const float brightness = 1.0f - static_cast<float>(behind) / kThrobberDots;
			const int alpha = static_cast<int>(255.0f * brightness * throbberFade);
			const float x = std::round(cx + std::cos(angle) * kThrobberRadius - kThrobberDot / 2.0f);
			const float y = std::round(cy + std::sin(angle) * kThrobberRadius - kThrobberDot / 2.0f);
			gd->DrawRectangle(camera, x, y, kThrobberDot, kThrobberDot, COLOR_ARGB(alpha, 255, 255, 255), true);
		}

		// Step text to the left of the throbber, vertically centred on it
		if (!statusLabel.empty()) {
			text->SetText(statusLabel);
			text->SetColor(COLOR_ARGB(static_cast<int>(255.0f * labelFade), 255, 255, 255));
			const float tw = static_cast<float>(text->GetWidth()), th = static_cast<float>(text->GetHeight());
			text->SetPosition(std::round(cx - kThrobberRadius - kThrobberDot - kLabelGap - tw), std::round(cy - th / 2.0f));
			text->Begin(); text->Draw(camera, deltaTime); text->End();
		}
		gd->SetAlphaBlending(false);
		gd->EndDraw();
	}

	void LoadingScene::DrawUI(unsigned long long deltaTime) {
		drawnSinceLastWork = true;
	}
}
