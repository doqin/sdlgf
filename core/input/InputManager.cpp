#include "pch.h"
#include "InputManager.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "Application.h"
#include "sdlgf.h"

sdlgf::InputManager* sdlgf::InputManager::instance = nullptr;

namespace {
	// DirectInput reported "down" as the high bit of a byte; the rest of the class still
	// tests for it that way.
	constexpr BYTE kDown = 0x80;

	// SDL button number -> sdlgf mouse button slot (Left/Right/Middle/X1), or -1.
	int ButtonSlot(Uint8 sdlButton)
	{
		switch (sdlButton) {
		case SDL_BUTTON_LEFT: return 0;
		case SDL_BUTTON_RIGHT: return 1;
		case SDL_BUTTON_MIDDLE: return 2;
		case SDL_BUTTON_X1: return 3;
		default: return -1;
		}
	}

	// Mouse position in client pixels, letterbox region of the virtual screen within it.
	struct Letterbox {
		float currentWidth, currentHeight;
		float scale, offsetX, offsetY, finalW, finalH;
	};

	Letterbox ComputeLetterbox(sdlgf::Application* app)
	{
		int clientW = 0, clientH = 0;
		app->GetClientSize(clientW, clientH);
		Letterbox box{};
		box.currentWidth = static_cast<float>(clientW);
		box.currentHeight = static_cast<float>(clientH);

		const float virtualWidth = static_cast<float>(app->GetScreenWidth());
		const float virtualHeight = static_cast<float>(app->GetScreenHeight());
		const float scaleX = box.currentWidth / virtualWidth;
		const float scaleY = box.currentHeight / virtualHeight;
		box.scale = (std::min)(scaleX, scaleY);
		box.finalW = virtualWidth * box.scale;
		box.finalH = virtualHeight * box.scale;
		box.offsetX = (box.currentWidth - box.finalW) / 2.0f;
		box.offsetY = (box.currentHeight - box.finalH) / 2.0f;
		return box;
	}
}

bool sdlgf::InputManager::CheckKeysBuffer(int DIKey) const
{
    return keysBuffer[DIKey] & 0x80;
}

bool sdlgf::InputManager::CheckMouseBuffer(MouseButton button)
{
    return mouseBuffer[button] & 0x80;
}

void sdlgf::InputManager::UpdateMouseBuffer(int buttonIndex)
{
    if (firstMouseCheck && (mouseButtons[buttonIndex] != mouseBuffer[buttonIndex])) {
        lastMouseBufferPressed = lastMouseButtonPressed;
        lastMouseButtonPressed = buttonIndex;
        mouseDelta = 0;
        firstMouseCheck = false;
    }
}

sdlgf::InputManager* sdlgf::InputManager::GetInstance()
{
    if (instance == nullptr) {
        instance = new InputManager();
    }
    return instance;
}

void sdlgf::InputManager::Init()
{
    // Start from where the cursor actually is rather than the corner
    SDL_GetMouseState(&liveMouseX, &liveMouseY);
}

void sdlgf::InputManager::OnEvent(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        // Auto-repeat re-sends KEY_DOWN for a key that is already down; state is unchanged
        if (event.key.repeat) break;
        const int dik = sdlgf::Keys::FromSDLScancode(static_cast<int>(event.key.scancode));
        if (dik > 0) {
            liveKeys[dik] = (event.type == SDL_EVENT_KEY_DOWN) ? kDown : 0;
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        const int slot = ButtonSlot(event.button.button);
        if (slot >= 0) {
            liveButtons[slot] = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? kDown : 0;
        }
        liveMouseX = event.button.x;
        liveMouseY = event.button.y;
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        liveMouseX = event.motion.x;
        liveMouseY = event.motion.y;
        break;
    case SDL_EVENT_MOUSE_WHEEL: {
        // Positive y is away from the user unless the OS reports natural (flipped) scrolling
        const float flip = (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -1.0f : 1.0f;
        wheelNotches += event.wheel.y * flip;
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        // Keys/buttons released while another window had focus would otherwise stay stuck
        std::memset(liveKeys, 0, sizeof(liveKeys));
        std::memset(liveButtons, 0, sizeof(liveButtons));
        break;
    }
}

void sdlgf::InputManager::ConsumeKey(int DIKey)
{
    if (DIKey >= 0 && DIKey < 256) {
        consumedKeys[DIKey] = true;
    }
}

void sdlgf::InputManager::ConsumeMouseButton(MouseButton button)
{
    if (button >= 0 && button < 4) {
        consumedMouseButtons[button] = true;
    }
}

void sdlgf::InputManager::ReadKeyboard(unsigned long long deltaTime)
{
    std::memset(consumedKeys, 0, sizeof(consumedKeys));
    keysDelta += deltaTime;
    if (keysDelta > KEYBOARD_LAST_PRESS_TIME) {
        ReleaseLastPressed();
    }
    for (int i = 0; i < 256; i++) {
        keysBuffer[i] = keys[i];
    }
    for (int i = 0; i < 256; i++) {
        keys[i] = static_cast<char>(liveKeys[i]);
    }
    firstKeyCheck = true;
}

bool sdlgf::InputManager::KeyPress(int DIKey)
{
    if (consumedKeys[DIKey]) return false;
    if (keys[DIKey] & 0x80) {
        if (firstKeyCheck && keysBuffer[DIKey] != keys[DIKey]) {
            lastBufferPressed = lastKeyPressed;
            lastKeyPressed = DIKey;
            keysDelta = 0;
            firstKeyCheck = false;
        }
        return true;
    }
    return false;
}

bool sdlgf::InputManager::KeyDown(int DIKey)
{
    if (consumedKeys[DIKey]) return false;
    return KeyPress(DIKey) && !CheckKeysBuffer(DIKey);
}

bool sdlgf::InputManager::KeyUp(int DIKey)
{
    if (consumedKeys[DIKey]) return false;
    return !KeyPress(DIKey) && CheckKeysBuffer(DIKey);
}

void sdlgf::InputManager::ReleaseLastPressed()
{
    keysDelta = 0;
    lastKeyPressed = -1;
    lastBufferPressed = -1;
}

void sdlgf::InputManager::ReadMouse(unsigned long long deltaTime)
{
    std::memset(consumedMouseButtons, 0, sizeof(consumedMouseButtons));
    mouseDelta += deltaTime;
    if (mouseDelta > MOUSE_LAST_PRESS_TIME) {
        mouseDelta = 0;
        lastMouseButtonPressed = -1;
        lastMouseBufferPressed = -1;
    }
    for (int i = 0; i < 4; i++) {
        mouseBuffer[i] = mouseButtons[i];
    }

    auto app = sdlgf::Application::GetInstance();
    const float density = app->GetPixelDensity();

    POINT rawMousePos;
    rawMousePos.x = static_cast<LONG>(liveMouseX * density);
    rawMousePos.y = static_cast<LONG>(liveMouseY * density);

    const Letterbox box = ComputeLetterbox(app);
    const float offsetX = box.offsetX, offsetY = box.offsetY;
    const float finalW = box.finalW, finalH = box.finalH;

    //Mouse Clamp
    if (rawMousePos.x < offsetX) rawMousePos.x = static_cast<LONG>(offsetX);
    if (rawMousePos.x > offsetX + finalW) rawMousePos.x = static_cast<LONG>(offsetX + finalW);
    if (rawMousePos.y < offsetY) rawMousePos.y = static_cast<LONG>(offsetY);
    if (rawMousePos.y > offsetY + finalH) rawMousePos.y = static_cast<LONG>(offsetY + finalH);

    mousePos.x = rawMousePos.x;
    mousePos.y = rawMousePos.y;

    if (!hasMousePos) {
        lastMousePos = mousePos;
        relativeMousePos = { 0, 0 };
        hasMousePos = true;
    }
    else {
        relativeMousePos = { mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };
        lastMousePos = mousePos;
    }

    for (int i = 0; i < 4; i++) {
        mouseButtons[i] = liveButtons[i];
    }
    // One wheel notch is 120 units (WHEEL_DELTA), as DirectInput reported it
    mouseScroll = static_cast<long>(std::lround(wheelNotches * 120.0f));
    wheelNotches = 0.0f;
    firstMouseCheck = true;
}

void sdlgf::InputManager::DrawCursor(sdlgf::Camera* uiCamera, unsigned long long deltaTime)
{
    if (!isCustomCursorActive || !uiCamera || !cursorSprites.count(currentCursorType)) return;

    // Re-assert the hidden state every frame: message boxes, Alt+Tab/focus changes and other
    // SDL users can flip cursor visibility back on. SDL_HideCursor() is a no-op when already
    // hidden, so this is cheap.
    if (SDL_CursorVisible()) SDL_HideCursor();

    auto activeSprite = cursorSprites[currentCursorType];
    float hX = hotspotsX[currentCursorType];
    float hY = hotspotsY[currentCursorType];

    POINT windowMousePos = this->GetAbsoluteMousePos();
    // Inverse transform screen coordinates to get world coordinates
    auto [worldX, worldY] = sdlgf::Utils::WindowToWorldCoords(*uiCamera, (float)windowMousePos.x, (float)windowMousePos.y);

    float drawX = worldX - hX;
    float drawY = worldY - hY;

    activeSprite->SetPosition(drawX, drawY);
    activeSprite->Begin();
    activeSprite->Draw(*uiCamera, deltaTime);
    activeSprite->End();

    if (liveButtons[0] & 0x80)
    {
        this->currentCursorType = CursorType::GRAB;
    }
    else
    {
        this->currentCursorType = CursorType::CURSOR;
    }
}

bool sdlgf::InputManager::MousePress(MouseButton button)
{
    if (consumedMouseButtons[button]) return false;
    if (mouseButtons[button] & 0x80) {
        UpdateMouseBuffer(button);
        return true;
    }
    return false;
}

bool sdlgf::InputManager::MouseDown(MouseButton button)
{
    if (consumedMouseButtons[button]) return false;
    return MousePress(button) && !CheckMouseBuffer(button);
}

bool sdlgf::InputManager::MouseUp(MouseButton button)
{
    if (consumedMouseButtons[button]) return false;
    return !MousePress(button) && CheckMouseBuffer(button);
}

long sdlgf::InputManager::GetRelativeMouseX() const
{
	return relativeMousePos.x;
}

long sdlgf::InputManager::GetRelativeMouseY() const
{
	return relativeMousePos.y;
}

POINT sdlgf::InputManager::GetRelativeMousePos() const
{
	return relativeMousePos;
}

long sdlgf::InputManager::GetAbsoluteMouseX() const
{
    return mousePos.x;
}

long sdlgf::InputManager::GetAbsoluteMouseY() const
{
    return mousePos.y;
}

POINT sdlgf::InputManager::GetAbsoluteMousePos() const
{
    return mousePos;
}

long sdlgf::InputManager::GetMouseScroll() const
{
    return mouseScroll;
}
//
void sdlgf::InputManager::EnableCustomCursor(bool enable)
{
    //if the state is the same, do nothing
    if (this->isCustomCursorActive == enable) return;

    this->isCustomCursorActive = enable;

    // The game draws its own cursor sprite, so hide the system one (and restore it when disabled)
    if (enable)
    {
        SDL_HideCursor();
    }
    else
    {
        SDL_ShowCursor();
    }
}
void sdlgf::InputManager::AddCursor(CursorType type, sdlgf::GraphicsDevice* gd, const std::wstring& path, float scale, float hX, float hY)
{
    auto tex = gd->GetAssets().GetTexture(path);
    if (tex->IsLoaded()) {
        auto sprite = std::make_shared<sdlgf::StaticSprite>(tex);
        sprite->SetScale(scale, scale);
        this->cursorTextures[type] = tex;
        this->cursorSprites[type] = sprite;
        this->hotspotsX[type] = hX;
        this->hotspotsY[type] = hY;
        this->EnableCustomCursor(true);
    }
}

void sdlgf::InputManager::SwitchCursor(CursorType type)
{
    if (this->cursorSprites.count(type)) {
        this->currentCursorType = type;
    }
}

void sdlgf::InputManager::Dispose()
{
    // Nothing to release: input state is plain data and SDL owns the devices.
}

std::tuple<float, float> sdlgf::InputManager::GetVirtualAbsoluteMousePos(const sdlgf::Camera* camera) const
{
    float mouseX = static_cast<float>(this->GetAbsoluteMouseX());
    float mouseY = static_cast<float>(this->GetAbsoluteMouseY());

    if (!camera) return { mouseX, mouseY };

    auto app = sdlgf::Application::GetInstance();
    float virtualW = static_cast<float>(app->GetScreenWidth());
    float virtualH = static_cast<float>(app->GetScreenHeight());
    auto [camW, camH] = camera->GetScreenResolution();

    if (camW == virtualW && camH == virtualH) {
        const Letterbox box = ComputeLetterbox(app);
        float currentWidth = box.currentWidth;
        float currentHeight = box.currentHeight;

        if (currentWidth > 0.0f && currentHeight > 0.0f) {
            float scale = box.scale;
            float offsetX = box.offsetX;
            float offsetY = box.offsetY;

            mouseX = (mouseX - offsetX) / scale;
            mouseY = (mouseY - offsetY) / scale;
        }
    }
    return { mouseX, mouseY };
}

std::tuple<float, float> sdlgf::InputManager::GetVirtualRelativeMousePos(const sdlgf::Camera* camera) const
{
    float dX = static_cast<float>(this->GetRelativeMouseX());
    float dY = static_cast<float>(this->GetRelativeMouseY());

    if (!camera) return { dX, dY };

    auto app = sdlgf::Application::GetInstance();
    float virtualW = static_cast<float>(app->GetScreenWidth());
    float virtualH = static_cast<float>(app->GetScreenHeight());
    auto [camW, camH] = camera->GetScreenResolution();

    if (camW == virtualW && camH == virtualH) {
        const Letterbox box = ComputeLetterbox(app);

        if (box.currentWidth > 0.0f && box.currentHeight > 0.0f) {
            dX /= box.scale;
            dY /= box.scale;
        }
    }
    return { dX, dY };
}
