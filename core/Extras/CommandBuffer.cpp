#include "pch.h"
#include "CommandBuffer.h"

void sdlgf::CommandBuffer::PushCommand(std::shared_ptr<ICommand> command)
{
	{
		std::scoped_lock<std::mutex> lock{mutex};
		this->queue.push_back(command);
	}
}

void sdlgf::CommandBuffer::StackCommand(std::shared_ptr<ICommand> command)
{
	std::scoped_lock<std::mutex> lock{ mutex };
	if (this->queue.empty()) {
		this->queue.push_back(command);
		return;
	}
	auto lastCommand = this->queue.back();
	auto concurrent = std::dynamic_pointer_cast<ConcurrentCommand>(lastCommand);
	if (concurrent) {
		concurrent->AddCommand(command);
	}
	else {
		auto newConcurrent = std::make_shared<ConcurrentCommand>();
		newConcurrent->AddCommand(lastCommand);
		newConcurrent->AddCommand(command);
		this->queue.back() = newConcurrent;
	}
}

void sdlgf::CommandBuffer::Update(unsigned long long deltaTime)
{
	if (currentCommand && currentCommand->IsFinished()) {
		currentCommand.reset();
	}

	if (!currentCommand) {
		if (queue.empty()) {
			return;
		}

		currentCommand = queue.front();
		queue.pop_front();
	}

	if (!currentCommand || currentCommand->IsFinished()) {
		currentCommand.reset();
		return;
	}

	currentCommand->Execute(deltaTime);
}

void sdlgf::CommandBuffer::Clear()
{
	queue.clear();
	currentCommand.reset();
}

bool sdlgf::CommandBuffer::IsBusy()
{
	std::scoped_lock<std::mutex> lock{ mutex };
	return currentCommand != nullptr || !queue.empty();
}
