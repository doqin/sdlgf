#pragma once
#include "sdlgf.h"
#include "Extras.h"
#include <memory>
#include <functional>
namespace Demo
{
	class IButton : public sdlgf::IGameObject
	{
	public:
		//button state to change its appearance, have to set it public for scene to use
		enum class ButtonState { IDLE, HOVER, CLICKED, DISABLED, LISTENING };
	protected:
		ButtonState currentState;

		float width;
		float height;

		//to reuse component as buttons may have different frame counts.
		int frameCount;

		std::shared_ptr<sdlgf::RectangleTrigger> trigger;
		std::function<void(sdlgf::ITrigger*)> callback;
		sdlgf::Camera* uiCamera = nullptr;

		std::string btnHoverSfxName = "btn_hover";
		std::string btnClickSfxName = "btn_click";
	public:

		IButton(std::shared_ptr<sdlgf::TransformManager> tm, float x, float y, float w, float h, int frames = 3)
			: IGameObject(tm, x, y)
		{
			this->width = w;
			this->height = h;
			this->currentState = ButtonState::IDLE;
			this->frameCount = frames;
		}

		virtual void Init(sdlgf::Camera* uiCamera) = 0;
		//abstract function to determine whether the button displays text or an icon
		virtual void Draw(sdlgf::GraphicsDevice* gd, unsigned long long deltaTime) = 0;


		virtual void SetOnReleaseLeft(std::function<void(sdlgf::ITrigger*)> cb);

		//check if the cursor is hovering over the button.
		virtual void Update(unsigned long long deltaTime);

		//programmatically invokes the button's click callback (used by keyboard navigation).
		void Activate();

		//plays this button's hover sound (used by keyboard navigation when the cursor lands on it).
		void PlayHoverSfx();

		//resizes the button and keeps its trigger box in sync.
		virtual void SetSize(float w, float h)
		{
			this->width = w;
			this->height = h;
			if (this->trigger) this->trigger->SetSize(w, h);
		}

		void SetState(ButtonState state) { this->currentState = state; }
		ButtonState GetState() const { return this->currentState; }
		std::shared_ptr<sdlgf::RectangleTrigger> GetTrigger() { return this->trigger; }

		float GetWidth() const { return this->width; }
		float GetHeight() const { return this->height; }
	};
}
