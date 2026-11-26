#include "pch.h"
#include "World1Scene.h"

void Demo::World1Scene::Init()
{
	transformManager = std::make_shared<sdlgf::TransformManager>();
	player = std::make_shared<Player>(transformManager);
	player->Init(game->GetGraphicsDevice(), &colliderManager, &camera);
	transformManager->RebuildHierarchy();
	camera.SetZoom(2.f);
}

void Demo::World1Scene::Update(unsigned long long deltaTime)
{
	auto inpMan = sdlgf::InputManager::GetInstance();
	inpMan->ReadMouse(deltaTime);
	inpMan->ReadKeyboard(deltaTime);
	if (inpMan->KeyDown(DIK_F1)) sdlgf::ICollider::drawCollider = !sdlgf::ICollider::drawCollider;
	if (inpMan->KeyDown(DIK_F2)) sdlgf::DebugDraw::drawGrid = !sdlgf::DebugDraw::drawGrid;
	player->Update(deltaTime);
	transformManager->UpdateAll();
	camera.Update();
}

void Demo::World1Scene::DrawWorld(unsigned long long deltaTime)
{
	auto gd = game->GetGraphicsDevice();
	if (SUCCEEDED(gd->BeginDraw())) {
		auto app = sdlgf::Application::GetInstance();
		if (sdlgf::DebugDraw::drawGrid) {
			sdlgf::DebugDraw::DrawGrid(game->GetGraphicsDevice(), camera, 0, 0, app->GetScreenWidth(), app->GetScreenHeight(), 16, 16, 0x88888888);
		}
		player->Draw(deltaTime);
		gd->EndDraw();
	}
}
void Demo::World1Scene::DrawUI(unsigned long long deltaTime)
{
}
