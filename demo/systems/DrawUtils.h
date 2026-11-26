#pragma once

#include "GraphicsDevice.h"

namespace Demo {
	/// <summary>
	/// Draws a dashed line whose dashes move from the first position toward the last position.
	/// </summary>
	/// <param name="tick">Elapsed animation time in milliseconds.</param>
	/// <param name="animationSpeed">Dash movement speed in pixels per second.</param>
	void DrawAnimatedDashedLine(
		sdlgf::GraphicsDevice* graphicsDevice,
		float firstX,
		float firstY,
		float lastX,
		float lastY,
		float lineThickness,
		sdlgf::Color lineColor,
		bool drawOutline,
		float outlineThickness,
		sdlgf::Color outlineColor,
		float dashLength,
		float dashDistance,
		float animationSpeed,
		unsigned long long tick);

	void DrawAnimatedDashedLine(
		sdlgf::GraphicsDevice* graphicsDevice,
		sdlgf::Camera& camera,
		float firstX,
		float firstY,
		float lastX,
		float lastY,
		float lineThickness,
		sdlgf::Color lineColor,
		bool drawOutline,
		float outlineThickness,
		sdlgf::Color outlineColor,
		float dashLength,
		float dashDistance,
		float animationSpeed,
		unsigned long long tick);

	/// <summary>
	/// Draws an animated dashed arrow from the first position toward the last position.
	/// </summary>
	/// <param name="tick">Elapsed animation time in milliseconds.</param>
	/// <param name="animationSpeed">Dash movement speed in pixels per second.</param>
	void DrawAnimatedDashedArrow(
		sdlgf::GraphicsDevice* graphicsDevice,
		float firstX,
		float firstY,
		float lastX,
		float lastY,
		float lineThickness,
		sdlgf::Color lineColor,
		bool drawOutline,
		float outlineThickness,
		sdlgf::Color outlineColor,
		float dashLength,
		float dashDistance,
		float animationSpeed,
		unsigned long long tick,
		float headLength,
		float headWidth);

	void DrawAnimatedDashedArrow(
		sdlgf::GraphicsDevice* graphicsDevice,
		sdlgf::Camera& camera,
		float firstX,
		float firstY,
		float lastX,
		float lastY,
		float lineThickness,
		sdlgf::Color lineColor,
		bool drawOutline,
		float outlineThickness,
		sdlgf::Color outlineColor,
		float dashLength,
		float dashDistance,
		float animationSpeed,
		unsigned long long tick,
		float headLength,
		float headWidth);

	void DrawAnimatedDashedRectangle(
		sdlgf::GraphicsDevice* graphicsDevice,
		float x,
		float y,
		float width,
		float height,
		float lineThickness,
		sdlgf::Color lineColor,
		bool drawOutline,
		float outlineThickness,
		sdlgf::Color outlineColor,
		float dashLength,
		float dashDistance,
		float animationSpeed,
		unsigned long long tick);

	void DrawAnimatedDashedRectangle(
		sdlgf::GraphicsDevice* graphicsDevice,
		sdlgf::Camera& camera,
		float x,
		float y,
		float width,
		float height,
		float lineThickness,
		sdlgf::Color lineColor,
		bool drawOutline,
		float outlineThickness,
		sdlgf::Color outlineColor,
		float dashLength,
		float dashDistance,
		float animationSpeed,
		unsigned long long tick);

	/// <summary>
	/// Draws the keyboard-navigation target reticle: a pulsing yellow rectangle around the
	/// target's bounds with a bouncing down-pointing triangle above it.
	/// </summary>
	/// <param name="tick">Elapsed animation time in milliseconds.</param>
	void DrawKeyboardTargetReticle(
		sdlgf::GraphicsDevice* graphicsDevice,
		sdlgf::Camera& camera,
		float x,
		float y,
		float width,
		float height,
		unsigned long long tick);
}
