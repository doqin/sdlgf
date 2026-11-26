#pragma once
#include <deque>
#include <memory>
#include <mutex>
#include "ICommand.h"
#include "IGameObject.h"
#include "ConcurrentCommand.h"

namespace sdlgf {
	class CommandBuffer {
	private:
		std::mutex mutex;
		std::deque<std::shared_ptr<ICommand>> queue;
		std::shared_ptr<ICommand> currentCommand;
	protected:
	public:
		void PushCommand(std::shared_ptr<ICommand> command);
		void StackCommand(std::shared_ptr<ICommand> command);
		void Update(unsigned long long deltaTime);
		void Clear();
		bool IsBusy();
	};
}