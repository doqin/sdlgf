#pragma once
#include "ICommand.h"
namespace sdlgf {
	class CustomCommand : public ICommand {
		std::function<void(std::function<void(void)> markFinished)> customCommand;
	public:
		CustomCommand(std::function<void(std::function<void(void)> markFinished)> customCommand) : customCommand(customCommand) {}
		void Execute(unsigned long long deltaTime) override;
	};
}