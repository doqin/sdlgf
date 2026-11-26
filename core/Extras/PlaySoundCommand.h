#pragma once
#include "ICommand.h"
#include "../AudioManager.h"
#include <string>

namespace sdlgf {
	class PlaySoundCommand : public ICommand {
	private:
		std::string soundId;
	public:
		PlaySoundCommand(const std::string& soundId);
		void Execute(unsigned long long deltaTime) override;
	};
}