#pragma once
#include "ICommand.h"
#include <vector>
#include <memory>

namespace sdlgf {
	class ConcurrentCommand : public ICommand {
	private:
		std::vector<std::shared_ptr<ICommand>> commands;
	public:
		ConcurrentCommand();
		void AddCommand(std::shared_ptr<ICommand> command);
		void Execute(unsigned long long deltaTime) override;
	};
}