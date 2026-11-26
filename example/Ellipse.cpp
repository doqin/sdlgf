#include "pch.h"
#include "Ellipse.h"

void GO::Ellipse::Update(unsigned long long deltaTime)
{
	trigger->Update(deltaTime);
}

std::weak_ptr<sdlgf::EllipseCollider> GO::Ellipse::GetCollider()
{
	return collider;
}

void GO::Ellipse::Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, std::shared_ptr<sdlgf::ColliderManager> colliderManager)
{
    this->graphicsDevice = graphicsDevice;
    this->camera = camera;
    collider = std::make_shared<sdlgf::EllipseCollider>(
        transformManager,
        shared_from_this(),
        width,
        height,
        0,
        0
    );
    collider->SetOriginCenter();
    colliderManager->Add(collider);
    trigger = std::make_shared<sdlgf::EllipseTrigger>(
        transformManager,
        shared_from_this(),
        width,
        height,
        0,
        0
    );
    trigger->SetOriginCenter();
    trigger->Init(camera);
    trigger->SetOnHeldRight([](sdlgf::ITrigger* thisObject) {
        auto parent = dynamic_pointer_cast<Ellipse>(thisObject->GetParent().value().lock());
        auto input = sdlgf::InputManager::GetInstance();
        auto deltaX = input->GetRelativeMouseX();
        parent->SetLocalRotation(parent->GetLocalRotation() + deltaX * 0.1f);
        });
    trigger->SetOnHeldLeft([colliderManager](sdlgf::ITrigger* thisObject) {
        auto parent = dynamic_pointer_cast<Ellipse>(thisObject->GetParent().value().lock());
        auto input = sdlgf::InputManager::GetInstance();
        auto deltaX = input->GetRelativeMouseX();
        auto deltaY = input->GetRelativeMouseY();
        auto localX = parent->GetLocalX();
        auto localY = parent->GetLocalY();
        auto parentCollider = parent->GetCollider().lock();
        float nextWorldX = parentCollider->GetWorldX() + deltaX;
        float nextWorldY = parentCollider->GetWorldY() + deltaY;
        auto safePos = colliderManager->GetSafePosition(parentCollider, nextWorldX, nextWorldY);
        if (safePos.has_value()) {
            auto& [posX, posY] = safePos.value();
            deltaX = posX - parentCollider->GetWorldX();
            deltaY = posY - parentCollider->GetWorldY();
        }

        parent->SetLocalPosition(
            localX + deltaX,
            localY + deltaY
        );
        });
}

void GO::Ellipse::Draw(unsigned long long deltaTime)
{
	graphicsDevice->DrawEllipse(
		*camera, 
		GetWorldX(), 
		GetWorldY(), 
		width, 
		height, 
		GetWorldRotation(),
		GetWorldScaleX(),
		GetWorldScaleY(),
		width / 2,
		height / 2,
		0xFF00FF00, 
		true
	);
	trigger->Draw(graphicsDevice, *camera);
	collider->Draw(graphicsDevice, *camera);
	
}

void GO::Ellipse::Dispose()
{
}
