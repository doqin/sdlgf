#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "LoadingScene.h"

namespace Demo {
    class Game;
    class Player;

    class SaveGameState : public sdlgf::ISaveable, public std::enable_shared_from_this<SaveGameState> {
        Game* game;
        std::shared_ptr<sdlgf::SaveManager> saveManager;
        std::unordered_map<std::string, size_t> sceneMap;
		void ClearScenes();
        // One step per scene, so a loading screen can repaint between the heavy constructions.
        std::vector<LoadStep> BuildSceneSteps();
        // Hands `steps` (plus the world asset manifest) to a loading screen that replaces the
        // scene stack on screen until everything is built.
        void RunLoading(std::vector<LoadStep> steps);
        // Shared by LoadSavedGame and ReloadLastSave so the load sequence lives in one place.
        void ReloadFromDisk();
    public:
        static constexpr const char* SAVE_FILE = "savegame.json";
        // Textures the world scenes use, streamed in by the loading screen before they are built.
        static constexpr const char* WORLD_MANIFEST = "assets/manifests/world.txt";
        /// Builds every world scene right now, without a loading screen (used to record the
        /// asset manifest and by tests). The scene stack is replaced.
        void BuildAllScenesNow();
        static bool HasSaveFile();
        std::shared_ptr<Player> GetPlayerFromScene(sdlgf::IScene* scene) const;
        int GetSceneIndex(const std::string& saveId) const;

        // Per-world presentation data, shared by loading a save and fast travelling.
        struct SceneInfo {
            const wchar_t* displayName;
            const char* bgm;
            float bgmVolume;
            float bgmFadeSeconds;
        };
        // Returns nullptr for scenes that have no info (intro, lab interior).
        static const SceneInfo* GetSceneInfo(const std::string& saveId);
        SaveGameState(Game* game, std::shared_ptr<sdlgf::SaveManager> saveManager);
        std::string GetSaveID() const override;
        void GenerateSaveData(nlohmann::json& outData) override;
        void RestoreSaveData(const nlohmann::json& inData) override;
        void ReloadLastSave();
        static std::shared_ptr<SaveGameState> StartNewGame(Game* game, const std::shared_ptr<sdlgf::SaveManager>& saveManager);
        static std::shared_ptr<SaveGameState> LoadSavedGame(Game* game, const std::shared_ptr<sdlgf::SaveManager>& saveManager);
    };
}
