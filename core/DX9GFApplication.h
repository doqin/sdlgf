#pragma once
#include "framework.h"
#include "DX9GFIGame.h"
#include <string>

struct SDL_Window;
union SDL_Event;

namespace DX9GF {
	class Application final {
	private:
		SDL_Window* window = nullptr;
		std::wstring appTitle;
		UINT screenWidth = 0;
		UINT screenHeight = 0;
		int frameRate = -1;

		std::string pendingIconPath; // UTF-8, applied once the window exists
		bool quitRequested = false;
		bool gameDisposed = false;
		// Last client size (pixels) forwarded to the game, so redundant resize events
		// (SDL can report one right after the window is created) don't trigger a device reset.
		int lastClientWidth = 0;
		int lastClientHeight = 0;
		static Application* instance;

		void ApplyIcon();
		void HandleEvent(const SDL_Event& event);
	public:
		/// <summary>
		/// Receives every SDL event before the engine handles it (used to feed Dear ImGui).
		/// </summary>
		using EventHook = void(*)(const SDL_Event&);
		void SetEventHook(EventHook hook);
		void SetOnDeviceResetHandler(void(*handler)());

		/// <summary>
		/// Initializes SDL and creates the application window.
		/// </summary>
		/// <param name="appTitle">The title to display in the application window's title bar.</param>
		/// <param name="screenWidth">The width of the application window's client area in pixels.</param>
		/// <param name="screenHeight">The height of the application window's client area in pixels.</param>
		/// <param name="resizable">Whether the application window is resizable.</param>
		void Init(std::wstring appTitle, UINT screenWidth, UINT screenHeight, bool resizable = true);
		/// <summary>
		/// Sets the window icon. Accepts BMP or PNG (not .ico). May be called before Init.
		/// </summary>
		void SetAppIcon(const std::wstring& iconPath);
		void SetTitle(const std::wstring& title);
		void AttachGame(IGame* game);
		void Run();
		/// <summary>
		/// Asks the main loop to exit after the current frame.
		/// </summary>
		void Quit();

		void SetFramerate(int fps);
		SDL_Window* GetWindow() const;
#ifdef _WIN32
		/// <summary>
		/// The window's native handle. Only needed by the Direct3D 9 renderer; goes away
		/// when that is replaced.
		/// </summary>
		HWND GetHWnd() const;
#endif
		static Application* GetInstance();
		unsigned int GetScreenWidth() const;
		unsigned int GetScreenHeight() const;
		/// <summary>
		/// Size of the window's drawable area in pixels (what GetClientRect used to return).
		/// </summary>
		void GetClientSize(int& width, int& height) const;
		/// <summary>
		/// Pixels per window coordinate unit (1.0 unless the platform scales window coordinates).
		/// </summary>
		float GetPixelDensity() const;

		void OnResize(UINT width, UINT height);
		void SetFullscreen(bool fullscreen);
		bool IsFullscreen() const { return isFullscreen; }
		/// <summary>
		/// Leaves fullscreen, sizes the client area to width x height and centers the window
		/// on its display (upper third, never closer than 10 px to the top of the work area).
		/// </summary>
		void SetWindowedSize(UINT width, UINT height);
	private:
		Application() {}
		~Application() {}
		bool isFullscreen = false;
	};
};