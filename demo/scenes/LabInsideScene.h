#pragma once
#include "WorldSceneBase.h"

namespace Demo {
	class LabInsideScene : public WorldSceneBase {
		bool isBossDead = false;

	public:
		LabInsideScene(Game* game, std::shared_ptr<sdlgf::SaveManager> sm, UINT sw, UINT sh)
			: WorldSceneBase(game, sm, sw, sh) {
		}
		std::string GetSaveID() const override;

	protected:
		void OnInit() override;
		bool ShowMiniMap() const override { return false; }
		void OnGenerateSaveData(nlohmann::json& outData) override;
		void OnRestoreSaveData(const nlohmann::json& inData) override;
		void DrawBackground(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime) override;
	};
}
