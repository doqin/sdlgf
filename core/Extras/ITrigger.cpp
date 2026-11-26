#include "pch.h"
#include "ITrigger.h"
#include "../input/InputManager.h"

namespace {
	bool IsClickedButton(sdlgf::ITrigger* trigger, unsigned long long deltaTime, sdlgf::InputManager::MouseButton button)
	{
		auto input = sdlgf::InputManager::GetInstance();
		return trigger->IsHovering(deltaTime) && input->MouseDown(button);
	}

	bool IsHeldButton(sdlgf::ITrigger* trigger, unsigned long long deltaTime, sdlgf::InputManager::MouseButton button, bool& isHeld)
	{
		auto input = sdlgf::InputManager::GetInstance();
		if (isHeld && input->MouseUp(button)) {
			isHeld = false;
			return false;
		}
		if (!isHeld && trigger->IsHovering(deltaTime) && input->MouseDown(button)) {
			isHeld = true;
			return true;
		}
		return isHeld;
	}
}

void sdlgf::ITrigger::Update(unsigned long long deltaTime)
{
	if (IsHovering(deltaTime)) {
		this->onHover(this);
	}

	bool cL = IsClickedLeft(deltaTime);
	bool cR = IsClickedRight(deltaTime);
	bool prevHeldLeft = isHeldLeft;
	bool prevHeldRight = isHeldRight;
	bool hL = IsHeldLeft(deltaTime);
	bool hR = IsHeldRight(deltaTime);
	

	if (cL) this->onClickLeft(this);
	if (cR) this->onClickRight(this);
	if (prevHeldLeft && !hL) this->onReleaseLeft(this);
	if (prevHeldRight && !hR) this->onReleaseRight(this);
	if (hL) this->onHeldLeft(this);
	if (hR) this->onHeldRight(this);

	auto input = sdlgf::InputManager::GetInstance();
	if (cL || isHeldLeft) {
		input->ConsumeMouseButton(sdlgf::InputManager::MouseButton::Left);
	}
	if (cR || isHeldRight) {
		input->ConsumeMouseButton(sdlgf::InputManager::MouseButton::Right);
	}
}

void sdlgf::ITrigger::Init(Camera* camera)
{
	this->camera = camera;
}

bool sdlgf::ITrigger::IsClicked(unsigned long long deltaTime)
{
	return IsClickedLeft(deltaTime);
}

bool sdlgf::ITrigger::IsClickedLeft(unsigned long long deltaTime)
{
	return IsClickedButton(this, deltaTime, InputManager::MouseButton::Left);
}

bool sdlgf::ITrigger::IsClickedRight(unsigned long long deltaTime)
{
	return IsClickedButton(this, deltaTime, InputManager::MouseButton::Right);
}

bool sdlgf::ITrigger::IsHeld(unsigned long long deltaTime)
{
	return IsHeldLeft(deltaTime);
}

bool sdlgf::ITrigger::IsHeldLeft(unsigned long long deltaTime)
{
	return IsHeldButton(this, deltaTime, InputManager::MouseButton::Left, isHeldLeft);
}

bool sdlgf::ITrigger::IsHeldRight(unsigned long long deltaTime)
{
	return IsHeldButton(this, deltaTime, InputManager::MouseButton::Right, isHeldRight);
}

void sdlgf::ITrigger::SetOnHover(std::function<void(ITrigger*)> onHover)
{
	this->onHover = onHover;
}

void sdlgf::ITrigger::SetOnClickLeft(std::function<void(ITrigger*)> onClick)
{
	this->onClickLeft = onClick;
}

void sdlgf::ITrigger::SetOnClickRight(std::function<void(ITrigger*)> onClick)
{
	this->onClickRight = onClick;
}

void sdlgf::ITrigger::SetOnHeldLeft(std::function<void(ITrigger*)> onHeld)
{
	this->onHeldLeft = onHeld;
}

void sdlgf::ITrigger::SetOnHeldRight(std::function<void(ITrigger*)> onHeld)
{
	this->onHeldRight = onHeld;
}

void sdlgf::ITrigger::SetOnReleaseLeft(std::function<void(ITrigger*)> onRelease)
{
	this->onReleaseLeft = onRelease;
}

void sdlgf::ITrigger::SetOnReleaseRight(std::function<void(ITrigger*)> onRelease)
{
	this->onReleaseRight = onRelease;
}
bool sdlgf::ITrigger::drawTrigger = false;