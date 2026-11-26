#include "pch.h"
#include "Sprites.h"
#include "GraphicsDevice.h"
#include <stdexcept>

namespace {
	// Draws the part `src` of a texture (null = all of it) at the local origin, transformed by `m`.
	void DrawTextureRegion(sdlgf::GraphicsDevice* gd, sdlgf::Texture* texture, const RECT* src, const sdlgf::Mat4& m, sdlgf::Color color)
	{
		if (gd == nullptr || texture == nullptr || !texture->IsLoaded()) return;
		const float texW = static_cast<float>(texture->GetWidth());
		const float texH = static_cast<float>(texture->GetHeight());
		const float left = src ? static_cast<float>(src->left) : 0.0f;
		const float top = src ? static_cast<float>(src->top) : 0.0f;
		const float right = src ? static_cast<float>(src->right) : texW;
		const float bottom = src ? static_cast<float>(src->bottom) : texH;
		gd->DrawTexturedQuad(texture, m, 0.0f, 0.0f, right - left, bottom - top,
			left / texW, top / texH, right / texW, bottom / texH, color);
	}
}

sdlgf::StaticSprite::StaticSprite(std::shared_ptr<Texture> texture) : ISprite(texture->GetGraphicsDevice()), texture(std::move(texture))
{
}

sdlgf::StaticSprite::~StaticSprite()
{
}

void sdlgf::StaticSprite::Begin()
{
}

void sdlgf::StaticSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();
	auto matFinal = matWorld * matCamera;
	DrawTextureRegion(graphicsDevice, texture.get(), hasSrc ? &src : nullptr, matFinal, color);
}

void sdlgf::StaticSprite::End()
{
}

void sdlgf::StaticSprite::SetSrcRect(RECT srcRect)
{
	this->src = srcRect;
	this->hasSrc = true;
}

sdlgf::AnimatedSprite::AnimatedSprite(
	std::shared_ptr<Texture> spritesheet, 
	std::vector<RECT> frames, 
	UINT frameRate, 
	bool isLooped
) : ISprite(spritesheet->GetGraphicsDevice()), 
	frames(frames), 
	frameRate(frameRate), 
	texture(std::move(spritesheet)), 
	isLooped(isLooped) 
{
}

sdlgf::AnimatedSprite::~AnimatedSprite()
{
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
	DrawTextureRegion(graphicsDevice, texture.get(), p_src, matFinal, color);
}

void sdlgf::AnimatedSprite::End()
{
}

sdlgf::FontSprite::FontSprite(std::shared_ptr<Font> font) : ISprite(font->GetGraphicsDevice()), font(std::move(font))
{
}

sdlgf::FontSprite::~FontSprite()
{
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
	return font->Measure(text, maxWidth).width;
}

LONG sdlgf::FontSprite::GetHeight() const
{
	return font->Measure(text, maxWidth).height;
}

void sdlgf::FontSprite::Begin()
{
}

void sdlgf::FontSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	const Font::Rendered rendered = font->Render(text, maxWidth);
	if (rendered.texture == nullptr) return;

	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();
	auto matFinal = matWorld * matCamera;

	const auto drawAt = [&](const sdlgf::Mat4& m, sdlgf::Color c) {
		graphicsDevice->DrawTexturedQuad(rendered.texture, m,
			static_cast<float>(rendered.offsetX), static_cast<float>(rendered.offsetY),
			static_cast<float>(rendered.width), static_cast<float>(rendered.height),
			0.0f, 0.0f, 1.0f, 1.0f, c);
	};

	if (isOutline) {
		for (float x = -outlineThickness; x <= outlineThickness; x += 1.0f) {
			for (float y = -outlineThickness; y <= outlineThickness; y += 1.0f) {
				// Ignore points outside the circle radius for rounded outline
				if (x * x + y * y > outlineThickness * outlineThickness) continue;

				auto matOffsetWorld = matWorld;
				matOffsetWorld._41 += x;
				matOffsetWorld._42 += y;
				auto matOffsetFinal = matOffsetWorld * matCamera;
				drawAt(matOffsetFinal, outlineColor);

				if (isBold) {
					matOffsetWorld._41 += 1.0f;
					matOffsetFinal = matOffsetWorld * matCamera;
					drawAt(matOffsetFinal, outlineColor);
				}
			}
		}
	}

	drawAt(matFinal, color);

	if (isBold) {
		auto matOffsetWorld = matWorld;
		matOffsetWorld._41 += 1.0f; // Offset X by 1
		auto matOffsetFinal = matOffsetWorld * matCamera;
		drawAt(matOffsetFinal, color);
	}
}

void sdlgf::FontSprite::End()
{
}

sdlgf::NineSliceSprite::NineSliceSprite(std::shared_ptr<Texture> texture, RECT srcRect, int left, int top, int right, int bottom)
	: ISprite(texture->GetGraphicsDevice()), texture(std::move(texture)), srcRect(srcRect),
	leftMargin(left), topMargin(top), rightMargin(right), bottomMargin(bottom)
{
	targetWidth = static_cast<float>(srcRect.right - srcRect.left);
	targetHeight = static_cast<float>(srcRect.bottom - srcRect.top);
}

sdlgf::NineSliceSprite::~NineSliceSprite()
{
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
}

void sdlgf::NineSliceSprite::Draw(const Camera& camera, unsigned long long deltaTime)
{
	if (graphicsDevice == nullptr || texture == nullptr || !texture->IsLoaded()) return;
	auto matWorld = GetTransformMatrix();
	auto matCamera = camera.GetTransformMatrix();
	const sdlgf::Mat4 matFinal = matWorld * matCamera;

	// Calculate the 4 clipping coordinates on the source image
	float srcX[4] = { (float)srcRect.left, (float)srcRect.left + leftMargin, (float)srcRect.right - rightMargin, (float)srcRect.right };
	float srcY[4] = { (float)srcRect.top, (float)srcRect.top + topMargin, (float)srcRect.bottom - bottomMargin, (float)srcRect.bottom };

	// Calculate the 4 drawing coordinates on the target screen
	float dstX[4] = { 0.0f, leftMargin * scale, targetWidth - (rightMargin * scale), targetWidth };
	float dstY[4] = { 0.0f, topMargin * scale, targetHeight - (bottomMargin * scale), targetHeight };

	const float texW = static_cast<float>(texture->GetWidth());
	const float texH = static_cast<float>(texture->GetHeight());

	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 3; ++col) {
			float sW = srcX[col + 1] - srcX[col];
			float sH = srcY[row + 1] - srcY[row];
			float dW = dstX[col + 1] - dstX[col];
			float dH = dstY[row + 1] - dstY[row];

			// Skip if the clipping array is invalid or the target size is too small
			if (sW <= 0.0f || sH <= 0.0f || dW <= 0.0f || dH <= 0.0f) continue;

			// Source rectangles are whole pixels
			const float sl = static_cast<float>(static_cast<LONG>(srcX[col]));
			const float st = static_cast<float>(static_cast<LONG>(srcY[row]));
			const float sr = static_cast<float>(static_cast<LONG>(srcX[col + 1]));
			const float sb = static_cast<float>(static_cast<LONG>(srcY[row + 1]));

			graphicsDevice->DrawTexturedQuad(texture.get(), matFinal, dstX[col], dstY[row], dW, dH,
				sl / texW, st / texH, sr / texW, sb / texH, color);
		}
	}
}

void sdlgf::NineSliceSprite::End()
{
}
