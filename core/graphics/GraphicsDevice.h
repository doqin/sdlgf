#pragma once
#include <vector>
#include "Camera.h"
#include "Platform.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Vertex;

namespace sdlgf {
	class Texture;

	/// 2D renderer on top of SDL_Renderer.
	///
	/// Coordinates are pixels of the current render target, origin top-left, +y down. Everything
	/// is drawn as textured/coloured triangles, so one code path serves every SDL render backend
	/// (Direct3D 11/12, Metal, Vulkan, OpenGL, software).
	///
	/// The drawing functions that take no camera are "instant" draws: they work in virtual
	/// coordinates and are scaled by the virtual transform (see SetVirtualTransform). The ones
	/// with a camera apply the camera's matrix instead.
	class GraphicsDevice final {
	private:
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		Texture* renderTarget = nullptr; // null = the window
		unsigned int alphaBlendMode = 0; // an SDL_BlendMode
		std::vector<Texture*> textures;  // every live Texture made for this device
		std::vector<SDL_Vertex> scratchVertices;
		bool alphaBlending = false;
		bool scissorEnabled = false;
		RECT scissorRect{};
		// Virtual-to-backbuffer transform applied to the camera-less (instant) draw calls
		float virtualScale = 1.0f;
		float virtualOffsetX = 0.0f;
		float virtualOffsetY = 0.0f;

		void ApplyBlend();
		void ApplyScissor();
		void DrawLineInternal(float x1, float y1, float x2, float y2, sdlgf::Color color, float thickness);
		void FillFan(const std::vector<sdlgf::Vec2>& points, sdlgf::Color color);
		void StrokeStrip(const std::vector<sdlgf::Vec2>& points, sdlgf::Color color);
	public:
		/// Creates the renderer for `window`. `rendererName` selects an SDL render driver
		/// ("direct3d11", "software", ...); null lets SDL pick the best one.
		/// Throws std::runtime_error if no renderer can be created.
		explicit GraphicsDevice(SDL_Window* window, const char* rendererName = nullptr);
		~GraphicsDevice();
		GraphicsDevice(const GraphicsDevice&) = delete;
		GraphicsDevice& operator=(const GraphicsDevice&) = delete;

		SDL_Renderer* GetRenderer() const { return renderer; }
		SDL_Window* GetWindow() const { return window; }

		/// The blend mode every sprite and (when SetAlphaBlending(true)) primitive uses:
		/// result = src * srcAlpha + dst * (1 - srcAlpha), applied to colour AND alpha, exactly as
		/// the fixed-function pipeline the game was written against did. This is an SDL_BlendMode;
		/// the software renderer cannot do the separate alpha part and gets SDL_BLENDMODE_BLEND.
		unsigned int GetAlphaBlendMode() const { return alphaBlendMode; }

		/// Textures register themselves so the device can detach them when it is destroyed.
		void RegisterTexture(Texture* texture);
		void UnregisterTexture(Texture* texture);

		/// Size in pixels of what is currently being drawn into: the window, or the render target.
		void GetOutputSize(int& width, int& height) const;

		void SetViewport(int x, int y, int width, int height);
		/// Kept for call-site compatibility: SDL needs no begin/end bracket.
		bool BeginDraw() { return true; }
		void Clear(sdlgf::ColorF color);
		void Clear();
		void EndDraw() {}
		void Present();

		/// Redirects drawing into a texture created with Texture::CreateRenderTarget.
		bool SetRenderTarget(Texture* renderTarget);
		/// Draws into the window again.
		void RestoreRenderTarget();

		void SetAlphaBlending(bool enabled);
		void SetScissorTest(bool enabled);
		void SetScissorRect(const RECT& rect);

		/// <summary>
		/// Sets the transform mapping virtual screen coordinates to actual backbuffer
		/// pixels for the camera-less (instant) draw calls. Set by IGame each frame so
		/// instant drawing scales properly when the backbuffer resolution differs from
		/// the virtual resolution; identity while rendering into the virtual render target.
		/// </summary>
		void SetVirtualTransform(float scale, float offsetX, float offsetY);
		void ResetVirtualTransform();

		void DrawLine(float x1, float y1, float x2, float y2, sdlgf::Color color, float thickness = 1.0f);
		void DrawLine(const sdlgf::Camera& camera, float x1, float y1, float x2, float y2, sdlgf::Color color, float thickness = 1.0f);
		void DrawRectangle(float x, float y, float width, float height, sdlgf::Color color, bool isFilled);
		void DrawRectangle(const sdlgf::Camera& camera, float x, float y, float width, float height, sdlgf::Color color, bool isFilled);
		void DrawRectangle(float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled);
		void DrawRectangle(const sdlgf::Camera& camera, float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled);
		void DrawEllipse(float centerX, float centerY, float width, float height, sdlgf::Color color, bool isFilled);
		void DrawEllipse(const sdlgf::Camera& camera, float centerX, float centerY, float width, float height, sdlgf::Color color, bool isFilled);
		void DrawEllipse(float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled);
		void DrawEllipse(const sdlgf::Camera& camera, float x, float y, float width, float height, float rotation, float scaleX, float scaleY, float offsetX, float offsetY, sdlgf::Color color, bool isFilled);

		/// <summary>
		/// Draws an isoceles triangle pointing down, centered horizontally at centerX, spanning from topY to topY + height.
		/// </summary>
		void DrawTriangle(float centerX, float topY, float width, float height, sdlgf::Color color, bool isFilled);
		void DrawTriangle(const sdlgf::Camera& camera, float centerX, float topY, float width, float height, sdlgf::Color color, bool isFilled);
		/// Filled triangle in render-target pixels: the virtual transform is NOT applied.
		void DrawTriangleScreenSpace(float x1, float y1, float x2, float y2, float x3, float y3, sdlgf::Color color);

		/// <summary>
		/// Draws a filled, textured convex polygon (as a triangle fan) in screen space.
		/// </summary>
		/// <param name="points">Boundary points of the polygon, in order.</param>
		/// <param name="uvs">Texture coordinates matching each point in points.</param>
		void DrawTexturedPolygon(const std::vector<sdlgf::Vec2>& points, const std::vector<sdlgf::Vec2>& uvs, sdlgf::Texture* texture, sdlgf::Color color = 0xFFFFFFFF);

		/// Draws the part (u0,v0)-(u1,v1) of `texture` onto the rectangle (x, y, width, height)
		/// transformed by `transform`. Always alpha-blended; `color` modulates the texture.
		/// This is what every sprite draws with.
		void DrawTexturedQuad(Texture* texture, const sdlgf::Mat4& transform, float x, float y, float width, float height,
			float u0, float v0, float u1, float v1, sdlgf::Color color);

		/// A vertex of DrawTexturedTriangles: position in render-target pixels, texture coordinates.
		struct Vertex {
			float x, y;
			float u, v;
			sdlgf::Color color;
		};
		/// Draws `vertexCount` / 3 alpha-blended triangles of `texture`.
		void DrawTexturedTriangles(Texture* texture, const Vertex* vertices, int vertexCount);
	};
};
