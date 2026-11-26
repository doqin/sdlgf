#pragma once
#include <string>
#include <optional>
#include <memory>
#include <ISprite.h>

namespace Demo {
	struct DialogueLine {
		std::optional<std::shared_ptr<sdlgf::ISprite>> left;
		std::optional<std::shared_ptr<sdlgf::ISprite>> right;
		std::wstring name;
		std::wstring content;
		std::optional<std::string> voiceClip; // Optional voice clip associated with the line
	};
}
