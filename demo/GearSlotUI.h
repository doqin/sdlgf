#pragma once
#include "sdlgf.h"
#include "IconButton.h"
#include <memory>
#include <vector>

namespace Demo {
	class GearSlotUI {
	private:
		std::shared_ptr<IconButton> button;
		int gearID = -1;
		bool isCore = false;
		float scale = 3.0f;
		float x = 0, y = 0;

		std::shared_ptr<sdlgf::StaticSprite> frameSprite;
		std::shared_ptr<sdlgf::StaticSprite> glowSprite;
		std::shared_ptr<sdlgf::AnimatedSprite> flameAnim;
		std::shared_ptr<sdlgf::StaticSprite> gearSprite;

	public:
	public:
		GearSlotUI(std::shared_ptr<sdlgf::TransformManager> tm, sdlgf::Camera* cam,
			std::shared_ptr<sdlgf::Texture> uiTex,
			std::shared_ptr<sdlgf::Texture> flameTex,
			std::shared_ptr<sdlgf::Texture> gearTex,
			bool coreSlot, float uiScale = 3.0f);

		void SetLocalPosition(float newX, float newY);
		void SetGearID(int id);
		int GetGearID() const { return gearID; }

		void Update(unsigned long long deltaTime);
		void Draw(sdlgf::GraphicsDevice* gd, sdlgf::Camera* cam, unsigned long long deltaTime);

		std::shared_ptr<IconButton> GetButton() { return button; }
	};
}