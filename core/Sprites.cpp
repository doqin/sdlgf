#include "pch.h"
#include "Sprites.h"
#include "GraphicsDevice.h"
#include <DxErr.h>
#include <d3dx9.h>
#include <cstring>
#include <stdexcept>

// Legacy D3DX sprites take D3DX types; our Mat4/Vec3 have the same layout. Removed with the D3D9 backend.
static_assert(sizeof(sdlgf::Mat4) == sizeof(D3DXMATRIX), "Mat4 must match D3DXMATRIX");
static void SetSpriteTransform(ID3DXSprite* sprite, const sdlgf::Mat4& m)
{
	D3DXMATRIX d;
	std::memcpy(&d, &m, sizeof(d));
	sprite->SetTransform(&d);
}

static void ApplyPixelArtSamplerState(IDirect3DDevice9* device)
{
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
}

void sdlgf::FontSprite::BuildTextRect(RECT& rect, DWORD& format, bool measure) const
{
	rect = RECT{};
	format = measure ? DT_CALCRECT : 0;
	if (p_src != nullptr) {
		rect = *p_src;
		format |= DT_TOP | DT_LEFT;
	}
	else {
		format |= DT_NOCLIP;
	}
	if (maxWidth > 0.f) {
		// DT_NOCLIP stays set: the rect has no height, so clipping would hide everything.
		rect.right = rect.left + static_cast<LONG>(maxWidth);
		format |= DT_WORDBREAK;
	}
}

sdlgf::StaticSprite::StaticSprite(Texture* texture) : ISprite(texture->GetGraphicsDevice()), texture(texture)
{
	HRESULT result = D3DXCreateSprite(graphicsDevice->GetDevice(), &p_sprite);
	if (result != D3D_OK) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::invalid_argument(what);
	}
}

sdlgf::StaticSprite::~StaticSprite()
{
	if (p_sprite != nullptr) p_sprite->Release();
}

void sdlgf::StaticSprite::Begin()
{
	p_sprite->OnLostDevice();
	p_sprite->OnResetDevice();
	if (auto result = p_sprite->Begin(D3DXSPRITE_ALPHABLEND); FAILED(result)) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::runtime_error(what);
	}
   ApplyPixelArtSamplerState(graphicsDevice->GetDevice());
}

void sdlgf::StaticSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();
	auto matFinal = matWorld * matCamera;
	D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
	SetSpriteTransform(p_sprite, matFinal);
	p_sprite->Draw(
		texture->GetRawTexture(),
		hasSrc ? &src : nullptr,
		nullptr,
		&zeroPos,
		color
	);
}

void sdlgf::StaticSprite::End()
{
	p_sprite->End();
}

void sdlgf::StaticSprite::SetSrcRect(RECT srcRect)
{
	this->src = srcRect;
	this->hasSrc = true;
}

sdlgf::AnimatedSprite::AnimatedSprite(
	Texture* spritesheet, 
	std::vector<RECT> frames, 
	UINT frameRate, 
	bool isLooped
) : ISprite(spritesheet->GetGraphicsDevice()), 
	frames(frames), 
	frameRate(frameRate), 
	texture(spritesheet), 
	isLooped(isLooped) 
{
	HRESULT result = D3DXCreateSprite(graphicsDevice->GetDevice(), &p_sprite);

	if (result != D3D_OK) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::runtime_error(what);
	}
}

sdlgf::AnimatedSprite::~AnimatedSprite()
{
	if (p_sprite != nullptr) p_sprite->Release();
}

void sdlgf::AnimatedSprite::SetFrame(unsigned int frameIndex)
{
	this->frame_index = frameIndex;
}

void sdlgf::AnimatedSprite::SetFrameRate(unsigned int frameRate)
{
	this->frameRate = frameRate;
}

void sdlgf::AnimatedSprite::SetLooped(bool isLooped)
{
	this->isLooped = isLooped;
}

bool sdlgf::AnimatedSprite::IsFinished() const
{
	if (isLooped) return false;
	return frame_index >= frames.size() - 1;
}

void sdlgf::AnimatedSprite::Begin()
{
	p_sprite->OnLostDevice();
	p_sprite->OnResetDevice();
	if (auto result = p_sprite->Begin(D3DXSPRITE_ALPHABLEND); FAILED(result)) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::runtime_error(what);
	}
   ApplyPixelArtSamplerState(graphicsDevice->GetDevice());
}

void sdlgf::AnimatedSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	if (frames.size() == 0) throw std::runtime_error("Sprite frames are empty!");
	delta += deltaTime;
	if (delta > 1000 / frameRate) {
		frame_index += delta / (1000 / frameRate); // this will handle skipping more than 1 frame if delta is worth 2+ frame time
		delta = 0;
	}
	if (!isLooped && frame_index >= frames.size()) {
		frame_index = frames.size() - 1;
	}
	frame_index %= frames.size();
	auto p_src = &frames.at(frame_index);
	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();
	auto matFinal = matWorld * matCamera;
	D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);
	SetSpriteTransform(p_sprite, matFinal);
	p_sprite->Draw(
		texture->GetRawTexture(),
		p_src,
		nullptr,
		&zeroPos,
		color
	);
}

void sdlgf::AnimatedSprite::End()
{
	p_sprite->End();
}

sdlgf::FontSprite::FontSprite(Font* font) : ISprite(font->GetGraphicsDevice()), font(font)
{
	HRESULT result = D3DXCreateSprite(graphicsDevice->GetDevice(), &p_sprite);
	if (result != D3D_OK) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::runtime_error(what);
	}
}

sdlgf::FontSprite::~FontSprite()
{
	if (p_src != nullptr) delete p_src;
	if (p_sprite != nullptr) p_sprite->Release();
}

void sdlgf::FontSprite::SetText(std::wstring&& text)
{
	this->text = text;
}

void sdlgf::FontSprite::SetText(const std::wstring& text)
{
	this->text = text;
}

void sdlgf::FontSprite::SetColor(sdlgf::Color color)
{
	this->color = color;
}

void sdlgf::FontSprite::SetBold(bool bold)
{
	this->isBold = bold;
}

void sdlgf::FontSprite::SetOutline(bool outline, sdlgf::Color color, float thickness)
{
	this->isOutline = outline;
	this->outlineColor = color;
	this->outlineThickness = thickness;
}

LONG sdlgf::FontSprite::GetWidth() const
{
	RECT rect;
	DWORD format;
	BuildTextRect(rect, format, true);
	font->GetRawFont()->DrawText(nullptr, text.c_str(), -1, &rect, format, color);
	return rect.right - rect.left;
}

LONG sdlgf::FontSprite::GetHeight() const
{
	RECT rect;
	DWORD format;
	BuildTextRect(rect, format, true);
	font->GetRawFont()->DrawText(nullptr, text.c_str(), -1, &rect, format, color);
	return rect.bottom - rect.top;
}

void sdlgf::FontSprite::Begin()
{
	p_sprite->OnLostDevice();
	p_sprite->OnResetDevice();
	font->GetRawFont()->OnLostDevice();
	font->GetRawFont()->OnResetDevice();
	if (auto result = p_sprite->Begin(D3DXSPRITE_ALPHABLEND); FAILED(result)) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::runtime_error(what);
	}
   ApplyPixelArtSamplerState(graphicsDevice->GetDevice());
}

void sdlgf::FontSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();
	auto matFinal = matWorld * matCamera;
	RECT rect;
	DWORD format;
	BuildTextRect(rect, format, false);

	if (isOutline) {
		for (float x = -outlineThickness; x <= outlineThickness; x += 1.0f) {
			for (float y = -outlineThickness; y <= outlineThickness; y += 1.0f) {
				// Ignore points outside the circle radius for rounded outline
				if (x * x + y * y > outlineThickness * outlineThickness) continue;

				auto matOffsetWorld = matWorld;
				matOffsetWorld._41 += x;
				matOffsetWorld._42 += y;
				auto matOffsetFinal = matOffsetWorld * matCamera;
				SetSpriteTransform(p_sprite, matOffsetFinal);
				font->GetRawFont()->DrawText(p_sprite, text.c_str(), -1, &rect, format, outlineColor);

				if (isBold) {
					matOffsetWorld._41 += 1.0f;
					matOffsetFinal = matOffsetWorld * matCamera;
					SetSpriteTransform(p_sprite, matOffsetFinal);
					font->GetRawFont()->DrawText(p_sprite, text.c_str(), -1, &rect, format, outlineColor);
				}
			}
		}
	}

	SetSpriteTransform(p_sprite, matFinal);
	font->GetRawFont()->DrawText(p_sprite, text.c_str(), -1, &rect, format, color);

	if (isBold) {
		auto matOffsetWorld = matWorld;
		matOffsetWorld._41 += 1.0f; // Offset X by 1
		auto matOffsetFinal = matOffsetWorld * matCamera;
		SetSpriteTransform(p_sprite, matOffsetFinal);
		font->GetRawFont()->DrawText(p_sprite, text.c_str(), -1, &rect, format, color);
	}
}

void sdlgf::FontSprite::End()
{
	p_sprite->End();
}

sdlgf::NineSliceSprite::NineSliceSprite(Texture* texture, RECT srcRect, int left, int top, int right, int bottom)
	: ISprite(texture->GetGraphicsDevice()), texture(texture), srcRect(srcRect),
	leftMargin(left), topMargin(top), rightMargin(right), bottomMargin(bottom)
{
	targetWidth = static_cast<float>(srcRect.right - srcRect.left);
	targetHeight = static_cast<float>(srcRect.bottom - srcRect.top);

	HRESULT result = D3DXCreateSprite(graphicsDevice->GetDevice(), &p_sprite);
	if (result != D3D_OK) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::invalid_argument(what);
	}
}

sdlgf::NineSliceSprite::~NineSliceSprite()
{
	if (p_sprite != nullptr) p_sprite->Release();
}

void sdlgf::NineSliceSprite::SetTargetSize(float width, float height)
{
	targetWidth = width;
	targetHeight = height;
}

void sdlgf::NineSliceSprite::SetSrcRect(RECT srcRect)
{
	this->srcRect = srcRect;
}

void sdlgf::NineSliceSprite::SetMargins(int left, int top, int right, int bottom)
{
	leftMargin = left;
	topMargin = top;
	rightMargin = right;
	bottomMargin = bottom;
}

void sdlgf::NineSliceSprite::Begin()
{
	p_sprite->OnLostDevice();
	p_sprite->OnResetDevice();
	if (auto result = p_sprite->Begin(D3DXSPRITE_ALPHABLEND); FAILED(result)) {
		auto error = DXGetErrorDescription(result);
		std::string what = std::string(error, error + wcslen(error));
		throw std::runtime_error(what);
	}
	ApplyPixelArtSamplerState(graphicsDevice->GetDevice());
}

void sdlgf::NineSliceSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();

	// Calculate the 4 clipping coordinates on the source image
	float srcX[4] = { (float)srcRect.left, (float)srcRect.left + leftMargin, (float)srcRect.right - rightMargin, (float)srcRect.right };
	float srcY[4] = { (float)srcRect.top, (float)srcRect.top + topMargin, (float)srcRect.bottom - bottomMargin, (float)srcRect.bottom };

	// Calculate the 4 drawing coordinates on the target screen
	float dstX[4] = { 0.0f, leftMargin * scale, targetWidth - (rightMargin * scale), targetWidth };
	float dstY[4] = { 0.0f, topMargin * scale, targetHeight - (bottomMargin * scale), targetHeight };

	D3DXVECTOR3 zeroPos(0.0f, 0.0f, 0.0f);

	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 3; ++col) {
			float sW = srcX[col + 1] - srcX[col];
			float sH = srcY[row + 1] - srcY[row];
			float dW = dstX[col + 1] - dstX[col];
			float dH = dstY[row + 1] - dstY[row];

			// Skip if the clipping array is invalid or the target size is too small
			if (sW <= 0.0f || sH <= 0.0f || dW <= 0.0f || dH <= 0.0f) continue;

			RECT pSrc = { (LONG)srcX[col], (LONG)srcY[row], (LONG)srcX[col + 1], (LONG)srcY[row + 1] };

			sdlgf::Mat4 matPatchScale, matPatchTrans;
			matPatchScale = sdlgf::Mat4::Scaling(dW / sW, dH / sH, 1.0f);
			matPatchTrans = sdlgf::Mat4::Translation(dstX[col], dstY[row], 0.0f);

			sdlgf::Mat4 matFinal = matPatchScale * matPatchTrans * matWorld * matCamera;

			SetSpriteTransform(p_sprite, matFinal);
			p_sprite->Draw(texture->GetRawTexture(), &pSrc, nullptr, &zeroPos, color);
		}
	}
}

void sdlgf::NineSliceSprite::End()
{
	p_sprite->End();
}