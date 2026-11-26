#pragma once
#include "ICommand.h"
#include <functional>
#include <memory>
#include <utility>
#include "IGameObject.h"

namespace sdlgf {
	class DestroyObjectCommand : public ICommand {
	private:
		std::weak_ptr<IGameObject> gameObject;
		std::function<void(void)> cleanup;
	public:
		DestroyObjectCommand(std::weak_ptr<IGameObject> gameObject, std::function<void(void)> cleanup)
			: gameObject(std::move(gameObject)), cleanup(std::move(cleanup)) {}
		void Execute(unsigned long long deltaTime) override;
	};
}