// Tests for DX9GFKeys: DIK <-> SDL scancode translation.
#include "DX9GFKeys.h"

#include <SDL3/SDL_scancode.h>

#include <cstdio>

static int g_failures = 0;
#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

using DX9GF::Keys::FromSDLScancode;
using DX9GF::Keys::ToSDLScancode;

static void TestKnownKeys()
{
	// Every DIK the game actually uses, spot-checked by meaning rather than by table row.
	CHECK(FromSDLScancode(SDL_SCANCODE_A) == DIK_A);
	CHECK(FromSDLScancode(SDL_SCANCODE_Z) == DIK_Z);
	CHECK(FromSDLScancode(SDL_SCANCODE_1) == DIK_1);
	CHECK(FromSDLScancode(SDL_SCANCODE_0) == DIK_0);
	CHECK(FromSDLScancode(SDL_SCANCODE_ESCAPE) == DIK_ESCAPE);
	CHECK(FromSDLScancode(SDL_SCANCODE_RETURN) == DIK_RETURN);
	CHECK(FromSDLScancode(SDL_SCANCODE_SPACE) == DIK_SPACE);
	CHECK(FromSDLScancode(SDL_SCANCODE_BACKSPACE) == DIK_BACK);
	CHECK(FromSDLScancode(SDL_SCANCODE_TAB) == DIK_TAB);
	CHECK(FromSDLScancode(SDL_SCANCODE_LSHIFT) == DIK_LSHIFT);
	CHECK(FromSDLScancode(SDL_SCANCODE_RSHIFT) == DIK_RSHIFT);
	CHECK(FromSDLScancode(SDL_SCANCODE_LCTRL) == DIK_LCONTROL);
	CHECK(FromSDLScancode(SDL_SCANCODE_RCTRL) == DIK_RCONTROL);   // 0xE0-prefixed on a real keyboard
	CHECK(FromSDLScancode(SDL_SCANCODE_LALT) == DIK_LMENU);
	CHECK(FromSDLScancode(SDL_SCANCODE_RALT) == DIK_RMENU);
	CHECK(FromSDLScancode(SDL_SCANCODE_UP) == DIK_UP);
	CHECK(FromSDLScancode(SDL_SCANCODE_DOWN) == DIK_DOWN);
	CHECK(FromSDLScancode(SDL_SCANCODE_LEFT) == DIK_LEFT);
	CHECK(FromSDLScancode(SDL_SCANCODE_RIGHT) == DIK_RIGHT);
	CHECK(FromSDLScancode(SDL_SCANCODE_HOME) == DIK_HOME);
	CHECK(FromSDLScancode(SDL_SCANCODE_END) == DIK_END);
	CHECK(FromSDLScancode(SDL_SCANCODE_PAGEUP) == DIK_PRIOR);
	CHECK(FromSDLScancode(SDL_SCANCODE_PAGEDOWN) == DIK_NEXT);
	CHECK(FromSDLScancode(SDL_SCANCODE_INSERT) == DIK_INSERT);
	CHECK(FromSDLScancode(SDL_SCANCODE_DELETE) == DIK_DELETE);
	CHECK(FromSDLScancode(SDL_SCANCODE_F1) == DIK_F1);
	CHECK(FromSDLScancode(SDL_SCANCODE_F10) == DIK_F10);
	CHECK(FromSDLScancode(SDL_SCANCODE_F12) == DIK_F12);
	CHECK(FromSDLScancode(SDL_SCANCODE_KP_0) == DIK_NUMPAD0);
	CHECK(FromSDLScancode(SDL_SCANCODE_KP_9) == DIK_NUMPAD9);
	CHECK(FromSDLScancode(SDL_SCANCODE_KP_ENTER) == DIK_NUMPADENTER);
	CHECK(FromSDLScancode(SDL_SCANCODE_KP_DIVIDE) == DIK_DIVIDE);
	CHECK(FromSDLScancode(SDL_SCANCODE_MINUS) == DIK_MINUS);
	CHECK(FromSDLScancode(SDL_SCANCODE_EQUALS) == DIK_EQUALS);
	CHECK(FromSDLScancode(SDL_SCANCODE_LEFTBRACKET) == DIK_LBRACKET);
	CHECK(FromSDLScancode(SDL_SCANCODE_RIGHTBRACKET) == DIK_RBRACKET);
	CHECK(FromSDLScancode(SDL_SCANCODE_SEMICOLON) == DIK_SEMICOLON);
	CHECK(FromSDLScancode(SDL_SCANCODE_APOSTROPHE) == DIK_APOSTROPHE);
	CHECK(FromSDLScancode(SDL_SCANCODE_COMMA) == DIK_COMMA);
	CHECK(FromSDLScancode(SDL_SCANCODE_PERIOD) == DIK_PERIOD);
	CHECK(FromSDLScancode(SDL_SCANCODE_SLASH) == DIK_SLASH);
	CHECK(FromSDLScancode(SDL_SCANCODE_BACKSLASH) == DIK_BACKSLASH);
	// DirectInput convention (SDL's own Windows table would say Num Lock): Pause is 0xC5
	CHECK(FromSDLScancode(SDL_SCANCODE_PAUSE) == DIK_PAUSE);
	CHECK(FromSDLScancode(SDL_SCANCODE_NUMLOCKCLEAR) == DIK_NUMLOCK);
}

static void TestUnknownAndOutOfRange()
{
	CHECK(FromSDLScancode(SDL_SCANCODE_UNKNOWN) == 0);
	CHECK(FromSDLScancode(-1) == 0);
	CHECK(FromSDLScancode(SDL_SCANCODE_COUNT) == 0);
	CHECK(FromSDLScancode(100000) == 0);
	CHECK(ToSDLScancode(0) == SDL_SCANCODE_UNKNOWN);
	CHECK(ToSDLScancode(-5) == SDL_SCANCODE_UNKNOWN);
	CHECK(ToSDLScancode(DX9GF::Keys::KeyCount) == SDL_SCANCODE_UNKNOWN);
	CHECK(ToSDLScancode(0xFF) == SDL_SCANCODE_UNKNOWN);   // no key has this ID
	CHECK(FromSDLScancode(SDL_SCANCODE_MEDIA_PLAY) == 0);   // media key with no DIK
}

static void TestRoundTrip()
{
	int mapped = 0;
	for (int dik = 1; dik < DX9GF::Keys::KeyCount; ++dik) {
		const int sdl = ToSDLScancode(dik);
		if (sdl == 0) continue;
		++mapped;
		if (FromSDLScancode(sdl) != dik) {
			std::fprintf(stderr, "  DIK 0x%02X -> SDL %d -> DIK 0x%02X\n", dik, sdl, FromSDLScancode(sdl));
			CHECK(!"DIK -> SDL -> DIK round trip");
		}
	}
	CHECK(mapped >= 120); // 133 at time of writing; catches an accidentally emptied table

	for (int sdl = 1; sdl < SDL_SCANCODE_COUNT; ++sdl) {
		const int dik = FromSDLScancode(sdl);
		if (dik == 0) continue;
		if (ToSDLScancode(dik) != sdl) {
			std::fprintf(stderr, "  SDL %d -> DIK 0x%02X -> SDL %d\n", sdl, dik, ToSDLScancode(dik));
			CHECK(!"SDL -> DIK -> SDL round trip");
		}
	}
}

int main()
{
	TestKnownKeys();
	TestUnknownAndOutOfRange();
	TestRoundTrip();
	if (g_failures == 0) std::puts("keys_test: all tests passed");
	else std::fprintf(stderr, "keys_test: %d check(s) failed\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
