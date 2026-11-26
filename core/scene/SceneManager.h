#pragma once
#include <memory>
#include <vector>

namespace sdlgf {
	class IScene;

	constexpr auto NO_INDEX = -1;

	class SceneManager final {
	private:
		int index = NO_INDEX;
		std::vector<IScene*> scenes;
		// A scene that takes over the screen while the stack is being (re)built. It is not part of
		// the stack, so stack indices are unaffected while it runs.
		std::unique_ptr<IScene> loadingScene;
		bool endLoadingRequested = false;
	protected:
		void Dispose();
	public:
		~SceneManager();
		int GetIndex() const { return index; }
		/// Shows `scene` instead of the stack (which neither updates nor draws) until EndLoading().
		/// While it is shown GetCurrentScene() returns it. Typically a loading screen that rebuilds
		/// the stack from its Update.
		void BeginLoading(std::unique_ptr<IScene> scene);
		/// Ends the loading scene once the current Update returns (safe to call from inside it).
		void EndLoading() { endLoadingRequested = true; }
		bool IsLoading() const { return loadingScene != nullptr; }
		IScene* GetCurrentScene();
		IScene* GetScene(size_t index);
		void ChangeScene(IScene*);
		void PushScene(IScene*);
		void InsertScene(size_t index, IScene* scene);
		void PopScene();
		void RemoveScene(size_t index);
		void Update(unsigned long long deltaTime);
		void DrawWorld(unsigned long long deltaTime);
		void DrawUI(unsigned long long deltaTime);
		void OnResize(int width, int height);
		void OnDeviceReset();
		void GoToNext();
		void GoToPrevious();
		void GoToScene(size_t index);
		size_t GetSceneCount() const;
	};
}