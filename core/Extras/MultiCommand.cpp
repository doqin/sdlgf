#include "pch.h"
#include "MultiCommand.h"

sdlgf::MultiCommand::MultiCommand(std::vector<std::shared_ptr<ICommand>>&& commandList)
{
	for (auto& command : commandList) {
		queue.push(command);
	}
}

void sdlgf::MultiCommand::Execute(unsigned long long deltaTime)
{
	if (currentCommand) {
		currentCommand->Execute(deltaTime);
		if (currentCommand->IsFinished()) {
			if (queue.size() > 0) {
				currentCommand = queue.front();
				queue.pop();
			}
			else {
				MarkFinished();
				currentCommand.reset();
			}
		}
	}
	else {
		if (queue.size() > 0) {
			currentCommand = queue.front();
			queue.pop();
		}
		else {
			MarkFinished();
		}
	}
}
