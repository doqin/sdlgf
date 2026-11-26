#pragma once
#include <memory>
#include "IGameObject.h"

namespace sdlgf {
	class ICommand {
	private:
		bool isFinished = false;
	protected:
		void MarkFinished();
		void SetObjectState(std::weak_ptr<IGameObject> object, IGameObject::State state);
	public:
		virtual ~ICommand() = default;
		virtual void Execute(unsigned long long deltaTime) = 0;
		bool IsFinished() const;
	};
}