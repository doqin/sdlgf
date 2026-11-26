#include "pch.h"
// Provides the platform entry point (WinMain on Windows) that calls main() below. Must be
// included in exactly one translation unit, the one that defines main().
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include "DX9GF.h"
#include "DX9GFCrashHandler.h"
#include "Game.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_dx9.h"

int main(int argc, char** argv)
{
	// Get the DX9GF Application
	DX9GF::Application* app = DX9GF::Application::GetInstance();

	DX9GF::InstallCrashHandler();

	try {
        app->SetAppIcon(L"assets/icon.png");
		// Dear ImGui sees every SDL event (mouse/keyboard/text) before the game does
		app->SetEventHook([](const SDL_Event& event) {
			ImGui_ImplSDL3_ProcessEvent(&event);
		});
		app->SetOnDeviceResetHandler([]() {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			ImGui_ImplDX9_CreateDeviceObjects();
		});
		app->Init(Demo::GAME_TITLE, 960, 720, false);
		// Create your game that interfaces with IGame
		Demo::Game game(app->GetHWnd(), app->GetScreenWidth(), app->GetScreenHeight());
		game.Init();
		// Attach your game to the application
		app->AttachGame(&game);
		// Run the application
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		ImGui::StyleColorsDark();
		ImGui_ImplSDL3_InitForOther(app->GetWindow());
		ImGui_ImplDX9_Init(game.GetGraphicsDevice()->GetDevice());
		app->Run();
	}
	catch (const std::exception& e) {
		DX9GF::Platform::ShowErrorMessage("Error", e.what());
		return 1;
	}
	// A throw of something that isn't std::exception would otherwise vanish
	// without a trace on machines we can't debug (sandbox/CI/end users)
	catch (...) {
		DX9GF::Platform::ShowErrorMessage("Error", "Unknown fatal error (non-standard exception)");
		return 1;
	}


	return 0;
}
