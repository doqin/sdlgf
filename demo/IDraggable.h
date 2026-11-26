#pragma once
#include "Extras.h"
#include "sdlgf.h"

namespace Demo {
	class DraggableManager;
  class IDraggable : virtual public sdlgf::IGameObject {
	protected:
		std::weak_ptr<sdlgf::IGameObject> preDragParent;
		float preDragWorldX = 0;
		float preDragWorldY = 0;
		inline static size_t nextID = 0;
		std::string id;
		size_t dragAreaWidth;
		size_t dragAreaHeight;
		std::shared_ptr<sdlgf::RectangleTrigger> trigger;
		std::shared_ptr<Demo::DraggableManager> draggableManager;
		sdlgf::Camera* camera = nullptr;
		sdlgf::GraphicsDevice* graphicsDevice = nullptr;
		D3DXCOLOR color;
		static std::shared_ptr<sdlgf::Font> debugFont;
		static std::shared_ptr<sdlgf::FontSprite> debugFontSprite;
		bool isDragging = false;
		RECT scissorRect;
		bool isCropped = false;
		bool isHidden = false;
		bool dragDisabled = false;
	public:
		inline static bool debug = false;
		inline IDraggable(std::weak_ptr<sdlgf::TransformManager> transformManager) : IGameObject(transformManager), dragAreaWidth(0), dragAreaHeight(0) { 
			id = "draggable" + std::to_string(nextID++); 
		}
		inline IDraggable(
			std::weak_ptr<sdlgf::TransformManager> transformManager, 
			size_t dragAreaWidth, 
			size_t dragAreaHeight, 
			float x = 0, 
			float y = 0, 
			float rotation = 0, 
			float scaleX = 1, 
			float scaleY = 1
		) : IGameObject(transformManager, x, y, rotation, scaleX, scaleY), 
			dragAreaWidth(dragAreaWidth), 
			dragAreaHeight(dragAreaHeight) { 
			id = "draggable" + std::to_string(nextID++);
		}
		inline IDraggable(
			std::weak_ptr<sdlgf::TransformManager> transformManager,
			std::weak_ptr<sdlgf::IGameObject> parent,
			size_t dragAreaWidth,
			size_t dragAreaHeight,
			float x = 0,
			float y = 0,
			float rotation = 0,
			float scaleX = 1,
			float scaleY = 1
		) : IGameObject(transformManager, parent, x, y, rotation, scaleX, scaleY),
			dragAreaWidth(dragAreaWidth),
			dragAreaHeight(dragAreaHeight) {
			id = "draggable" + std::to_string(nextID++);
		}
		virtual void Init(std::shared_ptr<DraggableManager> manager, sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera);
		void SetParent(std::weak_ptr<IGameObject> parent);
		void DetachParent();
		virtual void Update(unsigned long long deltaTime);
		virtual void Draw(unsigned long long deltaTime);
		virtual bool OnDrop(std::shared_ptr<IDraggable> other);
		virtual bool OnHover(std::shared_ptr<IDraggable> other);
		virtual void OnDropMissed(std::weak_ptr<sdlgf::IGameObject> oldParent, float oldWorldX, float oldWorldY);
		using DropMissedCallback = std::function<void(std::shared_ptr<IDraggable>)>;
		DropMissedCallback onDropMissedHandler;
		void SetOnDropMissedHandler(DropMissedCallback handler) { onDropMissedHandler = handler; }
		// With dragging disabled, a left click (release) invokes the click handler instead.
		using ClickCallback = std::function<void(std::shared_ptr<IDraggable>)>;
		ClickCallback onClickHandler;
		void SetOnClickHandler(ClickCallback handler) { onClickHandler = handler; }
		void SetDragDisabled(bool disabled) { dragDisabled = disabled; }
		std::weak_ptr<sdlgf::IGameObject> GetPreDragParent() const { return preDragParent; }
		virtual bool TryReclaim(std::weak_ptr<sdlgf::IGameObject> oldParent) { return false; }
		virtual bool CanBeStoredInContainer() const { return true; }

		void SetScissorRect(const RECT& rect);
		void ClearScissorRect();
		std::string GetID() const;
		virtual size_t GetWidth() const;
		virtual size_t GetHeight() const;
		std::weak_ptr<DraggableManager> GetDraggableManager();
		std::weak_ptr<sdlgf::RectangleTrigger> GetTrigger();
		bool IsDragging() const;
		bool IsHidden() const;
		void SetHidden(bool hidden);
	};

	class DraggableManager final {
	private:
		struct LevelBatch{
			size_t startIdx;
			size_t endIdx;
		};
		std::vector<std::weak_ptr<IDraggable>> hierarchy;
		std::vector<LevelBatch> levels;
		std::unordered_map<std::string, std::shared_ptr<IDraggable>> objectMap;
		std::vector<std::shared_ptr<IDraggable>> isDraggingDraggables;
		sdlgf::CommandBuffer drawBuffer;
		// While set, Add/Remove and IDraggable::SetParent/DetachParent skip the O(n^2)
		// RebuildHierarchy(); the batch's owner flips it back off (which rebuilds once).
		bool deferRebuild = false;
	public:
		void RebuildHierarchy();
		// Batch structural changes: SetDeferRebuild(true) before a burst of Add/Remove/SetParent,
		// SetDeferRebuild(false) after (it runs one RebuildHierarchy). Transform-side rebuilds are
		// suppressed too, so the caller must run transformManager->RebuildHierarchy() once itself.
		void SetDeferRebuild(bool defer);
		bool IsDeferRebuild() const { return deferRebuild; }
		void Add(std::shared_ptr<IDraggable> obj);
		void Remove(std::shared_ptr<IDraggable> obj);
		void HoverDroppable(std::shared_ptr<IDraggable> obj);
		void AttachDroppable(std::shared_ptr<IDraggable> obj);
		void Update(unsigned long long deltaTime);
		void Draw(unsigned long long deltaTime);
		void QueueDraw(std::shared_ptr<sdlgf::ICommand> cmd);
		// Draw() flushes the queue itself. Callers that draw draggables directly, without going
		// through Draw(), must flush too - otherwise queued commands outlive the objects that
		// queued them and run against freed memory.
		void FlushQueuedDraws(unsigned long long deltaTime);
		void ClearQueuedDraws();
		bool IsQueueBusy();
		std::vector<std::shared_ptr<IDraggable>> GetDraggingDraggables() const;
	};
}