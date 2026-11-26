#include "pch.h"
// Provides the platform entry point (WinMain on Windows) that calls main() below. Must be
// included in exactly one translation unit, the one that defines main().
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include "sdlgf.h"
#include "CrashHandler.h"
#include "Game.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

int main(int argc, char** argv)
{
	// Get the sdlgf Application
	sdlgf::Application* app = sdlgf::Application::GetInstance();

	sdlgf::InstallCrashHandler();

	try {
        app->SetAppIcon(L"assets/icon.png");
		// Dear ImGui sees every SDL event (mouse/keyboard/text) before the game does
		app->SetEventHook([](const SDL_Event& event) {
			ImGui_ImplSDL3_ProcessEvent(&event);
		});
		app->Init(Demo::GAME_TITLE, 960, 720, false);
		// Create your game that interfaces with IGame
		Demo::Game game(app->GetScreenWidth(), app->GetScreenHeight());
		game.Init();
		// Attach your game to the application
		app->AttachGame(&game);
		// Run the application
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		ImGui::StyleColorsDark();
		ImGui_ImplSDL3_InitForSDLRenderer(app->GetWindow(), game.GetGraphicsDevice()->GetRenderer());
		ImGui_ImplSDLRenderer3_Init(game.GetGraphicsDevice()->GetRenderer());
		app->Run();
	}
	catch (const std::exception& e) {
		sdlgf::Platform::ShowErrorMessage("Error", e.what());
		return 1;
	}
	// A throw of something that isn't std::exception would otherwise vanish
	// without a trace on machines we can't debug (sandbox/CI/end users)
	catch (...) {
		sdlgf::Platform::ShowErrorMessage("Error", "Unknown fatal error (non-standard exception)");
		return 1;
	}


	return 0;
}
