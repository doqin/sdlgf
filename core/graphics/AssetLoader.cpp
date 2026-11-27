#include "pch.h"
#include "AssetLoader.h"
#include <SDL3/SDL.h>
#include <unordered_set>

sdlgf::AssetLoader::AssetLoader(AssetManager& assets) : assets(assets)
{
	worker = std::thread([this] { WorkerMain(); });
}

sdlgf::AssetLoader::~AssetLoader()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		stopping = true;
	}
	wake.notify_all();
	if (worker.joinable()) worker.join();
}

void sdlgf::AssetLoader::WorkerMain()
{
	Platform::SdlThreadCleanup sdlCleanup; // IMG_Load sets SDL's per-thread error buffer
	for (;;) {
		std::wstring path;
		{
			std::unique_lock<std::mutex> lock(mutex);
			wake.wait(lock, [this] { return stopping || (!pending.empty() && ready.size() < kMaxDecodedAhead); });
			if (stopping) return;
			path = std::move(pending.front());
			pending.pop_front();
		}
		Decoded decoded{ path, nullptr, {} };
		try {
			decoded.surface = Texture::DecodeFile(path);
		}
		catch (const std::exception& e) {
			decoded.error = e.what();
		}
		{
			std::lock_guard<std::mutex> lock(mutex);
			ready.push_back(std::move(decoded));
		}
	}
}

void sdlgf::AssetLoader::Request(const std::vector<std::wstring>& paths)
{
	std::unordered_set<std::wstring> queued;
	{
		std::lock_guard<std::mutex> lock(mutex);
		queued.insert(pending.begin(), pending.end());
	}
	std::vector<std::wstring> toQueue;
	for (const auto& path : paths) {
		if (!queued.insert(path).second) continue;
		++total;
		// Already in memory: just keep it alive with the rest of the batch
		if (auto existing = assets.FindTexture(path)) {
			held.Add(std::move(existing));
			++completed;
			continue;
		}
		toQueue.push_back(path);
	}
	if (toQueue.empty()) return;
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto& path : toQueue) pending.push_back(std::move(path));
	}
	wake.notify_all();
}

bool sdlgf::AssetLoader::Pump(unsigned long long budgetMs)
{
	const unsigned long long start = SDL_GetTicks();
	bool uploadedAny = false;
	while (completed < total) {
		Decoded item;
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (ready.empty()) break;
			item = std::move(ready.front());
			ready.pop_front();
		}
		wake.notify_all(); // the worker may have been waiting for room in `ready`
		if (item.surface) {
			// Someone may have created it synchronously while it was in flight
			auto texture = assets.FindTexture(item.path);
			if (!texture) {
				texture = std::make_shared<Texture>(assets.GetGraphicsDevice());
				try {
					texture->LoadFromSurface(item.surface.get(), Platform::WideToUtf8(item.path));
					texture = assets.AdoptTexture(item.path, std::move(texture));
				}
				catch (const std::exception& e) {
					failures.push_back(e.what());
					texture.reset();
				}
			}
			if (texture) held.Add(std::move(texture));
		}
		else {
			failures.push_back(item.error);
		}
		++completed;
		uploadedAny = true;
		if (SDL_GetTicks() - start >= budgetMs) break;
	}
	(void)uploadedAny;
	return IsDone();
}
