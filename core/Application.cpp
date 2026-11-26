#include "pch.h"
#include "Application.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include "Font.h"
#include "InputManager.h"
#include "AudioManager.h"
sdlgf::Application* sdlgf::Application::instance = nullptr;
sdlgf::IGame* p_game = nullptr;
sdlgf::Application::EventHook eventHook = nullptr;

void sdlgf::Application::SetEventHook(EventHook hook)
{
	eventHook = hook;
}

void sdlgf::Application::HandleEvent(const SDL_Event& event)
{
	if (eventHook != nullptr) {
		eventHook(event);
	}
	sdlgf::InputManager::GetInstance()->OnEvent(event);

	switch (event.type)
	{
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
		const int width = event.window.data1;
		const int height = event.window.data2;
		// Minimized windows report no usable size; nothing to resize to
		if (width <= 0 || height <= 0) break;
		if (width == lastClientWidth && height == lastClientHeight) break;
		lastClientWidth = width;
		lastClientHeight = height;

		if (p_game != nullptr) {
			p_game->OnResize(width, height);
		}
		OnResize(width, height);
		break;
	}
	case SDL_EVENT_RENDER_TARGETS_RESET:
	case SDL_EVENT_RENDER_DEVICE_RESET:
		// The graphics driver lost the contents of render-target textures
		if (p_game != nullptr && !gameDisposed) p_game->OnRenderTargetsReset();
		break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
	case SDL_EVENT_QUIT:
		if (p_game != nullptr && !gameDisposed) {
			gameDisposed = true;
			p_game->Dispose();
		}
		quitRequested = true;
		break;
	}
}

void sdlgf::Application::Init(std::wstring appTitle, UINT screenWidth, UINT screenHeight, bool resizable)
{
	this->appTitle = appTitle;
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;

	// Ensure the current working directory is the executable directory so relative paths work
	const std::string exeDir = sdlgf::Platform::GetExecutableDirectory();
	if (!exeDir.empty()) {
		std::error_code ec;
		std::filesystem::current_path(std::filesystem::path(std::u8string(exeDir.begin(), exeDir.end())), ec);
	}

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		throw std::runtime_error(std::string("Error initializing SDL: ") + SDL_GetError());
	}

	SDL_WindowFlags flags = 0;
	if (resizable) flags |= SDL_WINDOW_RESIZABLE;
	window = SDL_CreateWindow(sdlgf::Platform::WideToUtf8(appTitle).c_str(),
		static_cast<int>(screenWidth), static_cast<int>(screenHeight), flags);
	if (window == nullptr) {
		throw std::runtime_error(std::string("Error creating window: ") + SDL_GetError());
	}

	ApplyIcon();
	GetClientSize(lastClientWidth, lastClientHeight);

	sdlgf::InputManager::GetInstance()->Init();
}

void sdlgf::Application::SetFramerate(int fps)
{
	frameRate = fps;
}

SDL_Window* sdlgf::Application::GetWindow() const
{
	return window;
}

sdlgf::Application* sdlgf::Application::GetInstance()
{
	if (instance == nullptr) {
		instance = new Application();
	}
	return instance;
}

unsigned int sdlgf::Application::GetScreenWidth() const
{
	return screenWidth;
}

unsigned int sdlgf::Application::GetScreenHeight() const
{
	return screenHeight;
}

void sdlgf::Application::GetClientSize(int& width, int& height) const
{
	width = 0;
	height = 0;
	if (window != nullptr) {
		SDL_GetWindowSizeInPixels(window, &width, &height);
	}
}

float sdlgf::Application::GetPixelDensity() const
{
	if (window == nullptr) return 1.0f;
	const float density = SDL_GetWindowPixelDensity(window);
	return density > 0.0f ? density : 1.0f;
}

void sdlgf::Application::OnResize(UINT width, UINT height)
{
	//this->screenWidth = width;
	//this->screenHeight = height;
}

void sdlgf::Application::AttachGame(IGame* game)
{
	p_game = game;
	gameDisposed = false;
	if (p_game != nullptr && window != nullptr) {
		int width = 0, height = 0;
		GetClientSize(width, height);
		if (width > 0 && height > 0) {
			lastClientWidth = width;
			lastClientHeight = height;
			p_game->OnResize(width, height);
		}
	}
}

void sdlgf::Application::Run()
{

	//test audio
	auto audioManager = AudioManager::GetInstance();
	audioManager->Init();

	unsigned long long start = sdlgf::Platform::GetTicksMs();
	while (!quitRequested) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			HandleEvent(event);
		}
		if (quitRequested) break;

		if (frameRate == -1 || sdlgf::Platform::GetTicksMs() - start >= 1000 / frameRate) {
			unsigned long long rawDeltaTime = sdlgf::Platform::GetTicksMs() - start;
			start = sdlgf::Platform::GetTicksMs();

			// Clamp the delta fed to Update/Draw so a stall (e.g. a heavy scene load)
			// doesn't get replayed as one giant time jump - that skips timer-driven
			// animations (fades, transitions) straight to their end instead of playing
			// them smoothly
			const unsigned long long MAX_DELTA_TIME_MS = 100;
			unsigned long long deltaTime = (rawDeltaTime > MAX_DELTA_TIME_MS) ? MAX_DELTA_TIME_MS : rawDeltaTime;

			p_game->Update(deltaTime);
			audioManager->Update(deltaTime);
			p_game->Draw(deltaTime);
		}
		else {
			SDL_Delay(1); // frame not due yet: don't spin a core
		}
	}
	// The renderer belongs to the window: release it (and everything drawn with it) first
	if (p_game != nullptr && !gameDisposed) {
		gameDisposed = true;
		p_game->Dispose();
	}
	// Cleanup any fonts added at runtime
	sdlgf::Font::RemoveAllFonts();
	audioManager->Shutdown();

	SDL_DestroyWindow(window);
	window = nullptr;
	SDL_Quit();
}

void sdlgf::Application::Quit()
{
	quitRequested = true;
}

void sdlgf::Application::SetAppIcon(const std::wstring& iconPath)
{
	pendingIconPath = sdlgf::Platform::WideToUtf8(iconPath);
	ApplyIcon(); // no-op until the window exists; Init applies it then
}

void sdlgf::Application::ApplyIcon()
{
	if (window == nullptr || pendingIconPath.empty()) return;
	SDL_Surface* icon = SDL_LoadSurface(pendingIconPath.c_str());
	if (icon == nullptr) {
		sdlgf::Platform::DebugLog("Could not load window icon '" + pendingIconPath + "': " + SDL_GetError() + "\n");
		return;
	}
	SDL_SetWindowIcon(window, icon);
	SDL_DestroySurface(icon);
}

void sdlgf::Application::SetTitle(const std::wstring& title)
{
	appTitle = title;
	if (window != nullptr) {
		SDL_SetWindowTitle(window, sdlgf::Platform::WideToUtf8(title).c_str());
	}
}

void sdlgf::Application::SetFullscreen(bool fullscreen)
{
	if (this->isFullscreen == fullscreen) return;
	this->isFullscreen = fullscreen;
	if (window == nullptr) return;

	// With no explicit fullscreen mode set this is borderless fullscreen at desktop
	// resolution; leaving it restores the previous size and position.
	SDL_SetWindowFullscreen(window, fullscreen);
}

void sdlgf::Application::SetWindowedSize(UINT width, UINT height)
{
	if (window == nullptr) return;

	SetFullscreen(false);
	// Leaving fullscreen is asynchronous on some platforms; wait so the size and position
	// below are applied to the restored window instead of being overwritten by it.
	SDL_SyncWindow(window);

	SDL_SetWindowSize(window, static_cast<int>(width), static_cast<int>(height));
	SDL_SyncWindow(window);

	int top = 0, left = 0, bottom = 0, right = 0;
	SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right);
	const int outerWidth = static_cast<int>(width) + left + right;
	const int outerHeight = static_cast<int>(height) + top + bottom;

	SDL_Rect work{};
	if (!SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(window), &work)) return;

	const int posX = work.x + (work.w - outerWidth) / 2;
	const int idealPosY = work.y + (work.h - outerHeight) / 3;
	const int safePosY = work.y + 10;
	const int posY = (std::max)(idealPosY, safePosY);

	// SDL positions the client area; the position above is for the outer frame.
	SDL_SetWindowPosition(window, posX + left, posY + top);
}
