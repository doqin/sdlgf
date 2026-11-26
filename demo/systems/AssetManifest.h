#pragma once
#include <string>
#include <vector>

namespace Demo {
	/// A manifest is a UTF-8 text file with one asset path per line (blank lines and lines starting
	/// with '#' are ignored). It lists what a group of scenes will ask the AssetManager for, so the
	/// loading screen can stream those files in before the scenes are built. A missing file or a
	/// stale entry is harmless: whatever is not preloaded is simply loaded on demand.
	///
	/// Regenerate with `demo --record-assets <file>` (see Game::RecordAssetManifest).
	std::vector<std::wstring> ReadAssetManifest(const std::string& file);
	bool WriteAssetManifest(const std::string& file, const std::vector<std::wstring>& paths);
}
