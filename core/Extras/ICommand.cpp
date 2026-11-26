#include "pch.h"
#include "ICommand.h"

void sdlgf::ICommand::MarkFinished()
{
    isFinished = true;
}

void sdlgf::ICommand::SetObjectState(std::weak_ptr<IGameObject> object, IGameObject::State state)
{
    if (auto lock = object.lock()) {
        lock->state = state;
    }
}

bool sdlgf::ICommand::IsFinished() const
{
    return isFinished;
}
