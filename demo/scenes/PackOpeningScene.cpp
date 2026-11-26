#include "pch.h"
#include "CardRegistry.h"
#include "PackOpeningScene.h"
#include "MainFont.h"
#include "LocalizationManager.h"
#include "ICard.h"
#include "RNG.h"
#include "SettingsManager.h"
#include <InputManager.h>
#include "AudioManager.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>

namespace Demo {

	namespace {
		constexpr float TWO_PI = 6.2831853f;
		constexpr float CARD_SCALE = 4.f;
		constexpr float PACK_SCALE = 4.f;
		constexpr float FADE_MS = 260.f;
		constexpr int   BURST_COUNT = 48;

		BYTE ChR(sdlgf::Color c) { return (c >> 16) & 0xFF; }
		BYTE ChG(sdlgf::Color c) { return (c >> 8) & 0xFF; }
		BYTE ChB(sdlgf::Color c) { return c & 0xFF; }
	}

	PackOpeningScene::PackOpeningScene(Game* game, int screenWidth, int screenHeight, std::vector<std::string> cardIds)
		: IScene(screenWidth, screenHeight), game(game), cardIds(std::move(cardIds))
	{
	}

	void PackOpeningScene::Init()
	{
		auto gd = game->GetGraphicsDevice();
		transformManager = std::make_shared<sdlgf::TransformManager>();

		packTex = gd->GetAssets().GetTexture(L"assets/packopening-Sheet.png");
		// 992x64 sheet -> 31 frames, 32x64 each. Non-looping: it plays once and we watch for the end.
		packSprite = std::make_shared<sdlgf::AnimatedSprite>(
			packTex, sdlgf::Utils::CreateRectsHorizontal(0, 0, 32, 64, 31), 18, false);
		packSprite->SetOrigin(16.f, 32.f);
		packSprite->SetPosition(0.f, 0.f);
		packSprite->SetScale(PACK_SCALE, PACK_SCALE);

		cardFrame = CardFrame::Get(gd);

		font = gd->GetAssets().GetFont(Demo::kMainFontName, Demo::kLargeFontSize);
		fontSprite = std::make_shared<sdlgf::FontSprite>(font);

		particleTex = std::make_shared<sdlgf::Texture>(gd);
		particleTex->CreatePlainTexture(0xFFFFFFFF, 4, 4);
		explosionEmitter = std::make_unique<sdlgf::ParticleSystem>(particleTex, 80);
		explosionEmitter->SetOrigin(2.f, 2.f);
		sdlgf::ConfigureExplosionEmitter(*explosionEmitter);

		for (const auto& id : cardIds) {
			auto card = CardRegistry::Get().Create(id, transformManager);
			faces.push_back(card ? CardFace{ true, card->GetCardTemplate(), card->GetDisplayName(), CardInputSignature(card->GetInputSlotCount(), 0), card->GetCost() } : CardFace{});
			rarities.push_back(CardCatalog::GetRarity(id));
		}

		transformManager->RebuildHierarchy();
	}

	void PackOpeningScene::Update(unsigned long long deltaTime)
	{
		auto inpMan = sdlgf::InputManager::GetInstance();
		inpMan->ReadMouse(deltaTime);
		inpMan->ReadKeyboard(deltaTime);
		uiCamera.Update();

		// A left click, the ACCEPT key or the INTERACT key all step the reveal forward.
		const int keyAccept = SettingsManager::GetInstance()->GetKeybind("ACCEPT");
		const int keyInteract = SettingsManager::GetInstance()->GetKeybind("INTERACT");
		bool advance = false;
		if (inpMan->MouseDown(sdlgf::InputManager::MouseButton::Left)) {
			inpMan->ConsumeMouseButton(sdlgf::InputManager::MouseButton::Left);
			advance = true;
		}
		else if (inpMan->KeyDown(keyAccept)) {
			inpMan->ConsumeKey(keyAccept);
			advance = true;
		}
		else if (inpMan->KeyDown(keyInteract)) {
			inpMan->ConsumeKey(keyInteract);
			advance = true;
		}

		const float cx = 0.f, cy = 0.f;

		if (state == State::Opening) {
			if (packSprite->IsFinished() && !burstFired) {
				burstFired = true;
				for (int i = 0; i < BURST_COUNT; ++i) {
					float ang = RNG::Range(0.f, TWO_PI);
					float speed = RNG::Range(120.f, 380.f);
					explosionEmitter->Spawn(cx, cy, ang,
						RNG::Range(0.8f, 1.7f), RNG::Range(0.8f, 1.7f), 0xFFFFFFFF,
						std::cos(ang) * speed, std::sin(ang) * speed);
				}
				sdlgf::AudioManager::GetInstance()->Play("shop_buy", false, 0.7f);
				holdTimer = 320.f;
				state = State::Revealing;
				revealIndex = 0;
				cardAlpha = 0.f;
				pendingCardSound = true;
			}
		}
		else if (state == State::Revealing) {
			if (holdTimer > 0.f) {
				holdTimer -= (float)deltaTime;
			}
			else {
				if (pendingCardSound) {
					sdlgf::AudioManager::GetInstance()->PlayRandom("card_draw", 0.5f);
					pendingCardSound = false;
				}
				cardAlpha = (std::min)(1.f, cardAlpha + (float)deltaTime / FADE_MS);
				if (advance) {
					if (cardAlpha >= 1.f) {
						++revealIndex;
						if (revealIndex >= (int)cardIds.size()) {
							state = State::Done;
						}
						else {
							cardAlpha = 0.f;
							pendingCardSound = true;
						}
					}
					else {
						cardAlpha = 1.f; // snap the current card in on an early input
					}
				}
			}
		}
		else { // Done
			if (advance) {
				shouldLeave = true;
			}
		}

		// Age/fade the burst every frame (active=false: never auto-emit).
		explosionEmitter->Update(deltaTime, cx, cy, 0.f, 1.f, 1.f, 0xFFFFFFFF, false);

		if (shouldLeave) {
			auto sceMan = game->GetSceneManager();
			sceMan->RemoveScene(sceMan->GetIndex());
			sceMan->GoToPrevious();
		}
	}

	void PackOpeningScene::DrawWorld(unsigned long long deltaTime)
	{
		// The scene underneath still renders its own world.
	}

	void PackOpeningScene::DrawUI(unsigned long long deltaTime)
	{
		auto gd = game->GetGraphicsDevice();
		const float vw = (float)game->GetVirtualWidth();
		const float vh = (float)game->GetVirtualHeight();

		if (!gd->BeginDraw()) return;

		gd->SetAlphaBlending(true);
		gd->DrawRectangle(uiCamera, -vw / 2.f, -vh / 2.f, vw, vh, COLOR_ARGB(210, 0, 0, 0), true);

		if (state == State::Opening) {
			packSprite->SetPosition(0.f, 0.f);
			packSprite->SetScale(PACK_SCALE, PACK_SCALE);
			packSprite->Begin();
			packSprite->Draw(uiCamera, deltaTime);
			packSprite->End();
		}

		explosionEmitter->Draw(uiCamera, deltaTime);

		if (state == State::Revealing || state == State::Done) {
			const int idx = (std::min)(revealIndex, (int)cardIds.size() - 1);
			const CardFace& face = faces[idx];
			const float fw = face.valid ? cardFrame->MeasureWidth(face.name, face.inputs, CARD_SCALE) : 0.f;
			const float fh = face.valid ? CardFrame::Height(CARD_SCALE) : 0.f;
			const BYTE a = (BYTE)((std::clamp)(cardAlpha, 0.f, 1.f) * 255.f);
			const sdlgf::Color rc = CardCatalog::RarityColor(rarities[idx]);

			if (fw > 0.f && fh > 0.f) {
				const float gw = fw + 48.f;
				const float gh = fh + 48.f;
				gd->DrawRectangle(uiCamera, -gw / 2.f, -gh / 2.f - 10.f, gw, gh,
					COLOR_ARGB((BYTE)(a * 0.45f), ChR(rc), ChG(rc), ChB(rc)), true);

				cardFrame->Draw(uiCamera, deltaTime, -fw / 2.f, -10.f - fh / 2.f, face.cardTemplate, face.name,
					face.inputs, face.cost, CARD_SCALE, COLOR_ARGB(a, 255, 255, 255));
			}

			fontSprite->Begin();

			fontSprite->SetScale(1.2f, 1.2f);
			fontSprite->SetOutline(true, 0xFF000000, 2.f);
			fontSprite->SetColor(COLOR_ARGB(a, ChR(rc), ChG(rc), ChB(rc)));
			fontSprite->SetText(CardCatalog::RarityName(rarities[idx]));
			const float labelW = fontSprite->GetWidth() * 1.2f;
			fontSprite->SetPosition(-labelW / 2.f, fh / 2.f + 8.f);
			fontSprite->Draw(uiCamera, deltaTime);

			fontSprite->SetScale(0.9f, 0.9f);
			fontSprite->SetOutline(true, 0xFF000000, 2.f);
			fontSprite->SetColor(0xFFFFFFFF);
			const std::wstring hint = state == State::Done
				? Tr(L"Click or press a key to close")
				: Tr(L"Card ") + std::to_wstring(revealIndex + 1) + L" / " + std::to_wstring(cardIds.size())
				+ Tr(L"   -   click or press a key for next");
			fontSprite->SetText(hint);
			const float hintW = fontSprite->GetWidth() * 0.9f;
			fontSprite->SetPosition(-hintW / 2.f, vh / 2.f - 60.f);
			fontSprite->Draw(uiCamera, deltaTime);

			fontSprite->End();
		}

		gd->SetAlphaBlending(false);
		gd->EndDraw();
	}
}
