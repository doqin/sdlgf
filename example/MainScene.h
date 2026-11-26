#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include "ExampleGame.h"
#include "Player.h"
#include "Rectangle.h"

class MainScene : public sdlgf::IScene, public sdlgf::ISaveable
{
private:
	ExampleGame* game;
	sdlgf::InputManager* inputManager = nullptr;
	sdlgf::CommandBuffer commandBuffer;
	std::shared_ptr<sdlgf::Font> fontArial;
	std::shared_ptr<sdlgf::FontSprite> fontSpriteArial;

	std::shared_ptr<sdlgf::Font> fontdetemination;
	std::shared_ptr<sdlgf::FontSprite> fontSpritedetemination;

	std::vector<std::shared_ptr<GO::Player>> players;
	std::shared_ptr<GO::Rectangle> rect;
	std::shared_ptr<sdlgf::TransformManager> transformManager;
	std::shared_ptr<sdlgf::ColliderManager> colliderManager;
	std::shared_ptr<sdlgf::Texture> whiteTexture;
	std::shared_ptr<sdlgf::Texture> dawgTexture;
	std::shared_ptr<sdlgf::StaticSprite> colorRec;
	std::shared_ptr<sdlgf::StaticSprite> textureRec;
	std::shared_ptr<sdlgf::Map> map;
	sdlgf::AudioManager* audioManager;
	float triggerX = 170.0f;
	float triggerY = 170.0f;
	float triggerRadius = 20.0f;
	bool wasInTrigger = false;
public:
	MainScene(ExampleGame* game, int width, int height) : game(game), IScene(width, height) {}
	void Init() override;
	void DrawWorld(unsigned long long deltaTime) override;
	void DrawUI(unsigned long long deltaTime) override;
	void Update(unsigned long long deltaTime) override;
	void Dispose() override;

	// Inherited via ISaveable
	std::string GetSaveID() const override;
	void GenerateSaveData(nlohmann::json& outData) override;
	void RestoreSaveData(const nlohmann::json& inData) override;
};