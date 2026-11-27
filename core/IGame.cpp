#include "pch.h"
#include "IGame.h"
#include "Application.h"
#include "SceneManager.h"
#include "IScene.h"
#include "GraphicsDevice.h"
#include "Texture.h"
#include "Sprites.h"
#include <algorithm>
#include <stdexcept>

sdlgf::IGame::~IGame()
{
}

sdlgf::GraphicsDevice* sdlgf::IGame::GetGraphicsDevice()
{
	return graphicsDevice;
}

sdlgf::SceneManager* sdlgf::IGame::GetSceneManager()
{
	return sceneManager;
}

void sdlgf::IGame::Update(unsigned long long deltaTime)
{
	sceneManager->Update(deltaTime);
}

void sdlgf::IGame::Draw(unsigned long long deltaTime)
{
	//RENDER INTO VIRTUAL TEXTURE (WORLD)
	graphicsDevice->SetRenderTarget(renderTargetTex.get());
	graphicsDevice->SetViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	// The virtual render target is 1:1 with virtual coordinates
	graphicsDevice->ResetVirtualTransform();

	graphicsDevice->Clear(0xFF000000);

	sceneManager->DrawWorld(deltaTime);

	// Render to actual screen and draw UI
	graphicsDevice->RestoreRenderTarget();

	int backBufferWidth = 0, backBufferHeight = 0;
	graphicsDevice->GetOutputSize(backBufferWidth, backBufferHeight);
	// A minimized window has nothing to draw into
	if (backBufferWidth <= 0 || backBufferHeight <= 0) return;
	graphicsDevice->SetViewport(0, 0, backBufferWidth, backBufferHeight);

	graphicsDevice->Clear(0xFF000000);

	float currentWidth = static_cast<float>(backBufferWidth);
	float currentHeight = static_cast<float>(backBufferHeight);

	float scaleX = currentWidth / static_cast<float>(SCREEN_WIDTH);
	float scaleY = currentHeight / static_cast<float>(SCREEN_HEIGHT);
	float scale = (std::min)(scaleX, scaleY);

	float finalW = SCREEN_WIDTH * scale;
	float finalH = SCREEN_HEIGHT * scale;
	float offsetX = (currentWidth - finalW) / 2.0f;
	float offsetY = (currentHeight - finalH) / 2.0f;

	if (graphicsDevice->BeginDraw()) {
		auto currentScene = sceneManager->GetCurrentScene();
		if (currentScene) {
			currentScene->GetUICamera().SetScreenResolution(static_cast<int>(currentWidth), static_cast<int>(currentHeight));
			currentScene->GetUICamera().SetZoom(scale);
			currentScene->GetUICamera().SetPosition(0.0f, 0.0f);
		}

		defaultCamera.SetScreenResolution(static_cast<int>(currentWidth), static_cast<int>(currentHeight));
		defaultCamera.SetPosition(currentWidth / 2.0f, currentHeight / 2.0f);

		renderTargetSprite->SetScale(scale, scale);
		renderTargetSprite->SetPosition(offsetX, offsetY);

		renderTargetSprite->Begin();
		renderTargetSprite->Draw(defaultCamera, deltaTime);
		renderTargetSprite->End();

		graphicsDevice->EndDraw();
	}

	// Camera-less (instant) draws during the UI phase take virtual coordinates,
	// scaled and letterboxed onto the actual backbuffer
	graphicsDevice->SetVirtualTransform(scale, offsetX, offsetY);
	sceneManager->DrawUI(deltaTime);

	graphicsDevice->Present();
}

void sdlgf::IGame::Init()
{
	SDL_Window* window = sdlgf::Application::GetInstance()->GetWindow();
	if (window == nullptr) throw std::runtime_error("IGame::Init needs the Application window: call Application::Init first");

	try {
		graphicsDevice = new GraphicsDevice(window, rendererName.empty() ? nullptr : rendererName.c_str(), vsync);
	}
	catch (const std::runtime_error&) {
		// The preferred driver is gone or broken (e.g. config.ini copied from another machine):
		// fall back to SDL's own choice. If that fails too the exception reaches the caller.
		if (rendererName.empty()) throw;
		rendererName.clear();
		graphicsDevice = new GraphicsDevice(window, nullptr, vsync);
	}
	sceneManager = new SceneManager();

	// Add letterboxing: initialize the virtual screen
	renderTargetTex = std::make_shared<sdlgf::Texture>(graphicsDevice);
	renderTargetTex->CreateRenderTarget(SCREEN_WIDTH, SCREEN_HEIGHT);
	renderTargetSprite = std::make_shared<sdlgf::StaticSprite>(renderTargetTex);
}

void sdlgf::IGame::OnResize(UINT width, UINT height)
{
	// Nothing to rebuild: the renderer follows the window by itself and Draw() reads the
	// current output size every frame.
}

void sdlgf::IGame::OnRenderTargetsReset()
{
	if (sceneManager != nullptr) sceneManager->OnDeviceReset();
}

void sdlgf::IGame::Dispose()
{
	// Scenes own textures: free them while the renderer still exists.
	renderTargetSprite.reset();
	renderTargetTex.reset();
	if (sceneManager != nullptr) { delete sceneManager; sceneManager = nullptr; }
	if (graphicsDevice != nullptr) { delete graphicsDevice; graphicsDevice = nullptr; }
}
