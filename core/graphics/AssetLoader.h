#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "AssetManager.h"
#include "Texture.h"

namespace sdlgf {
	/// Loads a list of textures into the AssetManager without stalling the frame loop.
	///
	/// A worker thread does the slow part (file read + image decode into RAM, Texture::DecodeFile);
	/// the main thread, which is the only one allowed to touch the renderer, calls Pump() once per
	/// frame to upload what is ready within a small time budget. The loaded textures are held by
	/// the loader (so the pool does not free them) until the loader is destroyed - keep it alive
	/// until the objects that need those textures have been created and hold their own references.
	///
	/// A file that cannot be decoded is recorded in Failures() and skipped; the normal
	/// AssetManager::GetTexture call that later wants it will throw with the file name.
	class AssetLoader final {
	private:
		struct Decoded {
			std::wstring path;
			SurfacePtr surface; // null if decoding failed
			std::string error;
		};

		AssetManager& assets;
		AssetSet held;

		std::mutex mutex;
		std::condition_variable wake;
		std::deque<std::wstring> pending;  // waiting to be decoded
		std::deque<Decoded> ready;         // decoded, waiting for upload
		bool stopping = false;
		std::thread worker;

		size_t total = 0;
		size_t completed = 0; // main thread only
		std::vector<std::string> failures;

		static constexpr size_t kMaxDecodedAhead = 8; // bounds RAM held by images not yet uploaded
		void WorkerMain();
	public:
		explicit AssetLoader(AssetManager& assets);
		~AssetLoader();
		AssetLoader(const AssetLoader&) = delete;
		AssetLoader& operator=(const AssetLoader&) = delete;

		/// Queues textures to load. Paths already in memory (or queued) are skipped.
		void Request(const std::vector<std::wstring>& paths);

		/// Main thread, once per frame: uploads decoded images to the GPU until about
		/// `budgetMs` milliseconds have been spent (always at least one). Returns IsDone().
		bool Pump(unsigned long long budgetMs = 4);

		bool IsDone() const { return completed >= total; }
		size_t GetTotal() const { return total; }
		size_t GetCompleted() const { return completed; }
		/// 0..1; 1 when there was nothing to load.
		float GetProgress() const { return total == 0 ? 1.0f : static_cast<float>(completed) / static_cast<float>(total); }
		const std::vector<std::string>& GetFailures() const { return failures; }
	};
}
