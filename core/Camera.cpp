#include "pch.h"
#include "Camera.h"

sdlgf::Camera::Camera(int screenWidth, int screenHeight) : screenWidth(screenWidth), screenHeight(screenHeight), position(0.0f, 0.0f), rotation(0.0f), zoom(1.0f), needUpdate(true)
{
	transformMatrix = sdlgf::Mat4::Identity();
}

void sdlgf::Camera::Update() const
{
	if (!needUpdate) return;
	sdlgf::Mat4 matTranslation;
	sdlgf::Mat4 matScale;
	sdlgf::Mat4 matRotation;
	sdlgf::Mat4 matScreenCenter;
	matTranslation = sdlgf::Mat4::Translation(-position.x, -position.y, 0.0f);
	matScale = sdlgf::Mat4::Scaling(zoom, zoom, 1.0f);
	matRotation = sdlgf::Mat4::RotationZ(rotation);
	matScreenCenter = sdlgf::Mat4::Translation(screenWidth / 2.0f, screenHeight / 2.0f, 0.0f); // translate back to the center of the screen
	transformMatrix = matTranslation * matScale * matRotation * matScreenCenter;
	needUpdate = false;
}

void sdlgf::Camera::SetScreenWidth(int width)
{
	screenWidth = width;
	needUpdate = true;
}

void sdlgf::Camera::SetScreenHeight(int height)
{
	screenHeight = height;
	needUpdate = true;
}

void sdlgf::Camera::SetScreenResolution(int width, int height)
{
	SetScreenWidth(width);
	SetScreenHeight(height);
}

std::tuple<int, int> sdlgf::Camera::GetScreenResolution() const
{
	return { screenWidth, screenHeight };
}

void sdlgf::Camera::SetPosition(const sdlgf::Vec2& position)
{
	this->position = position;
	needUpdate = true;
}

void sdlgf::Camera::SetPosition(float x, float y)
{
	this->position = sdlgf::Vec2(x, y);
	needUpdate = true;
}

sdlgf::Vec2 sdlgf::Camera::GetPosition() const
{
	return position;
}

void sdlgf::Camera::SetRotation(float radians)
{
	this->rotation = radians;
	needUpdate = true;
}

float sdlgf::Camera::GetRotation() const
{
	return rotation;
}

void sdlgf::Camera::SetZoom(float zoom)
{
	if (zoom <= 0.001f) zoom = 0.001f;
	this->zoom = zoom;
	needUpdate = true;
}

float sdlgf::Camera::GetZoom() const
{
	return zoom;
}

sdlgf::Mat4 sdlgf::Camera::GetTransformMatrix() const
{
	Update();
	return transformMatrix;
}
