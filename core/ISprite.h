#pragma once
#include <d3dx9.h>
#include "Camera.h"
#include "IDeviceLostAware.h"

namespace sdlgf {
	class GraphicsDevice;

	class ISprite : public IDeviceLostAware {
	protected:
		GraphicsDevice* graphicsDevice = nullptr;
		ID3DXSprite* p_sprite = nullptr;
		sdlgf::Vec3 pos = { 0.0f, 0.0f, 0.0f };
		sdlgf::Color color = 0xFFFFFFFF;
		sdlgf::Vec2 scale = sdlgf::Vec2(1.0f, 1.0f);
		float rotation = 0.0f;
		sdlgf::Vec2 origin = sdlgf::Vec2(0.0f, 0.0f);
		bool needUpdate = true;
		sdlgf::Mat4 localTransformMatrix;
		sdlgf::Mat4 GetTransformMatrix();
	public:
		ISprite(GraphicsDevice* graphicsDevice);
		virtual ~ISprite();

		// Default implementation forwards to p_sprite, which every subclass constructs into
		// this base-class member. FontSprite's underlying sdlgf::Font registers itself
		// separately and does not need an override here.
		void OnLostDevice() override;
		void OnResetDevice() override;

		virtual void Begin() = 0;
		virtual void Draw(const Camera& camera, unsigned long long deltaTime) = 0;
		virtual void End() = 0;

		/// <summary>
		/// Translates the current position by the specified x and y offsets.
		/// </summary>
		/// <param name="x">The horizontal offset to translate by.</param>
		/// <param name="y">The vertical offset to translate by.</param>
		void Translate(float x, float y);

		/// <summary>
		/// Sets the position of the sprite
		/// </summary>
		/// <param name="x">The position in the horizontal coordinate</param>
		/// <param name="y">The position in the vertical coordinate</param>
		void SetPosition(float x, float y);
		void SetPosition(float x, float y, float z);
		void SetPositionX(float x);
		void SetPositionY(float y);
		void SetPositionZ(float z);

		float GetPositionX() const;
		float GetPositionY() const;
		float GetPositionZ() const;
		sdlgf::Vec3 GetPosition() const;
		//color (modulates the sprite, including alpha for opacity)
		void SetColor(sdlgf::Color color);
		sdlgf::Color GetColor() const;
		//scale
		void SetScale(float s);
		void SetScale(float sx, float sy); //stretch
		void SetScaleX(float sx);
		void SetScaleY(float sy);
		sdlgf::Vec2 GetScale() const;
		//rotate
		void SetRotation(float radians);
		void Rotate(float radians);
		float GetRotation() const;
		//flip
		void FlipHorizontal();
		void FlipVertical();
		//origin
		void SetOrigin(float x, float y);
		GraphicsDevice* GetGraphicsDevice();
	};
}