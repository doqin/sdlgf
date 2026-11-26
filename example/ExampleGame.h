#pragma once
#include "sdlgf.h"
#include "Extras.h"

class ExampleGame : public sdlgf::IGame, public sdlgf::ISaveable
{
private:
	sdlgf::SaveManager* saveManager;
	std::unordered_map<std::string, int> sceneMap;
public:
	ExampleGame(HWND hwnd, const UINT screenWidth, const UINT screenHeight, sdlgf::SaveManager* saveManager) : IGame(hwnd, screenWidth, screenHeight), saveManager(saveManager) {};
	void Init() override;

	// Inherited via ISaveable
	std::string GetSaveID() const override;
	void GenerateSaveData(nlohmann::json& outData) override;
	void RestoreSaveData(const nlohmann::json& inData) override;
	sdlgf::SaveManager* GetSaveManager();
};

