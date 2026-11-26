#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "Game.h"
#include "Player.h"
#include "SavePoint.h"
#include "InventoryMenu.h"
#include "ShopPoint.h"
#include "HealingPoint.h"
#include "NPC.h"
#include "IConversation.h"
#include "TreasureChestNPC.h"
#include "PopUpMessage.h"
#include "PlayerHUD.h"
#include "MapEnemy.h"
#include "ChapterTitleUI.h"
#include "MapView.h"
#include "WorldOverview.h"
#include "TextIconButton.h"

namespace Demo {
	class WorldSceneBase : public sdlgf::IScene, public sdlgf::ISaveable {
	protected:
		bool isGamePaused = false;
		bool isTransitioning = false;
		bool hasSeenChapterIntro = false;

		// Minimap / full map. Portal layers must be registered per scene (RegisterPortalLayer)
		// because not every trigger_* layer is a portal (battle triggers, decoys, secrets).
		MapView mapView;
		std::vector<std::string> portalLayers;
		bool fullMapOpen = false;

		// Fast travel map: the full map with a tab per visited world, opened from a save point.
		// Worlds count as visited once one of their save points has been unlocked.
		bool travelMapOpen = false;
		int travelTab = 0;
		float travelInputGraceMs = 0.f;  // ignores clicks right after opening, so the popup click can't leak through
		// One overview per tab: this world is live, the others come from SaveGameState's cache because
		// only one world scene is in memory at a time.
		std::vector<std::shared_ptr<WorldOverview>> travelWorlds;
		MapView::TravelHits travelHits;
		std::string travelHoverId;  // save point highlighted by mouse hover or keyboard selection
		int travelHoverTab = -1;
		// Makes sure every visited world has an overview (briefly building the ones that have none yet
		// behind the loading screen), then opens the map.
		void OpenTravelMap();
		void OpenTravelMapNow();
		void CloseFullMap();
		void UpdateTravelMap(unsigned long long deltaTime);
		void ConfirmTravel(const WorldOverview& world, const WorldOverview::SavePointInfo& target);

		Game* game;
		std::shared_ptr<sdlgf::ColliderManager> colliderManager;
		std::shared_ptr<sdlgf::TransformManager> transformManager;
		std::shared_ptr<Demo::DraggableManager> draggableManager;
		std::shared_ptr<InventoryMenu> inventoryMenu;
		std::shared_ptr<PlayerHUD> playerHUD;
		std::shared_ptr<sdlgf::SaveManager> saveManager;

		std::vector<std::shared_ptr<SavePoint>> savePoints;
		std::vector<std::shared_ptr<ShopPoint>> shopPoints;
		std::vector<std::shared_ptr<HealingPoint>> healingPoints;
		std::shared_ptr<sdlgf::Font> font;
		std::shared_ptr<sdlgf::Texture> gearTex;
		// Next page of the first-gear tutorial to show (-1 = none pending); see WorldSceneBase::Update.
		int gearTutorialPage = -1;
		std::shared_ptr<Player> player;
		std::shared_ptr<sdlgf::Map> map;
		std::shared_ptr<sdlgf::CommandBuffer> drawBuffer;
		std::shared_ptr<sdlgf::CommandBuffer> commandBuffer;
		std::shared_ptr<PopUpMessage> popUpMessage;
		std::wstring chapterTitle = L"Unnamed";
		std::wstring chapterSubtitle = L"< No Subtitle >";
		std::shared_ptr<ChapterTitleUI> chapterTitleUI;
		std::shared_ptr<IConversation> currentConversation;
		std::shared_ptr<INPC> activeNPC = nullptr;
		std::vector<std::shared_ptr<NPC>> mapNPCs;
		std::vector<std::shared_ptr<TreasureChestNPC>> treasureChests;
		std::vector<std::shared_ptr<MapEnemy>> mapEnemies;
		std::function<void()> onConversationEnd;

		WorldSceneBase(Game* game, std::shared_ptr<sdlgf::SaveManager> sm, UINT sw, UINT sh);

		void GenerateSaveData(nlohmann::json& outData) override;
		void RestoreSaveData(const nlohmann::json& inData) override;

		void OpenChestWithDialog(std::shared_ptr<TreasureChestNPC>& chest);

		void SetChapterTitle(const std::wstring& title, const std::wstring& subtitle);

		void RegisterPortalLayer(const std::string& layerName) { portalLayers.push_back(layerName); }

		struct DepthNode {
			float y;
			std::function<void()> drawCall;
			bool operator<(const DepthNode& other) const { return y < other.y; }
		};

		void AddDepthNode(std::vector<DepthNode>& nodes, float y, std::function<void()> drawCall);

		// Fades out, then loads the world `sceneId` (see SaveGameState::TravelTo) and puts the player at
		// (targetX, targetY) in it. Only one world scene exists at a time: this one is unloaded.
		void CreatePortalTransition(const char* sceneId, float targetX, float targetY, const char* bgm = nullptr, float bgmVol = 0.3f);
		void TransitionToScene(const std::string& sceneId, float targetX, float targetY, const char* bgm = nullptr, float bgmVol = 0.3f, bool fastTravel = false);
		// Called on the destination scene once a fast travel has placed the player, so scenes with
		// per-area state (e.g. the boss world's island) can match it to where the player landed.
		virtual void OnFastTravelArrive(float x, float y) {}

		void SpawnMapEnemy(float x, float y, std::string id, std::vector<std::string> types,
			bool isRand, bool isGlobal, std::function<void(sdlgf::GraphicsDevice*, unsigned long long)> bgDraw,
			int tokenChance = 30, const std::string& questEvent = "", const std::string& questId = "");

		virtual void OnInit() = 0;
		virtual void OnUpdate(unsigned long long deltaTime) {}
		// Hook: a subclass-owned modal (e.g. a terminal UI) that should freeze the
		// world and suppress the shared inventory / interaction input while it is up.
		virtual bool IsSubsceneModalActive() const { return false; }
		// Hook: scenes where the corner minimap makes no sense (e.g. a single small room) return false.
		virtual bool ShowMiniMap() const { return true; }
		virtual void OnDrawWorld(std::vector<DepthNode>& depthNodes, unsigned long long deltaTime) {}
		virtual void OnDrawUI(unsigned long long deltaTime) {}
		virtual void OnGenerateSaveData(nlohmann::json& outData) {}
		virtual void OnRestoreSaveData(const nlohmann::json& inData) {}
		virtual void DrawBackground(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime) = 0;

	public:
		void Init() override;
		void InitCore(float playerX, float playerY, const wchar_t* mapFile);

		void Update(unsigned long long deltaTime) override;
		void DrawWorld(unsigned long long deltaTime) override;
		void DrawUI(unsigned long long deltaTime) override;
		std::shared_ptr<Player> GetPlayer() const { return player; }
		// Points the shared UI services (popups, quest panel) at this scene again; needed after another
		// world scene was built briefly (SaveGameState::EnsureWorldOverviews) and took them over.
		void RebindUiServices();
		// Puts the player at (x, y) after a portal or fast travel (called by SaveGameState once built).
		void Arrive(float x, float y, bool fastTravel) {
			player->SetLocalPosition(x, y);
			if (fastTravel) OnFastTravelArrive(x, y);
		}
		const std::vector<std::shared_ptr<SavePoint>>& GetSavePoints() const { return savePoints; }
		// Snapshot for the travel map; includePlayer = false for a world the player is not in.
		std::shared_ptr<WorldOverview> CaptureOverview(bool includePlayer) const;
		// includePlayer is false when another world's scene draws this one's map (its player isn't "here")
		std::vector<MapView::Marker> BuildMapMarkers(bool includePlayer = true) const;
	};
}
