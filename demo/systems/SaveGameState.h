#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "LoadingScene.h"

namespace Demo {
    class Game;
    class WorldSceneBase;
    struct WorldOverview;

    /// Owns the progress of the whole game and which world scene is in memory.
    ///
    /// Only ONE world scene exists at a time (the scene stack is [main menu, (intro), world] plus
    /// overlays like battles). Leaving a world snapshots its save data into a cache and destroys it;
    /// entering one builds it again and restores that snapshot, exactly like loading a save. Saving
    /// writes the live world plus the cached snapshots of the others.
    class SaveGameState : public sdlgf::ISaveable, public std::enable_shared_from_this<SaveGameState> {
        struct WorldSlot {
            std::string id;
            std::function<WorldSceneBase*()> make;     // builds a fresh scene (not yet Init()ed)
            nlohmann::json state;                      // save data from when it was last unloaded / loaded
            bool hasState = false;
            std::shared_ptr<WorldOverview> overview;   // for the fast travel map while unloaded
        };

        Game* game;
        std::shared_ptr<sdlgf::SaveManager> saveManager;
        std::vector<WorldSlot> worlds;
        std::string pendingWorldId;   // set by RestoreSaveData, built by the loading step that follows

        WorldSlot* FindWorld(const std::string& id);
        WorldSceneBase* ResidentWorld() const;
        // Pops everything but the main menu and forgets every cached world.
        void ClearScenes();
        // Builds the world scene `id` and puts it on the stack (without switching to it).
        WorldSceneBase* SpawnWorld(const std::string& id);
        // Restores the cached state into a freshly spawned world and switches to it.
        void ActivateWorld(const std::string& id, WorldSceneBase* scene);
        // Caches the resident world's save data and overview, then destroys it.
        void UnloadResidentWorld();
        // Hands `steps` to a loading screen that replaces the stack on screen. Textures listed in the
        // manifest of world `manifestWorldId` (if any) are streamed in first.
        void RunLoading(std::vector<LoadStep> steps, const std::string& manifestWorldId = "", std::function<void()> onDone = nullptr);
        // Shared by LoadSavedGame and ReloadLastSave so the load sequence lives in one place.
        void ReloadFromDisk();
        static std::string PeekSavedWorldId();
    public:
        static constexpr const char* SAVE_FILE = "savegame.json";
        static constexpr const char* DEFAULT_WORLD = "TutorialWorldScene";
        // Textures a world uses, streamed in by the loading screen before it is built.
        static std::string ManifestPath(const std::string& worldId) { return "assets/manifests/" + worldId + ".txt"; }
        static bool HasSaveFile();

        // Where and how the player arrives in a world.
        struct Arrival {
            std::optional<std::pair<float, float>> position; // none = wherever the world's save data says
            const char* bgm = nullptr;                       // a string literal; null keeps the current music
            float bgmVolume = 0.3f;
            bool fastTravel = false;
        };
        /// Moves the player to world `sceneId`: unloads the current world and builds the target behind a
        /// loading screen. Returns true if the current scene is going away; false if the target is the
        /// current world and the player was simply moved.
        bool TravelTo(const std::string& sceneId, const Arrival& arrival);

        /// The fast travel map lists worlds that are not in memory, so each visited world needs an
        /// overview. Worlds without one yet are built briefly behind the loading screen; `onReady`
        /// runs afterwards (immediately if nothing is missing).
        void EnsureWorldOverviews(std::function<void()> onReady);
        /// One overview per world that has fast travel info, in world order; `current` (the live
        /// scene) is captured fresh, the rest come from the cache.
        std::vector<std::shared_ptr<WorldOverview>> CollectWorldOverviews(const WorldSceneBase& current);

        /// Developer tool: builds every world once while logging the textures it requests and writes
        /// `<dir>/<world id>.txt` for each (the loading screen's manifests). Returns false on write failure.
        bool RecordWorldManifests(const std::string& dir);

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
