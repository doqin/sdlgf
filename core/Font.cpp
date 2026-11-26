#include "pch.h"
#include "Font.h"
#include <windows.h>
#include <DxErr.h>
#include <stdexcept>

sdlgf::Font::Font(GraphicsDevice* graphicsDevice, std::wstring fontName, INT height, INT width, UINT weight, UINT mipLevels, BOOL italic, DWORD charSet, DWORD outputPrecision, DWORD quality, DWORD pitchAndFamily) : graphicsDevice(graphicsDevice)
{
	auto result = D3DXCreateFont(graphicsDevice->GetDevice(), height, width, weight, mipLevels, italic, charSet, outputPrecision, quality, pitchAndFamily, fontName.c_str(), &font);
	if (FAILED(result)) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::invalid_argument(what);
	}
	graphicsDevice->RegisterVolatileResource(this);
}

sdlgf::Font::~Font()
{
	if (graphicsDevice != nullptr) graphicsDevice->UnregisterVolatileResource(this);
	if (font != nullptr) font->Release();
}

void sdlgf::Font::OnLostDevice()
{
	if (font != nullptr) font->OnLostDevice();
}

void sdlgf::Font::OnResetDevice()
{
	if (font != nullptr) font->OnResetDevice();
}

ID3DXFont* sdlgf::Font::GetRawFont()
{
	return font;
}

sdlgf::GraphicsDevice* sdlgf::Font::GetGraphicsDevice()
{
	return graphicsDevice;
}

std::vector<std::wstring> sdlgf::Font::addedFonts;

void sdlgf::Font::AddFont(std::wstring path)
{
	int result = AddFontResourceExW(
		path.c_str(),
		FR_PRIVATE,
		0
	);

	if (result > 0)
	{
		addedFonts.push_back(path);
	}
}

void sdlgf::Font::RemoveFont(std::wstring path)
{
	RemoveFontResourceExW(
		path.c_str(),
		FR_PRIVATE,
		0
	);
}

void sdlgf::Font::RemoveAllFonts()
{
	for (auto& font : addedFonts)
	{
		RemoveFontResourceExW(
			font.c_str(),
			FR_PRIVATE,
			0
		);
	}

	addedFonts.clear();
}
