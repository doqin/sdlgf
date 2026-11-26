#include "pch.h"
#include "IScene.h"

void sdlgf::IScene::Dispose()
{
}

sdlgf::IScene::~IScene() {
	Dispose();
}

sdlgf::Camera& sdlgf::IScene::GetCamera()
{
	return camera;
}

sdlgf::Camera& sdlgf::IScene::GetUICamera()
{
	return uiCamera;
}