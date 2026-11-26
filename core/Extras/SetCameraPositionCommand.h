#pragma once
#include "ICommand.h"
#include "../Camera.h"

namespace sdlgf {
	class SetCameraPositionCommand : public ICommand {
	private:
		sdlgf::Camera* camera;
		float targetX;
		float targetY;
	public:
		SetCameraPositionCommand(sdlgf::Camera* camera, float x, float y);
		void Execute(unsigned long long deltaTime) override;
	};
}
