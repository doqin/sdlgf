#pragma once
#include "ICommand.h"

namespace sdlgf {
	class DelayCommand : public ICommand {
		const float delayTime;
		float elapsed = 0.f;
	public:
		DelayCommand(float delayTime) : delayTime(delayTime) {}
		void Execute(unsigned long long deltaTime) override;
	};
}