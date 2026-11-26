#pragma once
#include "DX9GFPlatform.h"
#include "DX9GFKeys.h"
//
#include "DX9GFSprites.h"
#include "DX9GFCamera.h"
#include <map>
#include <memory>
#include <string>
#include <tuple>
//
#define KEYBOARD_BUFFER_SIZE 1024
#define KEYBOARD_LAST_PRESS_TIME 125
#define MOUSE_LAST_PRESS_TIME 125

union SDL_Event;

namespace DX9GF {
	class InputManager final {
	public:
		enum MouseButton {
			Left = 0,
			Right = 1,
			Middle = 2
		};

		enum class CursorType //i use enum class becuz it's more secure, i guess
		{
			CURSOR,
			POINTER,
			CLICK,
			GRAB,
			TEXTSELECT,
		};

	private:
		static InputManager* instance;

		// Live state, kept current by OnEvent() as SDL events arrive. ReadKeyboard() and
		// ReadMouse() snapshot it once per frame, which is what gives KeyDown/KeyUp/
		// MouseDown/MouseUp their "changed since last frame" meaning.
		BYTE liveKeys[256] = {};
		BYTE liveButtons[4] = {};
		float liveMouseX = 0.0f; // window coordinates, as reported by SDL
		float liveMouseY = 0.0f;
		float wheelNotches = 0.0f; // accumulated since the last ReadMouse (positive = away from user)

		BYTE mouseButtons[4] = {}; // button state as of the last ReadMouse
		long mouseScroll = 0;      // wheel movement during the last ReadMouse, 120 per notch
		BYTE mouseBuffer[4] = {};
		POINT mousePos = {0, 0};
		POINT lastMousePos = {0, 0};
		POINT relativeMousePos = {0, 0};
		bool hasMousePos = false;

		//CUSTOM CURSOR VARS
		std::map<CursorType, std::shared_ptr<DX9GF::Texture>> cursorTextures;
		std::map<CursorType, std::shared_ptr<DX9GF::StaticSprite>> cursorSprites;
		std::map<CursorType, float> hotspotsX; // = 
		std::map<CursorType, float> hotspotsY;
		CursorType currentCursorType = CursorType::CURSOR; //save state, cursor default
		bool isCustomCursorActive = false;

		InputManager() {}
		~InputManager() {}

		char keys[256] = {};
		char keysBuffer[256] = {};
		bool consumedKeys[256] = { false };
		bool consumedMouseButtons[4] = { false };
		int lastKeyPressed = -1;
		int lastBufferPressed = -1;
		int lastMouseButtonPressed = -1;
		int lastMouseBufferPressed = -1;
		bool firstKeyCheck = true;
		bool firstMouseCheck = true;
		unsigned long long keysDelta = 0;
		unsigned long long mouseDelta = 0;

		bool CheckKeysBuffer(int DIKey) const;
		bool CheckMouseBuffer(MouseButton button);
		void UpdateMouseBuffer(int buttonIndex);
	public:
		static InputManager* GetInstance();
		/// <summary>
		/// Call once after the application window exists.
		/// </summary>
		void Init();
		/// <summary>
		/// Feeds one SDL event (key, mouse button/motion/wheel, focus) into the live state.
		/// Called by Application for every event; tests can call it with synthetic events.
		/// </summary>
		void OnEvent(const SDL_Event& event);
		void ConsumeKey(int DIKey);
		void ConsumeMouseButton(MouseButton button);
		void ReadKeyboard(unsigned long long deltaTime);
		bool KeyPress(int DIKey);
		bool KeyDown(int DIKey);
		bool KeyUp(int DIKey);
		void ReleaseLastPressed();
		void ReadMouse(unsigned long long deltaTime);
		bool MousePress(MouseButton button);
		bool MouseDown(MouseButton button);
		bool MouseUp(MouseButton button);
		long GetRelativeMouseX() const;
		long GetRelativeMouseY() const;
		POINT GetRelativeMousePos() const;
		long GetAbsoluteMouseX() const;
		long GetAbsoluteMouseY() const;
		POINT GetAbsoluteMousePos() const;
		long GetMouseScroll() const;
		std::tuple<float, float> GetVirtualAbsoluteMousePos(const DX9GF::Camera* camera) const;
		std::tuple<float, float> GetVirtualRelativeMousePos(const DX9GF::Camera* camera) const;
		void AddCursor(CursorType type, DX9GF::GraphicsDevice* gd, const std::wstring& path, float scale, float hX, float hY);
		void SwitchCursor(CursorType type);
		void EnableCustomCursor(bool enable);
		void DrawCursor(DX9GF::Camera* uiCamera, unsigned long long deltaTime);
		void Dispose();

	};
}
