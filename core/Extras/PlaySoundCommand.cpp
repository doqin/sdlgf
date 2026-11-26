#include "pch.h"
#include "PlaySoundCommand.h"

sdlgf::PlaySoundCommand::PlaySoundCommand(const std::string& soundId) : soundId(soundId)
{
}

void sdlgf::PlaySoundCommand::Execute(unsigned long long deltaTime)
{
	AudioManager::GetInstance()->Play(soundId);

	MarkFinished();
}