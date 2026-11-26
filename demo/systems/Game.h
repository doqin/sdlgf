#pragma once
#include "sdlgf.h"

namespace Demo {
	static const wchar_t* const GAME_TITLE =
		L"T\u00f4i, sinh vi\u00ean UIT n\u0103m 6 b\u1ecb h\u00fat v\u00e0o cyberspace v\u00ec click v\u00e0o link \u0111\u1ed9c";

	class Game : public sdlgf::IGame {
	public:
	Game(const UINT screenWidth, const UINT screenHeight) : IGame(screenWidth, screenHeight) {}
	void Init() override;
	void Update(unsigned long long deltaTime) override;
	/// Builds every world scene once while logging the textures it requests, and writes one manifest
	/// per world into `dir` (what the loading screen preloads). Returns false if a file cannot be written.
	bool RecordAssetManifests(const std::string& dir);
	UINT GetVirtualWidth() const { return SCREEN_WIDTH; }
	UINT GetVirtualHeight() const { return SCREEN_HEIGHT; }

	private:
		unsigned long long fpsAccumulator = 0;
		int fpsFrames = 0;
	};

}