#pragma once
#include "MathTypes.h"
#include <tuple>
namespace sdlgf {
	class Camera {
	private:
		sdlgf::Vec2 position;
		float rotation; // in radians
		float zoom; // scale
		int screenWidth;
		int screenHeight;
		mutable sdlgf::Mat4 transformMatrix;
		mutable bool needUpdate;
	protected:
	public:
		Camera(int screenWidth, int screenHeight);
		~Camera() {}

		void Update() const;
		void SetScreenWidth(int width);
		void SetScreenHeight(int height);
		void SetScreenResolution(int width, int height);
		std::tuple<int, int> GetScreenResolution() const;
		void SetPosition(const sdlgf::Vec2& position);
		void SetPosition(float x, float y);
		sdlgf::Vec2 GetPosition() const;
		void SetRotation(float radians);
		float GetRotation() const;
		void SetZoom(float zoom);
		float GetZoom() const;
		sdlgf::Mat4 GetTransformMatrix() const;
	};
}