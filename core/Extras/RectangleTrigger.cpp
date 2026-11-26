#include "pch.h"
#include "RectangleTrigger.h"
#include "../InputManager.h"
#include "../Utils.h"
#include "../Application.h"
#include <array>
#include <cmath>

namespace {
	constexpr float floatEpsilon = 0.000001f;

	using sdlgf::Vec2;


	Vec2 Rotate(const Vec2& v, float radians)
	{
		float c = std::cos(radians);
		float s = std::sin(radians);
		return { v.x * c - v.y * s, v.x * s + v.y * c };
	}

	bool PointInParallelogram(const Vec2& p, const Vec2& p0, const Vec2& p1, const Vec2& p3)
	{
		Vec2 axisX = p1 - p0;
		Vec2 axisY = p3 - p0;
		float axisXLenSq = Dot(axisX, axisX);
		float axisYLenSq = Dot(axisY, axisY);
		if (axisXLenSq <= floatEpsilon || axisYLenSq <= floatEpsilon) {
			return false;
		}
		Vec2 d = p - p0;
		float u = Dot(d, axisX) / axisXLenSq;
		float v = Dot(d, axisY) / axisYLenSq;
		return u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f;
	}
}

bool sdlgf::RectangleTrigger::IsHovering(unsigned long long deltaTime)
{
	auto input = sdlgf::InputManager::GetInstance();
	auto [mouseX, mouseY] = input->GetVirtualAbsoluteMousePos(camera);
	auto [worldMouseX, worldMouseY] = Utils::WindowToWorldCoords(*camera, mouseX, mouseY);

	float worldX = GetWorldX();
	float worldY = GetWorldY();
	float r = GetWorldRotation();
	float sx = GetWorldScaleX();
	float sy = GetWorldScaleY();

	std::array<Vec2, 4> local = {
		Vec2{ 0.0f, 0.0f },
		Vec2{ width, 0.0f },
		Vec2{ width, height },
		Vec2{ 0.0f, height }
	};

	std::array<Vec2, 4> worldCorners;
	for (size_t i = 0; i < local.size(); i++) {
		Vec2 p = { (local[i].x - originX) * sx, (local[i].y - originY) * sy };
		p = Rotate(p, r);
		worldCorners[i] = { worldX + p.x, worldY + p.y };
	}

	return PointInParallelogram({ worldMouseX, worldMouseY }, worldCorners[0], worldCorners[1], worldCorners[3]);
}

void sdlgf::RectangleTrigger::SetOrigin(float x, float y)
{
	originX = x;
	originY = y;
}

void sdlgf::RectangleTrigger::SetOriginCenter()
{
	originX = width * 0.5f;
	originY = height * 0.5f;
}

void sdlgf::RectangleTrigger::SetSize(float width, float height)
{
	this->width = width;
	this->height = height;
}

float sdlgf::RectangleTrigger::GetOriginX() const
{
	return originX;
}

float sdlgf::RectangleTrigger::GetOriginY() const
{
	return originY;
}

float sdlgf::RectangleTrigger::GetWidth() const {
    return width;
}

float sdlgf::RectangleTrigger::GetHeight() const
{
    return height;
}

void sdlgf::RectangleTrigger::SetWidth(float width)
{
	this->width = width;
}

void sdlgf::RectangleTrigger::SetHeight(float height)
{
	this->height = height;
}

void sdlgf::RectangleTrigger::Draw(GraphicsDevice* graphicsDevice, const Camera& camera)
{
	if (!ITrigger::drawTrigger) return;

	float x = GetWorldX();
	float y = GetWorldY();
	float a = GetWorldRotation();
	float sx = GetWorldScaleX();
	float sy = GetWorldScaleY();
	float ox = GetOriginX();
	float oy = GetOriginY();
	graphicsDevice->DrawRectangle(
		camera,
		x,
		y,
		width,
		height,
		a,
		sx,
		sy,
		ox,
		oy,
		0x550000FF,
		false
	);
}
