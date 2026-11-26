#include "pch.h"
#include "SetCameraPositionCommand.h"

namespace sdlgf {
	SetCameraPositionCommand::SetCameraPositionCommand(sdlgf::Camera* camera, float x, float y)
		: camera(camera), targetX(x), targetY(y) {
	}

	void SetCameraPositionCommand::Execute(unsigned long long deltaTime) {
		if (!camera) {
			MarkFinished();
			return;
		}

		auto pos = camera->GetPosition();
		pos.x = targetX;
		pos.y = targetY;
		camera->SetPosition(pos);

		MarkFinished();
	}
}
