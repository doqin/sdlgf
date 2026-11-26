#pragma once
#include "pch.h"
#include "INPC.h"

namespace Demo {
    class DauDauNPC : public INPC {
    private:
		void MapCharacterVoices(std::unordered_map<std::wstring, std::string>& voiceMap) override {
			voiceMap[L"DauDau"] = "bleep12";
		}
    public:
        DauDauNPC(std::weak_ptr<sdlgf::TransformManager> tm, float x, float y);

        void Init(sdlgf::GraphicsDevice* gd, sdlgf::Camera* camera, std::shared_ptr<Player> p, std::shared_ptr<sdlgf::ColliderManager> cm, std::shared_ptr<sdlgf::Font> font, std::shared_ptr<sdlgf::CommandBuffer> drawBuffer) override;

        void Draw(const sdlgf::Camera& camera, unsigned long long deltaTime) override;

        bool CanInteract() const override { return isPlayerNear; }
    };
}