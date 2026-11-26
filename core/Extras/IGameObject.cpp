#include "pch.h"
#include "IGameObject.h"

sdlgf::IGameObject::IGameObject(std::weak_ptr<TransformManager> transformManager) : transformManager(transformManager)
{
    this->transformHandle = CreateTransform();
}

sdlgf::IGameObject::IGameObject(std::weak_ptr<TransformManager> transformManager, float x, float y, float rotation, float scaleX, float scaleY) : transformManager(transformManager)
{
    this->transformHandle = CreateTransform(x, y, rotation, scaleX, scaleY);
}

sdlgf::IGameObject::IGameObject(std::weak_ptr<TransformManager> transformManager, std::weak_ptr<IGameObject> parent, float x, float y, float rotation, float scaleX, float scaleY) : transformManager(transformManager)
{
    this->transformHandle = CreateTransform(parent, x, y, rotation, scaleX, scaleY);
}

sdlgf::IGameObject::~IGameObject()
{
    auto lock = this->transformManager.lock();
    if (lock) lock->DestroyTransform(this->GetTransformHandle());
}

bool sdlgf::IGameObject::operator==(const IGameObject& other) const
{
    return transformHandle.slotIndex == other.transformHandle.slotIndex && transformHandle.generation == other.transformHandle.generation;
}

std::optional<std::weak_ptr<sdlgf::IGameObject>> sdlgf::IGameObject::GetParent() const
{
    return parent;
}

sdlgf::TransformHandle sdlgf::IGameObject::GetTransformHandle() const
{
    return transformHandle;
}

void sdlgf::IGameObject::SetTransformHandle(TransformHandle handle)
{
    this->transformHandle = handle;
}

std::weak_ptr<sdlgf::TransformManager> sdlgf::IGameObject::GetTransformManager()
{
    return transformManager;
}

void sdlgf::IGameObject::SetTransformManager(std::weak_ptr<TransformManager> transformManager)
{
    this->transformManager = transformManager;
}

sdlgf::TransformHandle sdlgf::IGameObject::CreateTransform()
{
    if (parent.has_value()) parent = std::nullopt;
    return transformManager.lock()->CreateTransform();
}

sdlgf::TransformHandle sdlgf::IGameObject::CreateTransform(float x, float y, float rotation, float scaleX, float scaleY)
{
    if (parent.has_value()) parent = std::nullopt;
    return transformManager.lock()->CreateTransform(NO_PARENT, x, y, rotation, scaleX, scaleY);
}

sdlgf::TransformHandle sdlgf::IGameObject::CreateTransform(std::weak_ptr<IGameObject> parent, float x, float y, float rotation, float scaleX, float scaleY)
{
    this->parent = parent;
    return transformManager.lock()->CreateTransform(parent.lock()->GetTransformHandle().slotIndex, x, y, rotation, scaleX, scaleY);
}

void sdlgf::IGameObject::SetLocalX(float x)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localX = x;
}

void sdlgf::IGameObject::SetLocalY(float y)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localY = y;
}

void sdlgf::IGameObject::SetLocalPosition(float x, float y)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localX = x;
    transformData.localY = y;
}

void sdlgf::IGameObject::SetLocalRotation(float r)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localRotation = r;
}

void sdlgf::IGameObject::SetLocalScale(float sx, float sy)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localScaleX = sx;
    transformData.localScaleY = sy;
}

void sdlgf::IGameObject::SetLocalScaleX(float sx)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localScaleX = sx;
}

void sdlgf::IGameObject::SetLocalScaleY(float sy)
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    transformData.localScaleY = sy;
}

float sdlgf::IGameObject::GetLocalX() const
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.localX;
}

float sdlgf::IGameObject::GetLocalY() const
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.localY;
}

std::tuple<float, float> sdlgf::IGameObject::GetLocalPosition()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return std::tuple<float, float>(transformData.localX, transformData.localY);
}

float sdlgf::IGameObject::GetWorldX() const
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.worldX;
}

float sdlgf::IGameObject::GetWorldY() const
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.worldY;
}

std::tuple<float, float> sdlgf::IGameObject::GetWorldPosition()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return std::tuple<float, float>(transformData.worldX, transformData.worldY);
}

float sdlgf::IGameObject::GetLocalRotation() const
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.localRotation;
}

float sdlgf::IGameObject::GetWorldRotation() const
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.worldRotation;
}

std::tuple<float, float> sdlgf::IGameObject::GetLocalScale()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return std::tuple<float, float>(transformData.localScaleX, transformData.localScaleY);
}

float sdlgf::IGameObject::GetLocalScaleX()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.localScaleX;
}

float sdlgf::IGameObject::GetLocalScaleY()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.localScaleY;
}

std::tuple<float, float> sdlgf::IGameObject::GetWorldScale()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return std::tuple<float, float>(transformData.worldScaleX, transformData.worldScaleY);
}

float sdlgf::IGameObject::GetWorldScaleX()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.worldScaleX;
}

float sdlgf::IGameObject::GetWorldScaleY()
{
    auto lock = transformManager.lock();
    auto& transformData = lock->GetTransform(transformHandle.slotIndex);
    return transformData.worldScaleY;
}

sdlgf::IGameObject::State sdlgf::IGameObject::GetState() const {
    return state;
}
