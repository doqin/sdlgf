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
#include "WorldSceneBase.h"
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
		auto app = sdlgf::Application::GetInstance();
		const int w = app->GetScreenWidth(), h = app->GetScreenHeight();
		auto sm = this->saveManager;
		auto add = [&](const char* id, std::function<WorldSceneBase*()> make) {
			WorldSlot slot;
			slot.id = id;
			slot.make = std::move(make);
			worlds.push_back(std::move(slot));
		};
		// World order is also the order of the fast travel tabs
		add("TutorialWorldScene", [=]() -> WorldSceneBase* { return new TutorialWorldScene(game, sm, w, h); });
		add("LabInsideScene", [=]() -> WorldSceneBase* { return new LabInsideScene(game, sm, w, h); });
		add("SecretPuzzleScene", [=]() -> WorldSceneBase* { return new SecretPuzzleScene(game, sm, w, h); });
		add("ThreadAlleyScene", [=]() -> WorldSceneBase* { return new ThreadAlleyScene(game, sm, w, h); });
		add("BossWorldScene", [=]() -> WorldSceneBase* { return new BossWorldScene(game, sm, w, h); });
	}

	SaveGameState::WorldSlot* SaveGameState::FindWorld(const std::string& id) {
		for (auto& slot : worlds) if (slot.id == id) return &slot;
		return nullptr;
	}

	WorldSceneBase* SaveGameState::ResidentWorld() const {
		auto sceneManager = game->GetSceneManager();
		for (size_t i = 0; i < sceneManager->GetSceneCount(); ++i) {
			if (auto* world = dynamic_cast<WorldSceneBase*>(sceneManager->GetScene(i))) return world;
		}
		return nullptr;
	}

	void SaveGameState::ClearScenes()
	{
		while (game->GetSceneManager()->GetSceneCount() > 1) {
			game->GetSceneManager()->PopScene();
		}
		for (auto& slot : worlds) {
			slot.state = nlohmann::json();
			slot.hasState = false;
			slot.overview.reset();
		}
		pendingWorldId.clear();
	}

	WorldSceneBase* SaveGameState::SpawnWorld(const std::string& id) {
		auto* slot = FindWorld(id);
		if (slot == nullptr) throw std::runtime_error("Unknown world scene: " + id);
		WorldSceneBase* scene = slot->make();
		game->GetSceneManager()->PushScene(scene); // Init()s it
		return scene;
	}

	void SaveGameState::ActivateWorld(const std::string& id, WorldSceneBase* scene) {
		auto* slot = FindWorld(id);
		if (slot != nullptr && slot->hasState) {
			static_cast<sdlgf::ISaveable*>(scene)->RestoreSaveData(slot->state);
		}
		auto sceneManager = game->GetSceneManager();
		sceneManager->GoToScene(sceneManager->GetSceneCount() - 1);
	}

	void SaveGameState::UnloadResidentWorld() {
		auto sceneManager = game->GetSceneManager();
		for (size_t i = 0; i < sceneManager->GetSceneCount(); ++i) {
			auto* world = dynamic_cast<WorldSceneBase*>(sceneManager->GetScene(i));
			if (world == nullptr) continue;
			if (auto* slot = FindWorld(world->GetSaveID())) {
				slot->state = nlohmann::json::object();
				static_cast<sdlgf::ISaveable*>(world)->GenerateSaveData(slot->state);
				slot->hasState = true;
				slot->overview = world->CaptureOverview(false);
			}
			// Park on the main menu: the current index may be the scene about to be destroyed
			sceneManager->GoToScene(0);
			sceneManager->RemoveScene(i);
			return;
		}
	}

	void SaveGameState::RunLoading(std::vector<LoadStep> steps, const std::string& manifestWorldId, std::function<void()> onDone) {
		std::vector<std::wstring> assets;
		if (!manifestWorldId.empty()) assets = ReadAssetManifest(ManifestPath(manifestWorldId));
		auto app = sdlgf::Application::GetInstance();
		game->GetSceneManager()->BeginLoading(std::make_unique<LoadingScene>(
			game, app->GetScreenWidth(), app->GetScreenHeight(), std::move(assets), std::move(steps), std::move(onDone)));
	}

	bool SaveGameState::TravelTo(const std::string& sceneId, const Arrival& arrival) {
		auto* resident = ResidentWorld();
		if (resident != nullptr && resident->GetSaveID() == sceneId) {
			// Same world (fast travel between its own save points): nothing to load
			if (arrival.position) resident->Arrive(arrival.position->first, arrival.position->second, arrival.fastTravel);
			if (arrival.bgm) sdlgf::AudioManager::GetInstance()->PlayBGM_Fade(arrival.bgm, arrival.bgmVolume, 1.5f);
			return false;
		}
		if (FindWorld(sceneId) == nullptr) throw std::runtime_error("Unknown world scene: " + sceneId);

		auto self = shared_from_this();
		auto built = std::make_shared<WorldSceneBase*>(nullptr);
		std::vector<LoadStep> steps;
		if (resident != nullptr) {
			steps.push_back({ Tr(L"Leaving the area"), [self]() { self->UnloadResidentWorld(); } });
		}
		steps.push_back({ Tr(L"Building the world"), [self, sceneId, built]() { *built = self->SpawnWorld(sceneId); } });
		steps.push_back({ Tr(L"Arriving"), [self, sceneId, built, arrival]() {
			self->ActivateWorld(sceneId, *built);
			if (arrival.position) (*built)->Arrive(arrival.position->first, arrival.position->second, arrival.fastTravel);
			if (arrival.bgm) sdlgf::AudioManager::GetInstance()->PlayBGM_Fade(arrival.bgm, arrival.bgmVolume, 1.5f);
		} });
		RunLoading(std::move(steps), sceneId);
		return true;
	}

	void SaveGameState::EnsureWorldOverviews(std::function<void()> onReady) {
		auto* resident = ResidentWorld();
		const std::string residentId = resident ? resident->GetSaveID() : std::string();
		auto self = shared_from_this();
		std::vector<LoadStep> steps;
		for (auto& slot : worlds) {
			// Only worlds with fast travel info can be on the map; the live one is captured fresh
			if (slot.id == residentId || slot.overview || GetSceneInfo(slot.id) == nullptr) continue;
			const std::string id = slot.id;
			steps.push_back({ Tr(L"Mapping the world"), [self, id]() {
				auto* slot = self->FindWorld(id);
				// Build it like a normal visit (not on the stack) just to read its map, then drop it.
				std::unique_ptr<WorldSceneBase> scene(slot->make());
				scene->Init();
				if (slot->hasState) static_cast<sdlgf::ISaveable*>(scene.get())->RestoreSaveData(slot->state);
				slot->overview = scene->CaptureOverview(false);
				scene.reset();
				// Building it pointed the shared UI services at the scene that was just destroyed
				self->RebindUiServices();
			} });
		}
		if (steps.empty()) {
			onReady();
			return;
		}
		RunLoading(std::move(steps), "", std::move(onReady));
	}

	void SaveGameState::RebindUiServices() {
		if (auto* live = ResidentWorld()) live->RebindUiServices();
	}

	std::vector<std::shared_ptr<WorldOverview>> SaveGameState::CollectWorldOverviews(const WorldSceneBase& current) {
		std::vector<std::shared_ptr<WorldOverview>> result;
		const std::string currentId = current.GetSaveID();
		for (auto& slot : worlds) {
			if (slot.id == currentId) result.push_back(current.CaptureOverview(true));
			else if (slot.overview) result.push_back(slot.overview);
		}
		return result;
	}

	bool SaveGameState::RecordWorldManifests(const std::string& dir) {
		auto& assets = game->GetGraphicsDevice()->GetAssets();
		bool ok = true;
		for (auto& slot : worlds) {
			assets.StartRecording();
			{
				std::unique_ptr<WorldSceneBase> scene(slot.make());
				scene->Init();
			}
			const auto paths = assets.StopRecording();
			std::printf("%s: %zu textures\n", slot.id.c_str(), paths.size());
			ok = WriteAssetManifest(dir + "/" + slot.id + ".txt", paths) && ok;
		}
		return ok;
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

	std::string SaveGameState::GetSaveID() const {
		return "Game";
	}

	void SaveGameState::GenerateSaveData(nlohmann::json& outData) {
		auto* resident = ResidentWorld();
		if (resident) outData["current_scene"] = resident->GetSaveID();

		QuestManager::GetInstance()->GenerateSaveData(outData["quest_manager"]);

		for (auto& slot : worlds) {
			if (resident && resident->GetSaveID() == slot.id) {
				nlohmann::json data = nlohmann::json::object();
				static_cast<sdlgf::ISaveable*>(resident)->GenerateSaveData(data);
				outData["scene_data"][slot.id] = data;
			}
			else if (slot.hasState) {
				outData["scene_data"][slot.id] = slot.state;
			}
		}
	}

	void SaveGameState::RestoreSaveData(const nlohmann::json& inData) {
		if (inData.contains("scene_data")) {
			for (auto& [key, value] : inData["scene_data"].items()) {
				if (auto* slot = FindWorld(key)) {
					slot->state = value;
					slot->hasState = true;
				}
			}
		}
		if (inData.contains("quest_manager")) {
			QuestManager::GetInstance()->RestoreSaveData(inData["quest_manager"]);
		}
		// The saved world is built by the loading step that follows, after every saveable (including
		// the global data) has been restored.
		pendingWorldId = inData.value("current_scene", std::string(DEFAULT_WORLD));
		if (FindWorld(pendingWorldId) == nullptr) pendingWorldId = DEFAULT_WORLD;
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
		// The first world is built when the intro ends (IntroScene), behind its own loading screen
		steps.push_back({ Tr(L"Starting"), [saveState, game]() {
			auto app = sdlgf::Application::GetInstance();
			game->GetSceneManager()->PushScene(new IntroScene(game, app->GetScreenWidth(), app->GetScreenHeight()));
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

	std::string SaveGameState::PeekSavedWorldId() {
		try {
			std::ifstream f(SAVE_FILE);
			nlohmann::json save = nlohmann::json::parse(f);
			return save.at("Game").value("current_scene", std::string(DEFAULT_WORLD));
		}
		catch (const std::exception&) {
			return DEFAULT_WORLD;
		}
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
		steps.push_back({ Tr(L"Reading your save"), [self]() {
			QuestManager::GetInstance()->Reset();
			self->saveManager->Load(SAVE_FILE); // RestoreSaveData picks the world to build
		} });
		steps.push_back({ Tr(L"Building the world"), [self]() {
			const std::string id = self->pendingWorldId.empty() ? std::string(DEFAULT_WORLD) : self->pendingWorldId;
			self->pendingWorldId.clear();
			self->SpawnWorld(id);
			self->ActivateWorld(id, self->ResidentWorld());
			if (const SceneInfo* info = GetSceneInfo(id)) {
				sdlgf::AudioManager::GetInstance()->PlayBGM_Fade(info->bgm, info->bgmVolume, info->bgmFadeSeconds);
			}
		} });
		RunLoading(std::move(steps), PeekSavedWorldId());
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
