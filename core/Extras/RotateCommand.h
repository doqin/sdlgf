#pragma once
#include "ICommand.h"
#include "IGameObject.h"
#include <memory>

namespace sdlgf {
	class RotateCommand : public ICommand {
	private:
		std::weak_ptr<IGameObject> targetObject;
		float targetRotation;
		float angularVelocity;
	public:
		RotateCommand(std::weak_ptr<IGameObject> gameObject, float targetRotation, float angularVelocity);
		void Execute(unsigned long long deltaTime) override;
	};
}