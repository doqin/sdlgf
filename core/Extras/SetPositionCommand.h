#pragma once
#include "ICommand.h"
#include "IGameObject.h"

namespace sdlgf {
	class SetPositionCommand : public ICommand {
		std::weak_ptr<IGameObject> obj;
		float x, y;
	public:
		SetPositionCommand(std::weak_ptr<IGameObject> obj, float x, float y) : obj(obj), x(x), y(y) {}
		void Execute(unsigned long long deltaTime) override;
	};
}