#include "pch.h"
#include "Game.h"
#include "DebugScene.h"
#include "World1Scene.h"
#include "MainMenu.h"
#include "SettingsScene.h"
#include "AudioManager.h"
#include <cwchar>
#include <iterator>
#include "SettingsManager.h"
#include "SplashScene.h"
void Demo::Game::Init()
{
	srand(static_cast<unsigned int>(time(NULL)));
	IGame::Init();
	SettingsManager::GetInstance()->LoadSettings();
	SettingsManager::GetInstance()->ApplyResolution();

	auto audio = sdlgf::AudioManager::GetInstance();
	audio->Init();

	// Load sound effects
	audio->Load("btn_hover", "assets/audio/button-hover.wav");
	audio->Load("btn_click", "assets/audio/button-click.wav");
	audio->Load("open_inv", "assets/audio/inventory_open.wav");
	audio->Load("close_inv", "assets/audio/inventory_close.wav");

	audio->Load("step_d1", "assets/audio/step_default1.wav");
	audio->Load("step_d2", "assets/audio/step_default2.wav");
	audio->Load("step_d3", "assets/audio/step_default3.wav");
	audio->Load("step_d4", "assets/audio/step_default4.wav");

	audio->Load("step_l1", "assets/audio/step_leaves1.wav");
	audio->Load("step_l2", "assets/audio/step_leaves2.wav");
	audio->Load("step_l3", "assets/audio/step_leaves3.wav");
	audio->Load("step_l4", "assets/audio/step_leaves4.wav");

	audio->Load("step_m1", "assets/audio/step_metal1.wav");
	audio->Load("step_m2", "assets/audio/step_metal2.wav");
	audio->Load("step_m3", "assets/audio/step_metal3.wav");
	audio->Load("step_m4", "assets/audio/step_metal4.wav");
	audio->Load("step_m5", "assets/audio/step_metal5.wav");

	audio->Load("step_dirt1", "assets/audio/step_dirt1.wav");
	audio->Load("step_dirt2", "assets/audio/step_dirt2.wav");
	audio->Load("step_dirt3", "assets/audio/step_dirt3.wav");
	audio->Load("step_dirt4", "assets/audio/step_dirt4.wav");
	audio->Load("step_dirt5", "assets/audio/step_dirt5.wav");

	audio->Load("step_concrete1", "assets/audio/step_concrete1.wav");
	audio->Load("step_concrete2", "assets/audio/step_concrete2.wav");
	audio->Load("step_concrete3", "assets/audio/step_concrete3.wav");
	audio->Load("step_concrete4", "assets/audio/step_concrete4.wav");

	audio->Load("step_vinyl1", "assets/audio/step_vinyl1.wav");
	audio->Load("step_vinyl2", "assets/audio/step_vinyl2.wav");
	audio->Load("step_vinyl3", "assets/audio/step_vinyl3.wav");
	audio->Load("step_vinyl4", "assets/audio/step_vinyl4.wav");

	audio->RegisterBank("step_default", { "step_d1", "step_d2", "step_d3", "step_d4" });
	audio->RegisterBank("step_leaves", { "step_l1", "step_l2", "step_l3", "step_l4" });
	audio->RegisterBank("step_metal", { "step_m1", "step_m2", "step_m3", "step_m4", "step_m5" });
	audio->RegisterBank("step_dirt", { "step_dirt1", "step_dirt2", "step_dirt3", "step_dirt4", "step_dirt5" });
	audio->RegisterBank("step_concrete", { "step_concrete1", "step_concrete2", "step_concrete3", "step_concrete4" });
	audio->RegisterBank("step_vinyl", { "step_vinyl1", "step_vinyl2", "step_vinyl3", "step_vinyl4" });

	audio->Load("shop_buy", "assets/audio/buy_item.wav");
	audio->Load("error", "assets/audio/error.wav");

	audio->Load("card_draw1", "assets/audio/card_draw1.wav");
	audio->Load("card_draw2", "assets/audio/card_draw2.wav");
	audio->Load("card_draw3", "assets/audio/card_draw3.wav");
	audio->Load("card_draw4", "assets/audio/card_draw4.wav");
	audio->Load("card_draw5", "assets/audio/card_draw5.wav");

	audio->RegisterBank("card_draw", { "card_draw1", "card_draw2", "card_draw3", "card_draw4", "card_draw5" });

	audio->Load("hurt1", "assets/audio/hurt1.wav");
	audio->Load("hurt2", "assets/audio/hurt2.wav");
	audio->Load("hurt3", "assets/audio/hurt3.wav");
	audio->Load("hurt4", "assets/audio/hurt4.wav");
	audio->Load("hurt5", "assets/audio/hurt5.wav");

	audio->RegisterBank("take_dmg", { "hurt1", "hurt2", "hurt3", "hurt4", "hurt5" });

	audio->Load("player_dead", "assets/audio/dead.wav");
	audio->Load("coin_gather", "assets/audio/coins_gather.wav");

	audio->Load("card_snap1", "assets/audio/card_snap.wav");
	audio->Load("card_snap2", "assets/audio/card_snap2.wav");
	audio->Load("card_snap3", "assets/audio/card_snap3.wav");
	audio->Load("card_snap4", "assets/audio/card_snap4.wav");
	audio->RegisterBank("card_snap", { "card_snap1", "card_snap2", "card_snap3", "card_snap4" });

	audio->Load("dialog_voice1", "assets/audio/dialog_voice1.wav");
	audio->Load("dialog_voice2", "assets/audio/dialog_voice2.wav");
	audio->Load("dialog_voice3", "assets/audio/dialog_voice3.wav");
	audio->Load("dialog_voice4", "assets/audio/dialog_voice4.wav");
	audio->Load("dialog_voice5", "assets/audio/dialog_voice5.wav");
	audio->RegisterBank("dialog_voice", { "dialog_voice1", "dialog_voice2", "dialog_voice3", "dialog_voice4", "dialog_voice5" });

	audio->Load("bleep1", "assets/audio/bleeps/bleep001.wav");
	audio->Load("bleep2", "assets/audio/bleeps/bleep002.wav");
	audio->Load("bleep3", "assets/audio/bleeps/bleep003.wav");
	audio->Load("bleep4", "assets/audio/bleeps/bleep004.wav");
	audio->Load("bleep5", "assets/audio/bleeps/bleep005.wav");
	audio->Load("bleep6", "assets/audio/bleeps/bleep006.wav");
	audio->Load("bleep7", "assets/audio/bleeps/bleep007.wav");
	audio->Load("bleep8", "assets/audio/bleeps/bleep008.wav");
	audio->Load("bleep9", "assets/audio/bleeps/bleep009.wav");
	audio->Load("bleep10", "assets/audio/bleeps/bleep010.wav");
	audio->Load("bleep11", "assets/audio/bleeps/bleep011.wav");
	audio->Load("bleep12", "assets/audio/bleeps/bleep012.wav");
	audio->Load("bleep13", "assets/audio/bleeps/bleep013.wav");
	audio->Load("bleep14", "assets/audio/bleeps/bleep014.wav");
	audio->Load("bleep15", "assets/audio/bleeps/bleep015.wav");
	audio->Load("bleep16", "assets/audio/bleeps/bleep016.wav");
	audio->Load("bleep17", "assets/audio/bleeps/bleep017.wav");
	audio->Load("bleep18", "assets/audio/bleeps/bleep018.wav");
	audio->Load("bleep19", "assets/audio/bleeps/bleep019.wav");
	audio->Load("bleep20", "assets/audio/bleeps/bleep020.wav");
	audio->Load("bleep21", "assets/audio/bleeps/bleep021.wav");
	audio->Load("bleep22", "assets/audio/bleeps/bleep022.wav");
	audio->Load("bleep23", "assets/audio/bleeps/bleep023.wav");
	audio->Load("bleep24", "assets/audio/bleeps/bleep024.wav");
	audio->Load("bleep25", "assets/audio/bleeps/bleep025.wav");
	audio->Load("bleep26", "assets/audio/bleeps/bleep026.wav");
	audio->Load("bleep27", "assets/audio/bleeps/bleep027.wav");
	audio->Load("bleep28", "assets/audio/bleeps/bleep028.wav");
	audio->Load("bleep29", "assets/audio/bleeps/bleep029.wav");
	audio->Load("bleep30", "assets/audio/bleeps/bleep030.wav");

	audio->Load("power_up1", "assets/audio/power_up1.wav");
	audio->Load("power_up2", "assets/audio/power_up2.wav");
	audio->Load("power_up3", "assets/audio/power_up3.wav");

	audio->RegisterBank("power_up", { "power_up1", "power_up2", "power_up3" });

	audio->Load("checkpoint", "assets/audio/checkpoint.wav");
	audio->Load("quest_active", "assets/audio/quest_active.wav");
	audio->Load("quest_completed", "assets/audio/quest_completed.wav");
	audio->Load("projectile_spawn", "assets/audio/projectile_spawn.wav");
	audio->Load("projectile_launch", "assets/audio/projectile_launch.wav");

	//load maps bgm
	audio->Load("bgm_tutorial", "assets/audio/tutorial-loop.wav");
	audio->Load("bgm_sky", "assets/audio/sky-loop.wav");
	audio->Load("bgm_secret", "assets/audio/secret-loop.wav");
	audio->Load("bgm_boss", "assets/audio/boss-loop.wav");

	//boss world dynamic music: shared base plus one layer per island group
	audio->Load("bossworld_base", "assets/audio/bossworld-base.wav");
	audio->Load("bossworld_island12", "assets/audio/bossworld-island12.wav");
	audio->Load("bossworld_island3", "assets/audio/bossworld-island3.wav");
	audio->Load("bossworld_island4", "assets/audio/bossworld-island4.wav");
	audio->RegisterStemSet("bgm_bossworld", { "bossworld_base", "bossworld_island12", "bossworld_island3", "bossworld_island4" });
	audio->SetActiveStems("bgm_bossworld", { 0, 1 }, 0.0f);
	audio->Load("bgm_arcade", "assets/audio/arcade-music-loop.wav");

	auto app = sdlgf::Application::GetInstance();
	sdlgf::Font::AddFont(L"assets/arcade-among-2-r46pv.ttf");
	sdlgf::Font::AddFont(L"assets/statusplz.ttf");
	sdlgf::Font::AddFont(L"assets/cardpixel.ttf");
	sdlgf::Font::AddFont(L"assets/cardpixel-top.ttf");
	sdlgf::Font::AddFont(L"assets/cardpixel-bottom.ttf");
	sdlgf::Font::AddFont(L"assets/135openpixel-v2-3.ttf");
	//Load cursor textures
	auto input = sdlgf::InputManager::GetInstance();
	auto gd = GetGraphicsDevice();

	//hotspot (hX, hY) must be multiplied by the scale factor
	input->AddCursor(sdlgf::InputManager::CursorType::CURSOR, gd, L"assets/cursor.png", 0.2f, 0.0f, 0.0f);
	input->AddCursor(sdlgf::InputManager::CursorType::POINTER, gd, L"assets/pointer.png", 0.2f, 0.0f, 0.0f);
	input->AddCursor(sdlgf::InputManager::CursorType::CLICK, gd, L"assets/click.png", 0.2f, 4.0f, 6.0f);
	input->AddCursor(sdlgf::InputManager::CursorType::GRAB, gd, L"assets/grab.png", 0.2f, 14.8f, 14.8);
	input->AddCursor(sdlgf::InputManager::CursorType::TEXTSELECT, gd, L"assets/text-select.png", 0.2f, 10.0f, 16.0f);
#ifdef TESTING
	this->sceneManager->PushScene(new DebugScene(this, app->GetScreenWidth(), app->GetScreenHeight()));
#else
	// this->sceneManager->PushScene(new World1Scene(this, app->GetScreenWidth(), app->GetScreenHeight()));
	this->sceneManager->PushScene(new SplashScene(this, app->GetScreenWidth(), app->GetScreenHeight()));

#endif
	this->sceneManager->GoToNext();
}

void Demo::Game::Update(unsigned long long deltaTime)
{
	IGame::Update(deltaTime);

#ifdef _DEBUG
	fpsAccumulator += deltaTime;
	fpsFrames++;

	if (fpsAccumulator >= 1000) {
		float msPerFrame = (float)fpsAccumulator / fpsFrames;
		wchar_t buf[256];
		std::swprintf(buf, std::size(buf), L"%ls - FPS: %d (%.2f ms/frame)", GAME_TITLE, fpsFrames, msPerFrame);
		sdlgf::Application::GetInstance()->SetTitle(buf);

		fpsAccumulator = 0;
		fpsFrames = 0;
	}
#endif
}
