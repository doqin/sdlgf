#include "pch.h"
#include "CustomCommand.h"

void sdlgf::CustomCommand::Execute(unsigned long long deltaTime)
{
	customCommand([this]() { MarkFinished(); });
}
