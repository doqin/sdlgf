#include "pch.h"
#include "DestroyObjectCommand.h"

void sdlgf::DestroyObjectCommand::Execute(unsigned long long deltaTime)
{
	if (cleanup) {
		cleanup();
	}
	this->SetObjectState(gameObject, IGameObject::State::Destroyed);
	MarkFinished();
}
