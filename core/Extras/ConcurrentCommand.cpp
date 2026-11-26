#include "pch.h"
#include "ConcurrentCommand.h"

sdlgf::ConcurrentCommand::ConcurrentCommand()
{
}

void sdlgf::ConcurrentCommand::AddCommand(std::shared_ptr<ICommand> command)
{
	commands.push_back(command);
}

void sdlgf::ConcurrentCommand::Execute(unsigned long long deltaTime)
{
	bool allFinished = true;

	for (auto& cmd : commands) {
		if (!cmd->IsFinished()) {
			cmd->Execute(deltaTime);

			if (!cmd->IsFinished()) {
				allFinished = false;
			}
		}
	}

	if (allFinished) {
		MarkFinished();
	}
}