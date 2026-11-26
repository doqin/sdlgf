#include "pch.h"
#include "SetPositionCommand.h"

void sdlgf::SetPositionCommand::Execute(unsigned long long deltaTime)
{
	if (auto lock = obj.lock()) {
		lock->SetLocalPosition(x, y);
	}
	MarkFinished();
}
