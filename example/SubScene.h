#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "ExampleGame.h"
#include "Rectangle.h"
#include "Ellipse.h"

class SubScene : public sdlgf::IScene, public sdlgf::ISaveable {
private:
	ExampleGame* game = nullptr;
	sdlgf::InputManager* inputManager = nullptr;
	std::shared_ptr<sdlgf::TransformManager> transformManager;
	std::shared_ptr<sdlgf::ColliderManager> colliderManager;
	std::vector<std::shared_ptr<GO::Rectangle>> rects;
	std::vector<std::shared_ptr<GO::Ellipse>> ellipses;
	std::shared_ptr<sdlgf::RectangleTrigger> rectTrigger1;
	std::shared_ptr<sdlgf::RectangleTrigger> rectTrigger2;
public:
	SubScene(ExampleGame* game, int width, int height) : game(game), IScene(width, height) {}
	void Init() override;
	void Update(unsigned long long deltaTime) override;
	void DrawWorld(unsigned long long deltaTime) override;
	void DrawUI(unsigned long long deltaTime) override;
	void Dispose() override;

	// Inherited via ISaveable
	std::string GetSaveID() const override;
	void GenerateSaveData(nlohmann::json& outData) override;
	void RestoreSaveData(const nlohmann::json& inData) override;
};