#include "pch.h"
#include "ISprite.h"
#include "GraphicsDevice.h"

D3DXMATRIX sdlgf::ISprite::GetTransformMatrix()
{
	if (needUpdate) {
		D3DXMATRIX matOffset;
		D3DXMATRIX matScale;
		D3DXMATRIX matRotation;
		D3DXMATRIX matTranslation;
		D3DXMatrixScaling(&matScale, scale.x, scale.y, 1.0f);
		D3DXMatrixRotationZ(&matRotation, rotation);		
		D3DXMatrixTranslation(&matTranslation, pos.x, pos.y, pos.z);
		D3DXMatrixTranslation(&matOffset, -origin.x, -origin.y, 0.0f);
		localTransformMatrix = matOffset * matScale * matRotation * matTranslation;
		needUpdate = false;
	}
	return localTransformMatrix;
}

sdlgf::ISprite::ISprite(GraphicsDevice* graphicsDevice) : graphicsDevice(graphicsDevice) {
	D3DXMatrixIdentity(&localTransformMatrix);
	if (graphicsDevice != nullptr) graphicsDevice->RegisterVolatileResource(this);
}

sdlgf::ISprite::~ISprite() {
	if (graphicsDevice != nullptr) graphicsDevice->UnregisterVolatileResource(this);
}

void sdlgf::ISprite::OnLostDevice() {
	if (p_sprite != nullptr) p_sprite->OnLostDevice();
}

void sdlgf::ISprite::OnResetDevice() {
	if (p_sprite != nullptr) p_sprite->OnResetDevice();
}

void sdlgf::ISprite::Translate(
	float x,
	float y
)
{
	pos.x += x;
	pos.y += y;
	needUpdate = true;
}

void sdlgf::ISprite::SetColor(D3DCOLOR color)
{
	this->color = color;
}

D3DCOLOR sdlgf::ISprite::GetColor() const
{
	return color;
}

void sdlgf::ISprite::SetPosition(float x, float y)
{
	pos.x = x;
	pos.y = y;
	needUpdate = true;
}

void sdlgf::ISprite::SetPosition(float x, float y, float z)
{
	pos.x = x;
	pos.y = y;
	pos.z = z;
	needUpdate = true;
}

void sdlgf::ISprite::SetPositionX(float x)
{
	pos.x = x;
	needUpdate = true;
}

void sdlgf::ISprite::SetPositionY(float y)
{
	pos.y = y;
	needUpdate = true;
}

void sdlgf::ISprite::SetPositionZ(float z)
{
	pos.z = z;
	needUpdate = true;
}

float sdlgf::ISprite::GetPositionX() const
{
	return pos.x;
}

float sdlgf::ISprite::GetPositionY() const
{
	return pos.y;
}

float sdlgf::ISprite::GetPositionZ() const
{
	return pos.z;
}

D3DXVECTOR3 sdlgf::ISprite::GetPosition() const
{
	return pos;
}

//scale
void sdlgf::ISprite::SetScale(float s)
{
	scale.x = s;
	scale.y = s;
	needUpdate = true;
}

void sdlgf::ISprite::SetScale(float sx, float sy)
{
	scale.x = sx;
	scale.y = sy;
	needUpdate = true;
}

void sdlgf::ISprite::SetScaleX(float sx)
{
	scale.x = sx;
	needUpdate = true;
}

void sdlgf::ISprite::SetScaleY(float sy)
{
	scale.y = sy;
	needUpdate = true;
}

D3DXVECTOR2 sdlgf::ISprite::GetScale() const
{
	return scale;
}

//rotate
void sdlgf::ISprite::SetRotation(float radians)
{
	rotation = radians;
	needUpdate = true;
}

void sdlgf::ISprite::Rotate(float radians)
{
	rotation += radians;
	needUpdate = true;
}

float sdlgf::ISprite::GetRotation() const
{
	return rotation;
}
//flip
void sdlgf::ISprite::FlipHorizontal()
{
	scale.x *= -1;
	needUpdate = true;
}

void sdlgf::ISprite::FlipVertical()
{
	scale.y *= -1;
	needUpdate = true;
}
//origin
void sdlgf::ISprite::SetOrigin(float x, float y)
{
	origin.x = x;
	origin.y = y;
	needUpdate = true;
}

sdlgf::GraphicsDevice* sdlgf::ISprite::GetGraphicsDevice()
{
	return graphicsDevice;
}