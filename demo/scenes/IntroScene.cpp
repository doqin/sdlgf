#include "pch.h"
#include "IntroScene.h"
#include "MainFont.h"
#include "LocalizationManager.h"
#include "MainMenu.h"
#include "SaveGameState.h"

void Demo::IntroScene::Init() {
    font = game->GetGraphicsDevice()->GetAssets().GetFont(Demo::kMainFontName, Demo::kMainFontSize);
    fontSprite = std::make_shared<sdlgf::FontSprite>(font);

    auto [sw, sh] = camera.GetScreenResolution();
    conversation = std::make_shared<IConversation>(fontSprite, sw, sh);

    conversation->AddLine({ .name = Tr(L"???"), .content = Tr(L"Ugh... where... where am I?"), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"The last thing I remember... I clicked on that suspicious link in the chat."), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"\"Free Robux!\" it said. Of course it was too good to be true."), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"But before I could close the page, my screen started glitching..."), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"Lines of code flashed before my eyes, and then everything went black."), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"And now... I'm inside some kind of digital world?"), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"Everything around me looks like a corrupted program. I need to find a way out."), .voiceClip = std::optional<std::string>("bleep8") });
    conversation->AddLine({ .name = Tr(L"Player"), .content = Tr(L"Wait... I can see something in the distance. Maybe I should head that way."), .voiceClip = std::optional<std::string>("bleep8") });
}

void Demo::IntroScene::Update(unsigned long long deltaTime) {
    auto inpMan = sdlgf::InputManager::GetInstance();
    inpMan->ReadMouse(deltaTime);
    inpMan->ReadKeyboard(deltaTime);

    if (conversation) {
        conversation->Execute(deltaTime);
        if (conversation->IsFinished()) {
            conversation = nullptr;
        }
    }

    if (!conversation && !hasTransitioned) {
        hasTransitioned = true;
        sdlgf::AudioManager::GetInstance()->PlayBGM_Fade("bgm_tutorial", 0.5f, 5.0f);
        // Builds the first world behind a loading screen; the music started above carries over
        MainMenu::gameSaveState->TravelTo(SaveGameState::DEFAULT_WORLD, {});
    }
}

void Demo::IntroScene::DrawWorld(unsigned long long deltaTime) {
    auto gd = game->GetGraphicsDevice();

    if (gd->BeginDraw()) {
        gd->EndDraw();
    }
}

void Demo::IntroScene::DrawUI(unsigned long long deltaTime)
{
    auto gd = game->GetGraphicsDevice();

    if (gd->BeginDraw()) {

        if (conversation) {
            conversation->Draw(gd, &this->uiCamera, deltaTime);
        }

        sdlgf::InputManager::GetInstance()->DrawCursor(&this->uiCamera, deltaTime);

        gd->EndDraw();
    }
}