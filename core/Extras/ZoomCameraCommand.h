#pragma once
#include "ICommand.h"
#include "../Camera.h"

namespace sdlgf {
	class ZoomCameraCommand : public ICommand {
	private:
		sdlgf::Camera* camera;
		float targetZoom;
		float zoomVelocity;
	public:
		ZoomCameraCommand(sdlgf::Camera* camera, float scale, float zoomVelocity);
		void Execute(unsigned long long deltaTime) override;
	};
}