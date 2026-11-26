// Headless test of Application window creation + InputManager semantics.
//
// Uses SDL's "dummy" video driver (a real SDL window, no display needed) and feeds
// synthetic SDL events straight into InputManager::OnEvent, then checks the per-frame
// KeyPress/KeyDown/KeyUp, mouse, wheel and letterbox behaviour the game depends on.
#include <SDL3/SDL.h>

#include "Application.h"
#include "Camera.h"
#include "InputManager.h"
#include "Keys.h"

#include <cstdio>

static int g_failures = 0;
#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

using sdlgf::InputManager;

static SDL_Event Key(SDL_Scancode sc, bool down, bool repeat = false)
{
	SDL_Event e{};
	e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
	e.key.scancode = sc;
	e.key.down = down;
	e.key.repeat = repeat;
	return e;
}

static SDL_Event Button(Uint8 button, bool down, float x = 0, float y = 0)
{
	SDL_Event e{};
	e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
	e.button.button = button;
	e.button.down = down;
	e.button.x = x;
	e.button.y = y;
	return e;
}

static SDL_Event Motion(float x, float y)
{
	SDL_Event e{};
	e.type = SDL_EVENT_MOUSE_MOTION;
	e.motion.x = x;
	e.motion.y = y;
	return e;
}

static SDL_Event Wheel(float y, bool flipped = false)
{
	SDL_Event e{};
	e.type = SDL_EVENT_MOUSE_WHEEL;
	e.wheel.y = y;
	e.wheel.direction = flipped ? SDL_MOUSEWHEEL_FLIPPED : SDL_MOUSEWHEEL_NORMAL;
	return e;
}

static void Feed(InputManager* in, const SDL_Event& e) { in->OnEvent(e); }

static void TestKeyboardSemantics(InputManager* in)
{
	in->ReadKeyboard(16);
	CHECK(!in->KeyPress(DIK_A));

	Feed(in, Key(SDL_SCANCODE_A, true));
	in->ReadKeyboard(16);
	CHECK(in->KeyPress(DIK_A));
	CHECK(in->KeyDown(DIK_A));      // pressed this frame, not the previous one
	CHECK(!in->KeyUp(DIK_A));

	in->ReadKeyboard(16);           // still held
	CHECK(in->KeyPress(DIK_A));
	CHECK(!in->KeyDown(DIK_A));
	CHECK(!in->KeyUp(DIK_A));

	Feed(in, Key(SDL_SCANCODE_A, true, /*repeat*/ true));  // OS auto-repeat changes nothing
	in->ReadKeyboard(16);
	CHECK(in->KeyPress(DIK_A));
	CHECK(!in->KeyDown(DIK_A));

	Feed(in, Key(SDL_SCANCODE_A, false));
	in->ReadKeyboard(16);
	CHECK(!in->KeyPress(DIK_A));
	CHECK(in->KeyUp(DIK_A));
	in->ReadKeyboard(16);
	CHECK(!in->KeyUp(DIK_A));

	// A tap shorter than one frame (down+up between two reads) is invisible, as with polling
	Feed(in, Key(SDL_SCANCODE_B, true));
	Feed(in, Key(SDL_SCANCODE_B, false));
	in->ReadKeyboard(16);
	CHECK(!in->KeyPress(DIK_B) && !in->KeyDown(DIK_B));
}

static void TestConsume(InputManager* in)
{
	Feed(in, Key(SDL_SCANCODE_SPACE, true));
	in->ReadKeyboard(16);
	CHECK(in->KeyDown(DIK_SPACE));
	in->ConsumeKey(DIK_SPACE);
	CHECK(!in->KeyPress(DIK_SPACE));   // consumed for the rest of this frame
	CHECK(!in->KeyDown(DIK_SPACE));
	in->ReadKeyboard(16);
	CHECK(in->KeyPress(DIK_SPACE));    // consumption does not survive the next ReadKeyboard
	Feed(in, Key(SDL_SCANCODE_SPACE, false));
	in->ReadKeyboard(16);
}

static void TestExtendedAndUnmappedKeys(InputManager* in)
{
	Feed(in, Key(SDL_SCANCODE_UP, true));
	Feed(in, Key(SDL_SCANCODE_RCTRL, true));
	Feed(in, Key(SDL_SCANCODE_RETURN, true));
	in->ReadKeyboard(16);
	CHECK(in->KeyDown(DIK_UP));
	CHECK(in->KeyDown(DIK_RCONTROL));
	CHECK(in->KeyDown(DIK_RETURN));
	CHECK(!in->KeyPress(DIK_DOWN));
	Feed(in, Key(SDL_SCANCODE_UP, false));
	Feed(in, Key(SDL_SCANCODE_RCTRL, false));
	Feed(in, Key(SDL_SCANCODE_RETURN, false));

	// A key with no DIK mapping must be ignored, not crash or alias another key
	Feed(in, Key(SDL_SCANCODE_MEDIA_PLAY, true));
	Feed(in, Key(SDL_SCANCODE_UNKNOWN, true));
	in->ReadKeyboard(16);
	for (int k = 1; k < 256; ++k) CHECK(!in->KeyPress(k));
}

static void TestFocusLoss(InputManager* in)
{
	Feed(in, Key(SDL_SCANCODE_W, true));
	Feed(in, Button(SDL_BUTTON_LEFT, true));
	in->ReadKeyboard(16);
	in->ReadMouse(16);
	CHECK(in->KeyPress(DIK_W));
	CHECK(in->MousePress(InputManager::Left));

	SDL_Event lost{};
	lost.type = SDL_EVENT_WINDOW_FOCUS_LOST;
	Feed(in, lost);                  // key/button released while another window had focus
	in->ReadKeyboard(16);
	in->ReadMouse(16);
	CHECK(!in->KeyPress(DIK_W));
	CHECK(in->KeyUp(DIK_W));
	CHECK(!in->MousePress(InputManager::Left));
	CHECK(in->MouseUp(InputManager::Left));
}

static void TestMouseButtons(InputManager* in)
{
	in->ReadMouse(16);
	CHECK(!in->MousePress(InputManager::Left));

	Feed(in, Button(SDL_BUTTON_LEFT, true));
	CHECK(!in->MousePress(InputManager::Left));   // not visible until the next ReadMouse
	in->ReadMouse(16);
	CHECK(in->MousePress(InputManager::Left));
	CHECK(in->MouseDown(InputManager::Left));
	CHECK(!in->MouseUp(InputManager::Left));

	in->ReadMouse(16);
	CHECK(in->MousePress(InputManager::Left));
	CHECK(!in->MouseDown(InputManager::Left));

	Feed(in, Button(SDL_BUTTON_LEFT, false));
	in->ReadMouse(16);
	CHECK(!in->MousePress(InputManager::Left));
	CHECK(in->MouseUp(InputManager::Left));

	Feed(in, Button(SDL_BUTTON_RIGHT, true));
	Feed(in, Button(SDL_BUTTON_MIDDLE, true));
	in->ReadMouse(16);
	CHECK(in->MouseDown(InputManager::Right));
	CHECK(in->MouseDown(InputManager::Middle));
	CHECK(!in->MousePress(InputManager::Left));
	in->ConsumeMouseButton(InputManager::Right);
	CHECK(!in->MousePress(InputManager::Right));
	Feed(in, Button(SDL_BUTTON_RIGHT, false));
	Feed(in, Button(SDL_BUTTON_MIDDLE, false));
	in->ReadMouse(16);
}

static void TestWheel(InputManager* in)
{
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == 0);

	Feed(in, Wheel(1.0f));
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == 120);   // one notch, up/away from the user
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == 0);     // consumed

	Feed(in, Wheel(-2.0f));
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == -240);

	Feed(in, Wheel(1.0f));
	Feed(in, Wheel(1.0f));                // several events in one frame accumulate
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == 240);

	Feed(in, Wheel(0.5f));                // high-resolution wheels report fractions
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == 60);

	Feed(in, Wheel(1.0f, /*flipped*/ true));  // natural scrolling reports the opposite sign
	in->ReadMouse(16);
	CHECK(in->GetMouseScroll() == -120);
}

static void TestMousePosition(InputManager* in)
{
	// Window is 960x720 and so is the virtual screen: no letterboxing, scale 1
	Feed(in, Motion(100.0f, 50.0f));
	in->ReadMouse(16);
	CHECK(in->GetAbsoluteMouseX() == 100);
	CHECK(in->GetAbsoluteMouseY() == 50);

	Feed(in, Motion(130.0f, 40.0f));
	in->ReadMouse(16);
	CHECK(in->GetAbsoluteMouseX() == 130);
	CHECK(in->GetRelativeMouseX() == 30);
	CHECK(in->GetRelativeMouseY() == -10);

	in->ReadMouse(16);                      // no movement
	CHECK(in->GetRelativeMouseX() == 0 && in->GetRelativeMouseY() == 0);

	Feed(in, Motion(5000.0f, -20.0f));      // far outside: clamped to the drawable region
	in->ReadMouse(16);
	CHECK(in->GetAbsoluteMouseX() == 960);
	CHECK(in->GetAbsoluteMouseY() == 0);

	sdlgf::Camera camera(960, 720);
	Feed(in, Motion(480.0f, 360.0f));
	in->ReadMouse(16);
	auto [vx, vy] = in->GetVirtualAbsoluteMousePos(&camera);
	CHECK(vx == 480.0f && vy == 360.0f);
}

static void TestLetterbox(sdlgf::Application* app, InputManager* in)
{
	// Make the window twice as wide as the virtual screen: 480 px bars on each side
	SDL_SetWindowSize(app->GetWindow(), 1920, 720);
	SDL_SyncWindow(app->GetWindow());
	int w = 0, h = 0;
	app->GetClientSize(w, h);
	if (w != 1920 || h != 720) {
		std::fprintf(stderr, "FAIL: dummy driver did not resize the window (got %dx%d)\n", w, h);
		++g_failures;
		return;
	}

	Feed(in, Motion(1000.0f, 100.0f));
	in->ReadMouse(16);
	CHECK(in->GetAbsoluteMouseX() == 1000);   // inside the 480..1440 content region

	sdlgf::Camera camera(960, 720);
	auto [vx, vy] = in->GetVirtualAbsoluteMousePos(&camera);
	CHECK(vx == 520.0f);                      // (1000 - 480) / scale 1
	CHECK(vy == 100.0f);

	Feed(in, Motion(100.0f, 100.0f));         // inside the left bar: clamped to its edge
	in->ReadMouse(16);
	CHECK(in->GetAbsoluteMouseX() == 480);
	auto [lx, ly] = in->GetVirtualAbsoluteMousePos(&camera);
	CHECK(lx == 0.0f);

	// Relative movement is scaled by the same factor (1.0 here)
	Feed(in, Motion(1100.0f, 100.0f));
	in->ReadMouse(16);
	Feed(in, Motion(1140.0f, 120.0f));
	in->ReadMouse(16);
	auto [dx, dy] = in->GetVirtualRelativeMousePos(&camera);
	CHECK(dx == 40.0f && dy == 20.0f);

	SDL_SetWindowSize(app->GetWindow(), 960, 720);
	SDL_SyncWindow(app->GetWindow());
}

int main()
{
	// Must happen before SDL_Init (done by Application::Init)
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");

	auto app = sdlgf::Application::GetInstance();
	try {
		app->Init(L"input test", 960, 720, false);
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "Application::Init failed: %s\n", e.what());
		return 1;
	}

	CHECK(app->GetWindow() != nullptr);
	int w = 0, h = 0;
	app->GetClientSize(w, h);
	CHECK(w == 960 && h == 720);
	CHECK(app->GetScreenWidth() == 960 && app->GetScreenHeight() == 720);
	CHECK(app->GetPixelDensity() > 0.0f);

	app->SetTitle(L"\u0110\u00E2y l\u00E0 ti\u00EAu \u0111\u1EC1");   // non-ASCII title must not crash
	CHECK(std::string(SDL_GetWindowTitle(app->GetWindow())) == "\xC4\x90\xC3\xA2y l\xC3\xA0 ti\xC3\xAAu \xC4\x91\xE1\xBB\x81");

	auto in = InputManager::GetInstance();
	TestKeyboardSemantics(in);
	TestConsume(in);
	TestExtendedAndUnmappedKeys(in);
	TestFocusLoss(in);
	TestMouseButtons(in);
	TestWheel(in);
	TestMousePosition(in);
	TestLetterbox(app, in);

	app->Quit();   // sets the flag Run() polls; nothing to observe without running the loop
	SDL_DestroyWindow(app->GetWindow());
	SDL_Quit();

	if (g_failures == 0) std::puts("input_test: all tests passed");
	else std::fprintf(stderr, "input_test: %d check(s) failed\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
