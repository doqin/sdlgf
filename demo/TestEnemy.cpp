#include "pch.h"
#include "TestEnemy.h"
#include "PopUpMessage.h"
#include "resource.h"
#include "RNG.h"

void Demo::TestEnemy::Init(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera)
{
	texture = std::make_shared<sdlgf::Texture>(graphicsDevice);
	texture->LoadTexture(L"assets/placeholder.png");
	sprite = std::make_shared<sdlgf::StaticSprite>(texture.get());
	sprite->SetOrigin(32, 32);

	roundProjectileTexture = std::make_shared<sdlgf::Texture>(graphicsDevice);
	roundProjectileTexture->LoadTexture(L"assets/placeholder-round-projectile.png");

	SetGoldReward(static_cast<int>(std::round(GetMaxHealth())));
	InitCardSpawnTrigger(camera, 64.f, 64.f);
}

void Demo::TestEnemy::Draw(sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, unsigned long long deltaTime)
{
	if (sprite) {
		sprite->Begin();
		auto [x, y] = GetWorldPosition();
		sprite->SetPosition(x, y);
		sprite->Draw(*camera, deltaTime);
		sprite->End();
	}
	IEnemy::Draw(graphicsDevice, camera, deltaTime);
}

void Demo::TestEnemy::OnTurnBegin(std::shared_ptr<Player> player, std::shared_ptr<PopUpMessage> popUpMessage, int currentTurn) {
	this->player = player;

	int cycle = (currentTurn - 1) / 3;

	if (currentCycle != cycle) {
		currentCycle = cycle;

		skillTurnThisCycle = 1;
	}

	int turnInCycle = (currentTurn - 1) % 3 + 1;


	if (turnInCycle == skillTurnThisCycle) {
		CastAbility([this]() { 
			//this->Heal(20.f);

			//this->AddModifier(ModifierType::BuffDamage, 2, 5.0f, true);
			//this->AddModifier(ModifierType::BuffDefense, 2, 10.0f, true);

			//this->player.lock()->AddModifier(ModifierType::Poison, 2, 5.0f, false);
			//this->player.lock()->AddModifier(ModifierType::Vulnerable, 2, 0.f, false);
			//this->player.lock()->AddModifier(ModifierType::Weak, 2, 0.f, false);

			if (this->onRequestLockCard) {
				this->onRequestLockCard(2);
			}

			}, popUpMessage, L"TestEnemy used an ability!");
	}
}

void Demo::TestEnemy::StartAttack(std::shared_ptr<Player> player, std::vector<std::shared_ptr<IEnemy>>* enemies, std::shared_ptr<PopUpMessage> popUpMessage, sdlgf::GraphicsDevice* graphicsDevice, sdlgf::Camera* camera, int currentTurn) {
	(void)enemies;
	(void)popUpMessage;
	(void)graphicsDevice;
	(void)camera;
	this->player = player;

	float baseDamage = 5.f;

	for (int i = 0; i < 40; i++) {
		auto x = RNG::Range(-200, 200);
		auto y = RNG::Range(-200, 200);

		commandBuffer.PushCommand(std::make_shared<sdlgf::CustomCommand>([this, x, y, baseDamage](std::function<void(void)> markFinished) {
			if (auto lock = this->player.lock()) {
				float finalDamage = this->CalculateOutgoingDamage(baseDamage);

				projectiles.Spawn(
					lock,
					ProjectileDesc(roundProjectileTexture.get(), 8, 8, 16, 16, x, y)
					.SetTargetPosition(lock->GetCollider().lock()->GetWorldX(), lock->GetCollider().lock()->GetWorldY())
					.SetDelay(.2f)
					.SetDecayTime(4.f)
					.SetVelocity(200.f)
					.SetDamage(finalDamage)
				);
			}
			else {
				throw std::runtime_error("player is null");
			}
			markFinished();
			}));
		commandBuffer.PushCommand(std::make_shared<sdlgf::DelayCommand>(0.1f));
	}
	commandBuffer.PushCommand(std::make_shared<sdlgf::DelayCommand>(5.f));
}