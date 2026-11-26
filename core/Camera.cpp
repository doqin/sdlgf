#include "pch.h"
#include "Camera.h"

sdlgf::Camera::Camera(int screenWidth, int screenHeight) : screenWidth(screenWidth), screenHeight(screenHeight), position(0.0f, 0.0f), rotation(0.0f), zoom(1.0f), needUpdate(true)
{
	D3DXMatrixIdentity(&transformMatrix);
}

void sdlgf::Camera::Update() const
{
	if (!needUpdate) return;
	D3DXMATRIX matTranslation;
	D3DXMATRIX matScale;
	D3DXMATRIX matRotation;
	D3DXMATRIX matScreenCenter;
	D3DXMatrixTranslation(&matTranslation, -position.x, -position.y, 0.0f);
	D3DXMatrixScaling(&matScale, zoom, zoom, 1.0f);
	D3DXMatrixRotationZ(&matRotation, rotation);
	D3DXMatrixTranslation(&matScreenCenter, screenWidth / 2.0f, screenHeight / 2.0f, 0.0f); // translate back to the center of the screen
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

void sdlgf::Camera::SetPosition(const D3DXVECTOR2& position)
{
	this->position = position;
	needUpdate = true;
}

void sdlgf::Camera::SetPosition(float x, float y)
{
	this->position = D3DXVECTOR2(x, y);
	needUpdate = true;
}

D3DXVECTOR2 sdlgf::Camera::GetPosition() const
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

D3DXMATRIX sdlgf::Camera::GetTransformMatrix() const
{
	Update();
	return transformMatrix;
}
