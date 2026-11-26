#include "pch.h"
#include "DelayCommand.h"

void sdlgf::DelayCommand::Execute(unsigned long long deltaTime)
{
	if (elapsed > delayTime) {
		MarkFinished();
	}
	elapsed += deltaTime / 1000.f;
}
