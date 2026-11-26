#include "pch.h"
#include "SceneManager.h"
#include "IScene.h"
#include <stdexcept>
#include "Application.h"

sdlgf::SceneManager::~SceneManager()
{
	Dispose();
}

sdlgf::IScene* sdlgf::SceneManager::GetCurrentScene()
{
	if (loadingScene) return loadingScene.get();
	return scenes[index];
}

sdlgf::IScene* sdlgf::SceneManager::GetScene(size_t index) {
	return scenes[index];
}

void sdlgf::SceneManager::ChangeScene(IScene* scene)
{
	if (!scenes.empty()) {
		delete scenes[index];
	}
	scenes[index] = scene;
	scene->Init();
}

void sdlgf::SceneManager::PushScene(IScene* scene)
{
	scenes.push_back(scene);
	scene->Init();
}

void sdlgf::SceneManager::InsertScene(size_t index, IScene* scene)
{
	scenes.insert(scenes.begin() + index, scene);
	scene->Init();
}

void sdlgf::SceneManager::PopScene()
{
	auto scene = scenes.back();
	delete scene;
	scenes.pop_back();
}

void sdlgf::SceneManager::RemoveScene(size_t index)
{
	auto scene = scenes[index];
	delete scene;
	scenes.erase(scenes.begin() + index);
}

void sdlgf::SceneManager::BeginLoading(std::unique_ptr<IScene> scene)
{
	loadingScene = std::move(scene);
	endLoadingRequested = false;
	loadingScene->Init();
}

void sdlgf::SceneManager::Update(unsigned long long deltaTime)
{
	if (loadingScene) {
		loadingScene->Update(deltaTime);
		if (endLoadingRequested) {
			loadingScene.reset();
			endLoadingRequested = false;
		}
		return;
	}
	if (scenes.empty()) {
		throw std::runtime_error("No scene to update!");
	}
	scenes[index]->Update(deltaTime);
}

void sdlgf::SceneManager::DrawWorld(unsigned long long deltaTime)
{
	if (loadingScene) { loadingScene->DrawWorld(deltaTime); return; }
	if (scenes.empty()) throw std::runtime_error("No scene to draw!");
	int startIndex = index;
	while (startIndex > 0 && scenes[startIndex]->IsOverlay()) startIndex--;
	for (int i = startIndex; i <= index; ++i) scenes[i]->DrawWorld(deltaTime);
}

void sdlgf::SceneManager::DrawUI(unsigned long long deltaTime)
{
	if (loadingScene) { loadingScene->DrawUI(deltaTime); return; }
	if (scenes.empty()) throw std::runtime_error("No scene to draw!");
	int startIndex = index;
	while (startIndex > 0 && scenes[startIndex]->IsOverlay()) startIndex--;
	for (int i = startIndex; i <= index; ++i) scenes[i]->DrawUI(deltaTime);
}

void sdlgf::SceneManager::OnResize(int width, int height)
{
	for (auto& scene : scenes) {
		scene->GetUICamera().SetScreenResolution(width, height);
	}
	if (loadingScene) loadingScene->GetUICamera().SetScreenResolution(width, height);
}

void sdlgf::SceneManager::OnDeviceReset()
{
	for (auto& scene : scenes) {
		scene->OnDeviceReset();
	}
}

void sdlgf::SceneManager::GoToNext()
{
	index++;
	if (index >= scenes.size()) throw std::runtime_error("Attempted to switch to a scene out of bound");
}

void sdlgf::SceneManager::GoToPrevious()
{
	index--;
	if (index < 0) throw std::runtime_error("Attempted to switch to a scene out of bound");
}

void sdlgf::SceneManager::GoToScene(size_t index)
{
	this->index = index;
	if (index < 0 || index >= scenes.size()) throw std::runtime_error("Attempted to switch to a scene out of bound");
}

void sdlgf::SceneManager::Dispose()
{
	loadingScene.reset();
	while (!scenes.empty()) {
		PopScene();
	}
}

size_t sdlgf::SceneManager::GetSceneCount() const
{
	return scenes.size();
}