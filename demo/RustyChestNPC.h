#pragma once
#include "pch.h"
#include "INPC.h"

namespace Demo {
	class RustyChestNPC : public INPC {
	private:
		bool isOpened = false;
		std::shared_ptr<sdlgf::Texture> openedTexture;
		std::shared_ptr<sdlgf::AnimatedSprite> openedSprite;
		void MapCharacterVoices(std::unordered_map<std::wstring, std::string>& voiceMap) override {
			voiceMap[L"RustyChest"] = "bleep20";
		}
	public:
		RustyChestNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y);

		void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) override;

		void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) override;

		bool CanInteract() const override { return isPlayerNear && !isOpened; }

		bool GetIsOpened() const { return isOpened; }
		void SetOpened(bool state) { isOpened = state; }
	};
}