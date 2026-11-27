#pragma once
#include "framework.h"
#include <memory>
#include <string>
#include "Camera.h"
namespace sdlgf {
	class GraphicsDevice;
	class SceneManager;
	class Texture;
	class StaticSprite;

	/// Base class of a game: owns the renderer and the scene stack, and draws the virtual
	/// screen (SCREEN_WIDTH x SCREEN_HEIGHT) letterboxed into the window.
	class IGame
	{
	private:
		//LETTERBOXING
		std::shared_ptr<sdlgf::Texture> renderTargetTex;
		std::shared_ptr<sdlgf::StaticSprite> renderTargetSprite;
		sdlgf::Camera defaultCamera;
	protected:
		const UINT SCREEN_WIDTH;
		const UINT SCREEN_HEIGHT;

		GraphicsDevice* graphicsDevice = nullptr;
		SceneManager* sceneManager = nullptr;

		std::string rendererName; // empty = let SDL pick
		bool vsync = true;
	public:
		IGame(const UINT screenWidth, const UINT screenHeight)
			: SCREEN_WIDTH(screenWidth), SCREEN_HEIGHT(screenHeight),
			defaultCamera(screenWidth, screenHeight) {
		}
		virtual ~IGame();

		GraphicsDevice* GetGraphicsDevice();
		SceneManager* GetSceneManager();

		/// Render driver and V-Sync mode Init() will create the renderer with; call before Init().
		/// An empty name lets SDL pick the best driver (and honours SDL_RENDER_DRIVER).
		void SetRendererPreference(std::string name, bool enableVSync) { rendererName = std::move(name); vsync = enableVSync; }

		/// Creates the renderer for the Application's window and the scene manager.
		virtual void Init();
		virtual void OnResize(UINT width, UINT height);
		/// The contents of render-target textures were lost (SDL_EVENT_RENDER_TARGETS_RESET);
		/// scenes get to redraw anything they had rendered into one.
		virtual void OnRenderTargetsReset();
		virtual void Update(unsigned long long deltaTime);
		virtual void Draw(unsigned long long deltaTime);
		virtual void Dispose();
	};
}
