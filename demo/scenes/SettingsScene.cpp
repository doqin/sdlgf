#include "pch.h"
#include "SettingsScene.h"
#include "IconButton.h"
#include "SettingsManager.h"
#include "MainFont.h"
#include "LocalizationManager.h"
#include <algorithm>
#include <cmath>
#include "Keys.h"
#include <SDL3/SDL_keyboard.h>
namespace Demo
{
	//Extra helpers, use to get keyname and string
	std::wstring ToWString(const std::string& s)
	{
		return sdlgf::Utils::Utf8ToWide(s);
	}
	std::string GetKeyName(int dikCode)
	{
		if (dikCode <= 0 || dikCode > 255) return "None";
		// SDL's own name for the key (Win32's GetKeyNameTextA is Windows-only)
		const int scancode = sdlgf::Keys::ToSDLScancode(dikCode);
		const char* name = scancode != 0 ? SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode)) : "";
		if (name != nullptr && name[0] != '\0') return std::string(name);
		return "Key " + std::to_string(dikCode);
	}

	//Draw functions for scene (to avoid code duplication)
	void SettingsScene::DrawString(std::wstring text, float x, float y, sdlgf::Color color, DWORD format)
	{
		int screenX = (int)(lastScreenWidth / 2.0f + x);
		int screenY = (int)(lastScreenHeight / 2.0f + y);
		fontSprite->Begin();
		fontSprite->SetColor(color);
		fontSprite->SetPosition(x, y);
		fontSprite->SetText(std::move(text));
		fontSprite->Draw(uiCamera, 0);
		fontSprite->End();
	}

	void SettingsScene::DrawBackground(unsigned long long deltaTime)
	{
		auto [screenWidth, screenHeight] = camera.GetScreenResolution();

		game->GetGraphicsDevice()->DrawRectangle(0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0xFF000000, true);

		const sdlgf::Color polyColor = 0xFF9cdb43;

		static float timeAcc = 0.0f;
		timeAcc += static_cast<float>(deltaTime) * 0.001f;

		for (int i = 0; i < 15; ++i) {
			float size = 100.0f + (i % 3) * 50.0f;
			float margin = size * 2.0f;
			float bx = std::fmod((i * 123.0f) + timeAcc * 10.0f, static_cast<float>(screenWidth) + margin * 2.0f) - margin;
			float by = std::fmod((i * 456.0f) + timeAcc * 5.0f, static_cast<float>(screenHeight) + margin * 2.0f) - margin;
			float angle = timeAcc * 0.1f + i;

			float glitchSize = size + std::sinf(timeAcc * 2.0f + i) * 5.0f;

			float prevX = bx + std::cosf(angle) * glitchSize;
			float prevY = by + std::sinf(angle) * glitchSize;

			for (int v = 1; v <= 5; ++v) {
				float vAngle = angle + (v * 72.0f * 3.14159f / 180.0f);
				float vx = bx + std::cosf(vAngle) * glitchSize;
				float vy = by + std::sinf(vAngle) * glitchSize;

				game->GetGraphicsDevice()->DrawLine(prevX, prevY, vx, vy, polyColor);
				prevX = vx;
				prevY = vy;
			}
		}
	}

	void SettingsScene::DrawVolumeTrack(std::shared_ptr<sdlgf::NineSliceSprite> bg, std::shared_ptr<sdlgf::NineSliceSprite> fill, float vol, RECT originalRect, unsigned long long deltaTime)
	{
		if (bg)
		{
			bg->Begin();
			bg->Draw(uiCamera, deltaTime);
			bg->End();
		}
		//Only draw the fill bar if volume > 0 to prevent visual glitches
		if (fill && bg && vol > 0.001f)
		{
			float fillWidth = SLIDER_DESIRED_WIDTH * vol;
			fill->SetTargetSize(fillWidth, 7.0f);

			fill->Begin();
			fill->Draw(uiCamera, deltaTime);
			fill->End();
		}
	}

	bool SettingsScene::IsAnyKeybindListening() const
	{
		return std::any_of(keybindRows.begin(), keybindRows.end(), [](const KeybindRow& row) { return row.listening; });
	}

	SettingsScene::Layout SettingsScene::ComputeLayout(int screenH) const
	{
		Layout l{};
		float y = -screenH / 2.0f + HEADING_TOP;

		l.audioHeadingY = y;
		l.masterY = y + HEADING_H;
		l.musicY = l.masterY + ROW_H;
		l.sfxY = l.musicY + ROW_H;

		l.videoHeadingY = l.sfxY + ROW_H + SECTION_GAP;
		l.displayY = l.videoHeadingY + HEADING_H;
		l.resolutionY = l.displayY + ROW_H;
		l.rendererY = l.resolutionY + ROW_H;
		l.vsyncY = l.rendererY + ROW_H;

		l.generalHeadingY = l.vsyncY + ROW_H + SECTION_GAP;
		l.languageY = l.generalHeadingY + HEADING_H;
		l.moveSprintY = l.languageY + ROW_H;
		l.moveSlowY = l.moveSprintY + ROW_H;
		l.footnoteY = l.moveSlowY + ROW_H;

		l.controlsHeadingY = y;
		l.keybindStartY = y + HEADING_H;
		return l;
	}

	void SettingsScene::DrawHeading(const std::wstring& text, float x, float y, float width)
	{
		const sdlgf::Color accent = 0xFF9cdb43;
		DrawString(text, x, y, accent);
		game->GetGraphicsDevice()->DrawRectangle(uiCamera, x, y + 22.0f, width, 1.0f, accent, true);
	}

	int SettingsScene::CurrentBackendIndex() const
	{
		const std::string& saved = SettingsManager::GetInstance()->GetRendererBackend();
		for (size_t i = 0; i < backendOptions.size(); ++i) {
			if (backendOptions[i] == saved) return static_cast<int>(i);
		}
		return 0; // a driver this build does not have is shown (and replaced) as Auto
	}

	void SettingsScene::SetBackendIndex(int index)
	{
		if (index < 0 || index >= static_cast<int>(backendOptions.size())) return;
		auto sm = SettingsManager::GetInstance();
		sm->SetRendererBackend(backendOptions[index]);
		sm->SaveSettings();
	}

	std::vector<KeyboardNavigator::Candidate> SettingsScene::CollectKeyboardCandidates()
	{
		std::vector<KeyboardNavigator::Candidate> candidates;

		auto addButton = [&](std::shared_ptr<IButton> button) {
			if (!button || button->GetState() == IButton::ButtonState::DISABLED) {
				return;
			}
			candidates.push_back({
				button,
				button->GetWorldX(),
				button->GetWorldY(),
				(float)button->GetWidth(),
				(float)button->GetHeight(),
				[button]() { button->Activate(); }
				});
			};

		for (auto& button : uiButtons) {
			addButton(button);
		}

		auto sm = SettingsManager::GetInstance();
		if (!sm->GetFullscreen()) {
			const int resIdx = sm->GetCurrentResolutionIndex();
			const int maxIdx = static_cast<int>(sm->GetSupportedResolutions().size()) - 1;
			if (resIdx > 0) {
				addButton(btnResPrev);
			}
			if (resIdx < maxIdx) {
				addButton(btnResNext);
			}
		}

		const int backendIdx = CurrentBackendIndex();
		if (backendIdx > 0) {
			addButton(btnRendPrev);
		}
		if (backendIdx < static_cast<int>(backendOptions.size()) - 1) {
			addButton(btnRendNext);
		}

		return candidates;
	}

	//Update functions for component
	void SettingsScene::ResetListening()
	{
		for (auto& row : keybindRows) {
			row.listening = false;
			if (row.button) row.button->SetState(Demo::IButton::ButtonState::IDLE);
		}
	}

	void SettingsScene::UpdateLayout(int screenW, int screenH)
	{
		//Background
		float bgImageW = (float)bgTex->GetWidth();
		float bgImageH = (float)bgTex->GetHeight();
		if (!bgSprite)
		{
			bgSprite = std::make_shared<sdlgf::StaticSprite>(bgTex);
			bgSprite->SetSrcRect({ 0, 0, (LONG)bgImageW, (LONG)bgImageH });
		}
		bgSprite->SetScale(std::max(screenW / bgImageW, screenH / bgImageH));
		bgSprite->SetOrigin(bgImageW / 2.0f, bgImageH / 2.0f);
		bgSprite->SetPosition(0, 0);

		//UI Elements
		const Layout l = ComputeLayout(screenH);

		//LOCAL FUNCTION to set Row Slider positions to keep the code clean
		auto SetVolumeRowPosition = [&](std::shared_ptr<sdlgf::NineSliceSprite> track, std::shared_ptr<sdlgf::NineSliceSprite> fill,
			std::shared_ptr<IconButton> btnD, std::shared_ptr<IconButton> btnI, float y)
			{
				float gap = 5.0f;
				float btnWidth = 12.0f;
				float trackAlignY = y + 5.0f;

				if (track)
				{
					track->SetPosition(SLIDER_COLUMN_X, trackAlignY);
					track->SetTargetSize(SLIDER_DESIRED_WIDTH, 7.0f);
				}

				if (fill) fill->SetPosition(SLIDER_COLUMN_X, trackAlignY);

				if (btnD) btnD->SetLocalPosition(SLIDER_COLUMN_X - btnWidth - gap, trackAlignY + (7.0f - btnD->GetHeight()) / 2.0f);
				if (btnI) btnI->SetLocalPosition(SLIDER_COLUMN_X + SLIDER_DESIRED_WIDTH + gap, trackAlignY + (7.0f - btnI->GetHeight()) / 2.0f);
			};

		SetVolumeRowPosition(trackMaster, trackMasterFill, btnMasterDec, btnMasterInc, l.masterY);
		SetVolumeRowPosition(trackMusic, trackMusicFill, btnMusicDec, btnMusicInc, l.musicY);
		SetVolumeRowPosition(trackSFX, trackSFXFill, btnSFXDec, btnSFXInc, l.sfxY);

		backButton->SetLocalPosition(-screenW / 2.0f + 32.f, -screenH / 2.0f + 32.f);

		const float btnOffsetY = ROW_CONTROL_OFFSET_Y;

		btnWindowedCheck->SetLocalPosition(CONTROL_COLUMN_X, l.displayY + btnOffsetY);
		btnFullscreenCheck->SetLocalPosition(CONTROL_COLUMN_X + OPTION_SECOND_X, l.displayY + btnOffsetY);

		btnResPrev->SetLocalPosition(CONTROL_COLUMN_X, l.resolutionY + btnOffsetY);
		btnResNext->SetLocalPosition(CONTROL_COLUMN_X + RES_NEXT_X, l.resolutionY + btnOffsetY);

		btnRendPrev->SetLocalPosition(CONTROL_COLUMN_X, l.rendererY + btnOffsetY);
		btnRendNext->SetLocalPosition(CONTROL_COLUMN_X + RES_NEXT_X, l.rendererY + btnOffsetY);

		btnVSyncOn->SetLocalPosition(CONTROL_COLUMN_X, l.vsyncY + btnOffsetY);
		btnVSyncOff->SetLocalPosition(CONTROL_COLUMN_X + OPTION_SECOND_X, l.vsyncY + btnOffsetY);

		btnLangEN->SetLocalPosition(CONTROL_COLUMN_X, l.languageY + btnOffsetY);
		btnLangVI->SetLocalPosition(CONTROL_COLUMN_X + OPTION_SECOND_X, l.languageY + btnOffsetY);

		btnMoveSprint->SetLocalPosition(CONTROL_COLUMN_X, l.moveSprintY + btnOffsetY);
		btnMoveSlow->SetLocalPosition(CONTROL_COLUMN_X, l.moveSlowY + btnOffsetY);

		for (size_t i = 0; i < keybindRows.size(); ++i) {
			if (keybindRows[i].button) {
				keybindRows[i].button->SetLocalPosition(KEYBIND_BUTTON_X, l.keybindStartY + KEYBIND_ROW_H * i);
			}
		}
	}

	void SettingsScene::Init()
	{
		transformManager = std::make_shared<sdlgf::TransformManager>();
		auto [camW, camH] = camera.GetScreenResolution();
		lastScreenWidth = camW;
		lastScreenHeight = camH;

		//Load assets
		font = game->GetGraphicsDevice()->GetAssets().GetFont(Demo::kMainFontName, Demo::kMainFontSize);
		fontSprite = std::make_shared<sdlgf::FontSprite>(font);
		fontSprite->SetColor(0xFF000000);

		placeholderTex = game->GetGraphicsDevice()->GetAssets().GetTexture(L"assets/ui-pack.png");
		uiSheetTex = game->GetGraphicsDevice()->GetAssets().GetTexture(L"assets/ui.png");
		bgTex = game->GetGraphicsDevice()->GetAssets().GetTexture(L"assets/test-background.png");

		//LOCAL FUNCTION to init track and trackfill
		auto InitTrack = [&](std::shared_ptr<sdlgf::NineSliceSprite>& track, std::shared_ptr<sdlgf::NineSliceSprite>& fill, RECT trackR, RECT fillR) {
			track = std::make_shared<sdlgf::NineSliceSprite>(placeholderTex, trackR, 4, 2, 4, 2);
			fill = std::make_shared<sdlgf::NineSliceSprite>(placeholderTex, fillR, 4, 2, 4, 2);
			};

		InitTrack(trackMaster, trackMasterFill, { 113, 483, 160, 490 }, { 65, 483, 112, 490 });
		InitTrack(trackMusic, trackMusicFill, { 113, 499, 160, 506 }, { 65, 499, 112, 506 });
		InitTrack(trackSFX, trackSFXFill, { 113, 515, 160, 522 }, { 65, 515, 112, 522 });

		//LOCAL FUNCTION to init decs,inc buttons
		auto CreateVolBtn = [&](int srcX, int srcY, const std::function<void(sdlgf::ITrigger*)>& action) {
			auto btn = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 12, 11, uiSheetTex);
			btn->SetSpriteCoords(srcX, srcY, 16, 16, 0);

			btn->SetOnReleaseLeft([action](sdlgf::ITrigger* t)
				{
					action(t);
					SettingsManager::GetInstance()->SaveSettings();
				});
			return btn;
			};

		auto sm = SettingsManager::GetInstance();
		btnMasterDec = CreateVolBtn(240, 96, [](sdlgf::ITrigger*) { SettingsManager::GetInstance()->SetMasterVolume(std::max(0.0f, SettingsManager::GetInstance()->GetMasterVolume() - 0.2f)); });
		btnMasterInc = CreateVolBtn(240, 112, [](sdlgf::ITrigger*) { SettingsManager::GetInstance()->SetMasterVolume(std::min(1.0f, SettingsManager::GetInstance()->GetMasterVolume() + 0.2f)); });

		btnMusicDec = CreateVolBtn(240, 96, [](sdlgf::ITrigger*) { SettingsManager::GetInstance()->SetMusicVolume(std::max(0.0f, SettingsManager::GetInstance()->GetMusicVolume() - 0.2f)); });
		btnMusicInc = CreateVolBtn(240, 112, [](sdlgf::ITrigger*) { SettingsManager::GetInstance()->SetMusicVolume(std::min(1.0f, SettingsManager::GetInstance()->GetMusicVolume() + 0.2f)); });

		btnSFXDec = CreateVolBtn(240, 96, [](sdlgf::ITrigger*) { SettingsManager::GetInstance()->SetSfxVolume(std::max(0.0f, SettingsManager::GetInstance()->GetSfxVolume() - 0.2f)); });
		btnSFXInc = CreateVolBtn(240, 112, [](sdlgf::ITrigger*) { SettingsManager::GetInstance()->SetSfxVolume(std::min(1.0f, SettingsManager::GetInstance()->GetSfxVolume() + 0.2f)); });

		backButton = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 96, 32, uiSheetTex, 3);
		backButton->SetSpriteRects(sdlgf::Utils::CreateRectsVertical(96, 48, 48, 16, 3));
		backButton->SetOnReleaseLeft([this](sdlgf::ITrigger*) { this->isGoingBack = true; });
		backButton->SetSpriteScale(2.f, 2.f);

		// Init 2 Checkboxes
		btnWindowedCheck = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnWindowedCheck->SetSpriteCoords(sm->GetFullscreen() ? 256 : 272, 352, 16, 16, 0, true);
		btnWindowedCheck->SetSpriteScale(2.f, 2.f);
		btnWindowedCheck->SetOnReleaseLeft([](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			if (sm->GetFullscreen()) {
				sm->SetFullscreen(false);
				sm->SaveSettings();
				sm->ApplyResolution();
			}
			});

		btnFullscreenCheck = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnFullscreenCheck->SetSpriteCoords(sm->GetFullscreen() ? 272 : 256, 352, 16, 16, 0, true);
		btnFullscreenCheck->SetSpriteScale(2.f, 2.f);
		btnFullscreenCheck->SetOnReleaseLeft([](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			if (!sm->GetFullscreen()) {
				sm->SetFullscreen(true);
				sm->SaveSettings();
				sm->ApplyResolution();
			}
			});

		// Init 2 Language checkboxes (English / Tieng Viet)
		btnLangEN = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnLangEN->SetSpriteCoords(sm->GetLanguage() == Language::EN ? 272 : 256, 352, 16, 16, 0, true);
		btnLangEN->SetSpriteScale(2.f, 2.f);
		btnLangEN->SetOnReleaseLeft([](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			if (sm->GetLanguage() != Language::EN) {
				sm->SetLanguage(Language::EN);
				sm->SaveSettings();
			}
			});

		btnLangVI = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnLangVI->SetSpriteCoords(sm->GetLanguage() == Language::VI ? 272 : 256, 352, 16, 16, 0, true);
		btnLangVI->SetSpriteScale(2.f, 2.f);
		btnLangVI->SetOnReleaseLeft([](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			if (sm->GetLanguage() != Language::VI) {
				sm->SetLanguage(Language::VI);
				sm->SaveSettings();
			}
			});

		// Init 2 Movement Style checkboxes (Shift to sprint / Shift to slow)
		btnMoveSprint = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnMoveSprint->SetSpriteCoords(sm->GetMovementStyle() == MovementStyle::ShiftToSprint ? 272 : 256, 352, 16, 16, 0, true);
		btnMoveSprint->SetSpriteScale(2.f, 2.f);
		btnMoveSprint->SetOnReleaseLeft([](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			if (sm->GetMovementStyle() != MovementStyle::ShiftToSprint) {
				sm->SetMovementStyle(MovementStyle::ShiftToSprint);
				sm->SaveSettings();
			}
			});

		btnMoveSlow = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnMoveSlow->SetSpriteCoords(sm->GetMovementStyle() == MovementStyle::ShiftToSlow ? 272 : 256, 352, 16, 16, 0, true);
		btnMoveSlow->SetSpriteScale(2.f, 2.f);
		btnMoveSlow->SetOnReleaseLeft([](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			if (sm->GetMovementStyle() != MovementStyle::ShiftToSlow) {
				sm->SetMovementStyle(MovementStyle::ShiftToSlow);
				sm->SaveSettings();
			}
			});

		btnResPrev = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnResPrev->SetSpriteCoords(240, 96, 16, 16, 0);
		btnResPrev->SetSpriteScale(2.f, 2.f);
		btnResPrev->SetSpriteOrigin(8.f, 8.f); //center of sprite
		btnResPrev->SetSpriteRotation(1.5708f);
		btnResPrev->SetSpriteOffset(16.f, 16.f);
		btnResPrev->SetOnReleaseLeft([this](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			int idx = sm->GetCurrentResolutionIndex();
			if (idx > 0) {
				sm->SetResolutionIndex(idx - 1);
				sm->SaveSettings();
				sm->ApplyResolution();
			}
			});
		btnResPrev->Init(&uiCamera);

		btnResNext = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
		btnResNext->SetSpriteCoords(240, 112, 16, 16, 0);
		btnResNext->SetSpriteScale(2.f, 2.f);
		btnResNext->SetSpriteOrigin(8.f, 8.f);
		btnResNext->SetSpriteRotation(1.5708f);
		btnResNext->SetSpriteOffset(16.f, 16.f);
		btnResNext->SetOnReleaseLeft([this](sdlgf::ITrigger*) {
			auto sm = SettingsManager::GetInstance();
			int idx = sm->GetCurrentResolutionIndex();
			if (idx < sm->GetSupportedResolutions().size() - 1) {
				sm->SetResolutionIndex(idx + 1);
				sm->SaveSettings();
				sm->ApplyResolution();
			}
			});
		btnResNext->Init(&uiCamera);

		// Renderer backend selector (arrows like Resolution). The driver is chosen when the renderer
		// is created, so a change is saved now and applies on the next launch.
		backendOptions.push_back("");
		for (auto& name : sdlgf::GraphicsDevice::GetRenderDrivers()) backendOptions.push_back(name);

		auto MakeArrow = [&](int srcY, const std::function<void()>& action) {
			auto btn = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
			btn->SetSpriteCoords(240, srcY, 16, 16, 0);
			btn->SetSpriteScale(2.f, 2.f);
			btn->SetSpriteOrigin(8.f, 8.f);
			btn->SetSpriteRotation(1.5708f);
			btn->SetSpriteOffset(16.f, 16.f);
			btn->SetOnReleaseLeft([action](sdlgf::ITrigger*) { action(); });
			btn->Init(&uiCamera);
			return btn;
			};
		btnRendPrev = MakeArrow(96, [this]() { SetBackendIndex(CurrentBackendIndex() - 1); });
		btnRendNext = MakeArrow(112, [this]() { SetBackendIndex(CurrentBackendIndex() + 1); });

		// V-Sync checkboxes (On / Off), applied to the renderer immediately
		auto MakeVSyncCheck = [&](bool value) {
			auto btn = std::make_shared<Demo::IconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, 3);
			btn->SetSpriteCoords(sm->GetVSync() == value ? 272 : 256, 352, 16, 16, 0, true);
			btn->SetSpriteScale(2.f, 2.f);
			btn->SetOnReleaseLeft([this, value](sdlgf::ITrigger*) {
				auto sm = SettingsManager::GetInstance();
				if (sm->GetVSync() != value) {
					sm->SetVSync(value);
					sm->SaveSettings();
					game->GetGraphicsDevice()->SetVSync(value);
				}
				});
			return btn;
			};
		btnVSyncOn = MakeVSyncCheck(true);
		btnVSyncOff = MakeVSyncCheck(false);

		auto SetupKeybindBtn = [&](const std::string& action, const wchar_t* label) {
			const size_t rowIndex = keybindRows.size();
			keybindRows.push_back({ action, label, nullptr, false });
			auto& btn = keybindRows.back().button;
			btn = std::make_shared<Demo::TextIconButton>(transformManager, 0, 0, 32, 32, uiSheetTex, font, L"", 3);
			btn->SetSpriteRects(sdlgf::Utils::CreateRectsVertical(0, 0, 16, 16, 3));
			btn->SetSpriteScale(2.f, 2.f);

			//long key names ("Shift", "Backspace") widen the button instead of spilling out of it
			btn->SetSliceMargins(4, 4);
			btn->SetAutoResize(true, 10.f);

			btn->SetTextScale(1.0f, 1.0f);
			btn->SetTextColor(COLOR_XRGB(0, 0, 0));

			Demo::TextIconButton* rawBtn = btn.get();

			btn->SetDynamicTextGetter([rawBtn, sm, action]() {
				if (rawBtn->GetState() == Demo::IButton::ButtonState::LISTENING) {
					return std::wstring(L"...");
				}
				return ToWString(GetKeyName(sm->GetKeybind(action)));
				});

			btn->SetOnReleaseLeft([this, rawBtn, rowIndex](sdlgf::ITrigger*) {
				this->ResetListening();
				keybindRows[rowIndex].listening = true;
				rawBtn->SetState(Demo::IButton::ButtonState::LISTENING);
				});
			};

		SetupKeybindBtn("MOVE_UP", L"Move up");
		SetupKeybindBtn("MOVE_DOWN", L"Move down");
		SetupKeybindBtn("MOVE_LEFT", L"Move left");
		SetupKeybindBtn("MOVE_RIGHT", L"Move right");
		SetupKeybindBtn("ACCEPT", L"Accept");
		SetupKeybindBtn("OPEN_INVENTORY", L"Open inventory");
		SetupKeybindBtn("INTERACT", L"Interact");
		SetupKeybindBtn("SPRINT", L"Sprint");
		SetupKeybindBtn("TOGGLE_GEAR", L"Toggle Gear");
		SetupKeybindBtn("OPEN_MAP", L"Open map");

		// Active Buttons
		std::vector<std::shared_ptr<Demo::IButton>> buttons = { backButton, btnMasterDec, btnMasterInc, btnMusicDec, btnMusicInc, btnSFXDec, btnSFXInc, btnWindowedCheck, btnFullscreenCheck, btnVSyncOn, btnVSyncOff, btnLangEN, btnLangVI, btnMoveSprint, btnMoveSlow };
		for (auto& row : keybindRows) buttons.push_back(row.button);
		for (auto& btn : buttons)
		{
			if (btn)
			{
				btn->Init(&uiCamera);
				uiButtons.push_back(btn);
			}
		}

		UpdateLayout(lastScreenWidth, lastScreenHeight);
		transformManager->RebuildHierarchy();
	}

	void SettingsScene::Update(unsigned long long deltaTime)
	{
		auto inpMan = sdlgf::InputManager::GetInstance();
		inpMan->ReadMouse(deltaTime);
		inpMan->ReadKeyboard(deltaTime);

		auto sm = SettingsManager::GetInstance();

		//sprite coords checkbox base on state
		if (sm->GetFullscreen()) {
			btnWindowedCheck->SetSpriteCoords(256, 352, 16, 16, 0, true);
			btnFullscreenCheck->SetSpriteCoords(272, 352, 16, 16, 0, true);
		}
		else {
			btnWindowedCheck->SetSpriteCoords(272, 352, 16, 16, 0, true);
			btnFullscreenCheck->SetSpriteCoords(256, 352, 16, 16, 0, true);
		}

		if (sm->GetLanguage() == Language::VI) {
			btnLangEN->SetSpriteCoords(256, 352, 16, 16, 0, true);
			btnLangVI->SetSpriteCoords(272, 352, 16, 16, 0, true);
		}
		else {
			btnLangEN->SetSpriteCoords(272, 352, 16, 16, 0, true);
			btnLangVI->SetSpriteCoords(256, 352, 16, 16, 0, true);
		}

		btnVSyncOn->SetSpriteCoords(sm->GetVSync() ? 272 : 256, 352, 16, 16, 0, true);
		btnVSyncOff->SetSpriteCoords(sm->GetVSync() ? 256 : 272, 352, 16, 16, 0, true);

		const bool slowStyle = sm->GetMovementStyle() == MovementStyle::ShiftToSlow;
		btnMoveSprint->SetSpriteCoords(slowStyle ? 256 : 272, 352, 16, 16, 0, true);
		btnMoveSlow->SetSpriteCoords(slowStyle ? 272 : 256, 352, 16, 16, 0, true);

		for (auto& button : uiButtons) button->Update(deltaTime);

		if (!sm->GetFullscreen()) {
			int resIdx = sm->GetCurrentResolutionIndex();
			int maxIdx = sm->GetSupportedResolutions().size() - 1;
			if (resIdx > 0) btnResPrev->Update(deltaTime);
			if (resIdx < maxIdx) btnResNext->Update(deltaTime);
		}
		const int backendIdx = CurrentBackendIndex();
		if (backendIdx > 0) btnRendPrev->Update(deltaTime);
		if (backendIdx < static_cast<int>(backendOptions.size()) - 1) btnRendNext->Update(deltaTime);

		transformManager->UpdateAll();
		camera.Update();

		// While a rebind capture is in progress every key belongs to the capture -
		// suspend navigation so arrows don't move the cursor mid-listen.
		if (!IsAnyKeybindListening()) {
			keyboardNavigator.Update(deltaTime, CollectKeyboardCandidates());
		}

		//LOCAL FUNCTION to handle key presses
		auto HandleKeybind = [&](bool& isListeningFlag, const std::string& actionName, std::shared_ptr<IconButton> btn)
			{
				if (!isListeningFlag) return;

				for (int i = 1; i < 256; i++)
				{
					if (i == DIK_ESCAPE) continue;

					// Edge-triggered: a key already held when listening began (e.g. the Accept
					// key that activated this button via keyboard navigation) must not be captured.
					if (inpMan->KeyDown(i))
					{
						SettingsManager::GetInstance()->SetKeybind(actionName, i);
						btn->SetState(IButton::ButtonState::IDLE);
						isListeningFlag = false;
						SettingsManager::GetInstance()->SaveSettings();

						inpMan->ConsumeKey(i);
						break;
					}
				}
			};

		for (auto& row : keybindRows) {
			HandleKeybind(row.listening, row.action, row.button);
		}

		if (this->isGoingBack)
		{
			auto sm = this->game->GetSceneManager();
			sm->RemoveScene(sm->GetIndex()); //remove self from scene manager
			sm->GoToPrevious();
			return;
		}
	}

	void SettingsScene::DrawWorld(unsigned long long deltaTime)
	{
		auto gd = game->GetGraphicsDevice();
		if (gd->BeginDraw()) {
			DrawBackground(deltaTime);
			gd->EndDraw();
		}
	}

	void SettingsScene::DrawUI(unsigned long long deltaTime)
	{
		auto gd = game->GetGraphicsDevice();
		if (gd->BeginDraw()) {

			gd->SetAlphaBlending(true);
			gd->DrawRectangle(uiCamera, -lastScreenWidth / 2.f, -lastScreenHeight / 2.f, lastScreenWidth, lastScreenHeight, COLOR_ARGB(200, 0, 0, 0), true);
			gd->SetAlphaBlending(false);

			const Layout l = ComputeLayout(lastScreenHeight);
			auto sm = SettingsManager::GetInstance();
			const sdlgf::Color white = 0xFFFFFFFF;

			//Draw text at the same x as the label/option column, via the shared font sprite
			auto drawText = [&](const std::wstring& text, float x, float y, sdlgf::Color color) { DrawString(text, x, y, color); };

			//Audio
			DrawHeading(Tr(L"Audio"), PANEL_LEFT_X, l.audioHeadingY, PANEL_LEFT_W);
			drawText(Tr(L"Master Volume"), LABEL_COLUMN_X, l.masterY, white);
			drawText(Tr(L"Music Volume"), LABEL_COLUMN_X, l.musicY, white);
			drawText(Tr(L"Sfx Volume"), LABEL_COLUMN_X, l.sfxY, white);

			//Video
			DrawHeading(Tr(L"Video"), PANEL_LEFT_X, l.videoHeadingY, PANEL_LEFT_W);
			drawText(Tr(L"Display Mode"), LABEL_COLUMN_X, l.displayY, white);
			drawText(Tr(L"Windowed"), CONTROL_COLUMN_X + OPTION_TEXT_GAP, l.displayY, white);
			drawText(Tr(L"Fullscreen"), CONTROL_COLUMN_X + OPTION_SECOND_X + OPTION_TEXT_GAP, l.displayY, white);

			sdlgf::Color resColor = sm->GetFullscreen() ? 0xFF888888 : 0xFFFFFFFF;
			drawText(Tr(L"Resolution"), LABEL_COLUMN_X, l.resolutionY, resColor);

			int resIdx = sm->GetCurrentResolutionIndex();
			auto resList = sm->GetSupportedResolutions();
			drawText(ToWString(resList[resIdx].label), CONTROL_COLUMN_X + RES_LABEL_X, l.resolutionY, resColor);

			if (!sm->GetFullscreen()) {
				if (resIdx > 0) btnResPrev->Draw(gd, deltaTime);
				if (resIdx < resList.size() - 1) btnResNext->Draw(gd, deltaTime);
			}

			//Renderer backend (applies on the next launch) and V-Sync (applies immediately)
			drawText(Tr(L"Renderer"), LABEL_COLUMN_X, l.rendererY, white);
			const int backendIdx = CurrentBackendIndex();
			const bool restartPending = sm->IsRendererRestartPending();
			std::wstring backendText = backendIdx == 0
				? Tr(L"Auto") + L" (" + ToWString(gd->GetRendererName()) + L")"
				: ToWString(backendOptions[backendIdx]);
			if (restartPending) backendText += L" *";
			drawText(backendText, CONTROL_COLUMN_X + RES_LABEL_X, l.rendererY, restartPending ? 0xFFFFD34D : white);
			if (backendIdx > 0) btnRendPrev->Draw(gd, deltaTime);
			if (backendIdx < static_cast<int>(backendOptions.size()) - 1) btnRendNext->Draw(gd, deltaTime);

			drawText(Tr(L"V-Sync"), LABEL_COLUMN_X, l.vsyncY, white);
			drawText(Tr(L"On"), CONTROL_COLUMN_X + OPTION_TEXT_GAP, l.vsyncY, white);
			drawText(Tr(L"Off"), CONTROL_COLUMN_X + OPTION_SECOND_X + OPTION_TEXT_GAP, l.vsyncY, white);

			//General
			DrawHeading(Tr(L"General"), PANEL_LEFT_X, l.generalHeadingY, PANEL_LEFT_W);
			drawText(Tr(L"Language"), LABEL_COLUMN_X, l.languageY, white);
			// Language names are shown as endonyms, not translated.
			drawText(L"English", CONTROL_COLUMN_X + OPTION_TEXT_GAP, l.languageY, white);
			drawText(L"Tiếng Việt", CONTROL_COLUMN_X + OPTION_SECOND_X + OPTION_TEXT_GAP, l.languageY, white);

			drawText(Tr(L"Movement Style"), LABEL_COLUMN_X, l.moveSprintY, white);
			drawText(Tr(L"Hold to sprint"), CONTROL_COLUMN_X + OPTION_TEXT_GAP, l.moveSprintY, white);
			drawText(Tr(L"Hold to slow"), CONTROL_COLUMN_X + OPTION_TEXT_GAP, l.moveSlowY, white);

			if (restartPending) drawText(L"* " + Tr(L"Restart required to change renderer"), LABEL_COLUMN_X, l.footnoteY, 0xFFFFD34D);

			//Controls
			DrawHeading(Tr(L"Controls"), PANEL_RIGHT_X, l.controlsHeadingY, PANEL_RIGHT_W);
			for (size_t i = 0; i < keybindRows.size(); ++i) {
				const wchar_t* label = keybindRows[i].label;
				if (keybindRows[i].action == "SPRINT" && sm->GetMovementStyle() == MovementStyle::ShiftToSlow) label = L"Focus";
				drawText(Tr(label), KEYBIND_LABEL_X, l.keybindStartY + KEYBIND_ROW_H * i, white);
			}

			//draw tracks
			DrawVolumeTrack(trackMaster, trackMasterFill, sm->GetMasterVolume(), { 65, 483, 112, 490 }, deltaTime);
			DrawVolumeTrack(trackMusic, trackMusicFill, sm->GetMusicVolume(), { 65, 499, 112, 506 }, deltaTime);
			DrawVolumeTrack(trackSFX, trackSFXFill, sm->GetSfxVolume(), { 65, 515, 112, 522 }, deltaTime);

			//draw buttons
			for (auto& btn : uiButtons) btn->Draw(gd, deltaTime);

			keyboardNavigator.Draw(gd, uiCamera, CollectKeyboardCandidates());
			if (!keyboardNavigator.IsInKeyboardMode()) {
				sdlgf::InputManager::GetInstance()->DrawCursor(&this->uiCamera, deltaTime);
			}
			gd->EndDraw();
		}
	}

}