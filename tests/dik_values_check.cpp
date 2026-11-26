// Windows-only compile check: DX9GFKeys.h must agree with the real <dinput.h>.
//
// Both headers #define the same DIK_* names. If a value (or an alias) in DX9GFKeys.h
// differed from the Windows SDK, the second definition would be a macro redefinition
// with different tokens (C4005), which this target compiles as an error (/we4005).
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "DX9GFKeys.h"

// A few spot checks that survive even if the macros were somehow identical text.
static_assert(DIK_ESCAPE == 0x01 && DIK_UP == 0xC8 && DIK_NUMPADENTER == 0x9C, "DIK values");

int sdlgf_dik_values_check() { return 0; }
