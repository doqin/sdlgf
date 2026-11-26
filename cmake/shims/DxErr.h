#pragma once
// Stand-in for the legacy DirectX SDK's DxErr.h, which is not part of the
// Microsoft.DXSDK.D3DX NuGet package. Only DXGetErrorDescription is used by the
// code base. Goes away together with the Direct3D 9 renderer.
#include <windows.h>
#include <wchar.h>

inline const WCHAR* DXGetErrorDescription(HRESULT hr)
{
	static thread_local WCHAR buffer[512];
	DWORD length = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, static_cast<DWORD>(hr), 0, buffer,
		static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), nullptr);
	while (length > 0 && (buffer[length - 1] == L'\r' || buffer[length - 1] == L'\n' || buffer[length - 1] == L' ')) {
		buffer[--length] = L'\0';
	}
	if (length == 0) {
		swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"HRESULT 0x%08X", static_cast<unsigned int>(hr));
	}
	return buffer;
}
