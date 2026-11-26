#include "pch.h"
#include "SaveGameState.h"
#include "Game.h"
#include "Player.h"
#include "PlayerGlobalData.h"
#include "SketchyGuyGlobalData.h"
#include "ShopPurchaseGlobalData.h"
#include "BossDeathGlobalData.h"
#include "IntroScene.h"
#include "AssetManifest.h"
#include "LocalizationManager.h"
#include "TutorialWorldScene.h"
#include "LabInsideScene.h"
#include "SecretPuzzleScene.h"
#include "ThreadAlleyScene.h"
#include "BossWorldScene.h"
#include "QuestManager.h"
#include <fstream>

namespace Demo {
	SaveGameState::SaveGameState(Game* game, std::shared_ptr<sdlgf::SaveManager> saveManager)
		: game(game), saveManager(std::move(saveManager)) {
	}

	std::vector<LoadStep> SaveGameState::BuildSceneSteps() {
		// Each scene is constructed (and Init()ed) by SceneManager::PushScene; one per step so the
		// loading screen repaints in between. `self` keeps this state alive until the last step ran.
		auto self = shared_from_this();
		auto addScene = [self](const char* saveId, std::wstring label, std::function<sdlgf::IScene*()> make) {
			return LoadStep{ std::move(label), [self, saveId, make]() {
				auto sceneManager = self->game->GetSceneManager();
				sceneManager->PushScene(make());
				if (saveId != nullptr) self->sceneMap[saveId] = sceneManager->GetSceneCount() - 1;
			} };
		};
		auto app = sdlgf::Application::GetInstance();
		const int w = app->GetScreenWidth(), h = app->GetScreenHeight();
		Game* g = game;
		auto sm = saveManager;
		return {
			addScene(nullptr, Tr(L"Preparing intro"), [=]() -> sdlgf::IScene* { return new IntroScene(g, w, h); }),
			addScene("TutorialWorldScene", Tr(L"Building Cloud Canopy"), [=]() -> sdlgf::IScene* { return new TutorialWorldScene(g, sm, w, h); }),
			addScene("LabInsideScene", Tr(L"Building the lab"), [=]() -> sdlgf::IScene* { return new LabInsideScene(g, sm, w, h); }),
			addScene("SecretPuzzleScene", Tr(L"Building The Root"), [=]() -> sdlgf::IScene* { return new SecretPuzzleScene(g, sm, w, h); }),
			addScene("ThreadAlleyScene", Tr(L"Building Thread Alley"), [=]() -> sdlgf::IScene* { return new ThreadAlleyScene(g, sm, w, h); }),
			addScene("BossWorldScene", Tr(L"Building The Overflow"), [=]() -> sdlgf::IScene* { return new BossWorldScene(g, sm, w, h); }),
		};
	}

	void SaveGameState::RunLoading(std::vector<LoadStep> steps) {
		auto assets = ReadAssetManifest(WORLD_MANIFEST);
		auto app = sdlgf::Application::GetInstance();
		game->GetSceneManager()->BeginLoading(std::make_unique<LoadingScene>(
			game, app->GetScreenWidth(), app->GetScreenHeight(), std::move(assets), std::move(steps), nullptr));
	}

	void SaveGameState::BuildAllScenesNow() {
		ClearScenes();
		for (auto& step : BuildSceneSteps()) step.run();
	}

	void SaveGameState::ClearScenes()
	{
		while (game->GetSceneManager()->GetSceneCount() > 1) {
			game->GetSceneManager()->PopScene();
		}
	}

	int SaveGameState::GetSceneIndex(const std::string& saveId) const {
		auto it = sceneMap.find(saveId);
		return it != sceneMap.end() ? static_cast<int>(it->second) : -1;
	}

	const SaveGameState::SceneInfo* SaveGameState::GetSceneInfo(const std::string& saveId) {
		static const std::unordered_map<std::string, SceneInfo> infos = {
			{ "TutorialWorldScene", { L"Cloud Canopy", "bgm_tutorial",  0.5f, 5.0f } },
			{ "SecretPuzzleScene",  { L"The Root",     "bgm_secret",    0.3f, 1.5f } },
			{ "ThreadAlleyScene",   { L"Thread Alley", "bgm_arcade",    0.2f, 1.5f } },
			{ "BossWorldScene",     { L"The Overflow", "bgm_bossworld", 0.3f, 1.5f } },
		};
		auto it = infos.find(saveId);
		return it != infos.end() ? &it->second : nullptr;
	}

	std::shared_ptr<Player> SaveGameState::GetPlayerFromScene(sdlgf::IScene* scene) const {
		if (auto tutorialScene = dynamic_cast<Demo::TutorialWorldScene*>(scene)) {
			return tutorialScene->GetPlayer();
		}
		if (auto secretScene = dynamic_cast<Demo::SecretPuzzleScene*>(scene)) {
			return secretScene->GetPlayer();
		}
		if (auto threadScene = dynamic_cast<Demo::ThreadAlleyScene*>(scene)) {
			return threadScene->GetPlayer();
		}
		if (auto threadScene = dynamic_cast<Demo::BossWorldScene*>(scene)) {
			return threadScene->GetPlayer();
		}
		if (auto labInsideScene = dynamic_cast<Demo::LabInsideScene*>(scene)) {
			return labInsideScene->GetPlayer();
		}
		return nullptr;
	}

	std::string SaveGameState::GetSaveID() const {
		return "Game";
	}

	void SaveGameState::GenerateSaveData(nlohmann::json& outData) {
		auto sceneManager = game->GetSceneManager();
		auto currentScene = dynamic_cast<sdlgf::ISaveable*>(sceneManager->GetCurrentScene());
		if (currentScene) {
			outData["current_scene"] = currentScene->GetSaveID();
		}

		QuestManager::GetInstance()->GenerateSaveData(outData["quest_manager"]);

		for (size_t i = 0; i < sceneManager->GetSceneCount(); i++) {
			nlohmann::json data;
			auto scene = dynamic_cast<sdlgf::ISaveable*>(sceneManager->GetScene(i));
			if (scene) {
				scene->GenerateSaveData(data);
				outData["scene_data"][scene->GetSaveID()] = data;
			}
		}
	}

	void SaveGameState::RestoreSaveData(const nlohmann::json& inData) {
		auto sceneManager = game->GetSceneManager();
		if (inData.contains("current_scene")) {
			const std::string sceneId = inData["current_scene"].get<std::string>();
			auto sceneIt = sceneMap.find(sceneId);
			if (sceneIt != sceneMap.end()) {
				sceneManager->GoToScene(sceneIt->second);

				if (const SceneInfo* info = GetSceneInfo(sceneId)) {
					sdlgf::AudioManager::GetInstance()->PlayBGM_Fade(info->bgm, info->bgmVolume, info->bgmFadeSeconds);
				}
			}
		}
		if (inData.contains("quest_manager")) {
			QuestManager::GetInstance()->RestoreSaveData(inData["quest_manager"]);
		}
		if (inData.contains("scene_data")) {
			for (auto& [key, value] : inData["scene_data"].items()) {
				auto sceneIt = sceneMap.find(key);
				if (sceneIt != sceneMap.end()) {
					auto scene = dynamic_cast<sdlgf::ISaveable*>(sceneManager->GetScene(sceneIt->second));
					if (scene) {
						scene->RestoreSaveData(value);
					}
				}
			}
		}
	}

	std::shared_ptr<SaveGameState> SaveGameState::StartNewGame(Game* game, const std::shared_ptr<sdlgf::SaveManager>& saveManager) {
		saveManager->Clear();
		PlayerGlobalData::GetInstance()->Reset();
		SketchyGuyGlobalData::GetInstance()->Reset();
		ShopPurchaseGlobalData::GetInstance()->Reset();
		BossDeathGlobalData::GetInstance()->Reset();
		auto saveState = std::make_shared<SaveGameState>(game, saveManager);
		saveManager->Register(saveState.get());
		saveManager->Register(PlayerGlobalData::GetInstance());
		saveManager->Register(SketchyGuyGlobalData::GetInstance());
		saveManager->Register(ShopPurchaseGlobalData::GetInstance());
		saveManager->Register(BossDeathGlobalData::GetInstance());
		std::vector<LoadStep> steps;
		steps.push_back({ Tr(L"Clearing the stage"), [saveState]() { saveState->ClearScenes(); } });
		for (auto& step : saveState->BuildSceneSteps()) steps.push_back(std::move(step));
		steps.push_back({ Tr(L"Starting"), [saveState, game]() {
			QuestManager::GetInstance()->Reset();
			sdlgf::AudioManager::GetInstance()->StopAll();
			game->GetSceneManager()->GoToScene(1);
		} });
		saveState->RunLoading(std::move(steps));
		return saveState;
	}

	bool SaveGameState::HasSaveFile() {
		std::ifstream f(SAVE_FILE);
		return f.good();
	}

	void SaveGameState::ReloadFromDisk() {
		auto self = shared_from_this();
		std::vector<LoadStep> steps;
		steps.push_back({ Tr(L"Clearing the stage"), [self]() {
			// Park on the main menu first. ClearScenes pops everything above index 0, and the
			// current index may point at a scene that is about to be destroyed.
			self->game->GetSceneManager()->GoToScene(0);
			self->saveManager->Clear();
			self->saveManager->Register(self.get());
			self->saveManager->Register(PlayerGlobalData::GetInstance());
			self->saveManager->Register(SketchyGuyGlobalData::GetInstance());
			ShopPurchaseGlobalData::GetInstance()->Reset();
			self->saveManager->Register(ShopPurchaseGlobalData::GetInstance());
			self->saveManager->Register(BossDeathGlobalData::GetInstance());
			self->ClearScenes();
		} });
		for (auto& step : BuildSceneSteps()) steps.push_back(std::move(step));
		steps.push_back({ Tr(L"Restoring your save"), [self]() {
			QuestManager::GetInstance()->Reset();
			// RestoreSaveData moves the scene manager to the saved "current_scene".
			self->saveManager->Load(SAVE_FILE);
		} });
		RunLoading(std::move(steps));
	}

	void SaveGameState::ReloadLastSave() {
		ReloadFromDisk();
	}

	std::shared_ptr<SaveGameState> SaveGameState::LoadSavedGame(Game* game, const std::shared_ptr<sdlgf::SaveManager>& saveManager) {
		auto saveState = std::make_shared<SaveGameState>(game, saveManager);
		saveState->ReloadFromDisk();
		return saveState;
	}
}
