// Compile-only check of the non-Windows branch of Platform.h.
//
// SDLGF_PLATFORM_FORCE_PORTABLE_TYPES makes the header define the Win32 stand-in types
// itself even on Windows, so we can verify here that their layout matches the real
// Win32 ABI (the part of the portable branch that the rest of the code relies on).
// This translation unit is built as its own OBJECT library and never linked with
// anything that includes <windows.h>, so the duplicate type names cannot clash.
#define SDLGF_PLATFORM_FORCE_PORTABLE_TYPES
#include "Platform.h"

static_assert(sizeof(BYTE) == 1, "BYTE");
static_assert(sizeof(WORD) == 2, "WORD");
static_assert(sizeof(DWORD) == 4, "DWORD");
static_assert(sizeof(UINT) == 4, "UINT");
static_assert(sizeof(LONG) == 4, "LONG must be 32-bit like Win32, even on LP64");
static_assert(sizeof(BOOL) == 4, "BOOL");
static_assert(sizeof(HRESULT) == 4, "HRESULT");

static_assert(sizeof(RECT) == 16 && alignof(RECT) == 4, "RECT layout");
static_assert(sizeof(POINT) == 8 && alignof(POINT) == 4, "POINT layout");
static_assert(sizeof(SIZE) == 8 && alignof(SIZE) == 4, "SIZE layout");

// Win32 RECT member order: left, top, right, bottom (designated initializers are used in Utils).
constexpr RECT kRect = { .left = 1, .top = 2, .right = 3, .bottom = 4 };
static_assert(kRect.left == 1 && kRect.top == 2 && kRect.right == 3 && kRect.bottom == 4, "RECT order");

static_assert(SUCCEEDED(S_OK) && !FAILED(S_OK), "S_OK");
static_assert(FAILED(E_FAIL) && !SUCCEEDED(E_FAIL), "E_FAIL");
static_assert(TRUE == 1 && FALSE == 0, "TRUE/FALSE");

// Exists so the OBJECT library has a symbol and is never empty.
int sdlgf_portable_types_check() { return 0; }
