#include "pch.h"
#include "LoadingScene.h"
#include "MainFont.h"
#include "LocalizationManager.h"
#include <algorithm>

namespace Demo {
	namespace {
		constexpr unsigned long long kUploadBudgetMs = 4;
		constexpr float kAssetsShare = 0.6f; // the rest of the bar is the steps
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
	}

	float LoadingScene::TargetProgress() const {
		const float stepsDone = steps.empty() ? 1.0f : static_cast<float>(nextStep) / static_cast<float>(steps.size());
		if (assets.empty()) return stepsDone;
		return kAssetsShare * loader->GetProgress() + (1.0f - kAssetsShare) * stepsDone;
	}

	void LoadingScene::Update(unsigned long long deltaTime) {
		elapsedSeconds += static_cast<float>(deltaTime) / 1000.0f;
		camera.Update();

		// Ease the bar toward the real progress so a big step does not make it teleport
		const float target = TargetProgress();
		shownProgress += (target - shownProgress) * (std::min)(1.0f, static_cast<float>(deltaTime) / 120.0f);

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
			return;
		}
		// Everything is built. Release the loader's references only after the objects that needed the
		// textures hold their own.
		for (const auto& failure : loader->GetFailures()) sdlgf::Platform::DebugLog("Loading screen: " + failure + "\n");
		sdlgf::Platform::DebugLog("Loading finished in " + std::to_string(static_cast<int>(elapsedSeconds * 1000.0f)) + " ms ("
			+ std::to_string(loader->GetCompleted()) + " textures preloaded, " + std::to_string(steps.size()) + " steps)\n");
		loader.reset();
		shownProgress = 1.0f;
		if (onDone) onDone();
		game->GetSceneManager()->EndLoading();
	}

	void LoadingScene::DrawWorld(unsigned long long deltaTime) {
		auto gd = game->GetGraphicsDevice();
		if (!gd->BeginDraw()) return;

		const float w = static_cast<float>(game->GetVirtualWidth());
		const float h = static_cast<float>(game->GetVirtualHeight());
		gd->DrawRectangle(camera, -w / 2.0f, -h / 2.0f, w, h, 0xFF0B0F14, true);

		// Progress bar
		const float barW = w * 0.5f, barH = 10.0f;
		const float barX = -barW / 2.0f, barY = h * 0.18f;
		gd->DrawRectangle(camera, barX - 2.0f, barY - 2.0f, barW + 4.0f, barH + 4.0f, 0xFF3A4654, false);
		const float fill = std::clamp(shownProgress, 0.0f, 1.0f) * barW;
		if (fill > 0.0f) gd->DrawRectangle(camera, barX, barY, fill, barH, 0xFF00FF41, true);

		// "Loading..." with animated dots, centered above the bar
		gd->SetAlphaBlending(true);
		const int dots = static_cast<int>(elapsedSeconds * 3.0f) % 4;
		text->SetText(Tr(L"Loading") + std::wstring(dots, L'.'));
		text->SetColor(0xFFFFFFFF);
		text->SetPosition(-static_cast<float>(text->GetWidth()) / 2.0f, barY - 40.0f);
		text->Begin(); text->Draw(camera, deltaTime); text->End();

		if (!statusLabel.empty()) {
			text->SetText(statusLabel);
			text->SetColor(0xFF8A97A6);
			text->SetPosition(-static_cast<float>(text->GetWidth()) / 2.0f, barY + barH + 12.0f);
			text->Begin(); text->Draw(camera, deltaTime); text->End();
		}
		gd->SetAlphaBlending(false);
		gd->EndDraw();
	}

	void LoadingScene::DrawUI(unsigned long long deltaTime) {
		drawnSinceLastWork = true;
	}
}
