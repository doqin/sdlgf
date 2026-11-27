#pragma once
#include "Game.h"
#include "sdlgf.h"
#include "Extras.h"
#include "IconButton.h"
#include "TextIconButton.h"
#include "KeyboardNavigator.h"
namespace Demo
{
    //UI constants. The scene is laid out in the 960x720 virtual UI space, origin at the centre:
    //  left panel  = Audio / Video / General settings, right panel = Controls (keybinds).
    constexpr float PANEL_LEFT_X = -456.0f;   // label column of the left panel
    constexpr float PANEL_RIGHT_X = 119.0f;   // label column of the right panel
    constexpr float PANEL_LEFT_W = 551.0f;    // width of the left panel (ends 24px before the right one)
    constexpr float PANEL_RIGHT_W = 337.0f;

    constexpr float LABEL_COLUMN_X = PANEL_LEFT_X;
    constexpr float CONTROL_COLUMN_X = LABEL_COLUMN_X + 215.0f;  // checkboxes / resolution arrows start here
    constexpr float SLIDER_COLUMN_X = CONTROL_COLUMN_X + 22.0f;  // room on the left for the "-" button
    constexpr float SLIDER_DESIRED_WIDTH = 100.0f;
    constexpr float OPTION_SECOND_X = 145.0f;                    // second checkbox of a pair, relative to CONTROL_COLUMN_X
    constexpr float OPTION_TEXT_GAP = 35.0f;                     // checkbox -> its text
    constexpr float RES_LABEL_X = 40.0f;                         // resolution text, relative to CONTROL_COLUMN_X
    constexpr float RES_NEXT_X = 248.0f;                         // "next" arrow, relative to CONTROL_COLUMN_X

    constexpr float KEYBIND_LABEL_X = PANEL_RIGHT_X;
    constexpr float KEYBIND_BUTTON_X = PANEL_RIGHT_X + 190.0f;

    constexpr float HEADING_TOP = 80.0f;     // heading baseline offset from the top of the screen
    constexpr float HEADING_H = 34.0f;       // heading text + rule, before the first row
    constexpr float ROW_H = 38.0f;           // pitch of a settings row (controls are 32px tall)
    constexpr float KEYBIND_ROW_H = 44.0f;
    constexpr float SECTION_GAP = 12.0f;
    constexpr float ROW_CONTROL_OFFSET_Y = -8.0f;  // control top relative to the label's top, centres 32px controls on the text

    class SettingsScene : public sdlgf::IScene
    {
    private:
        Game* game;
        std::shared_ptr<sdlgf::TransformManager> transformManager;

        std::shared_ptr<sdlgf::StaticSprite> bgSprite;
        std::shared_ptr<sdlgf::Texture> bgTex;

        std::shared_ptr<sdlgf::Font> font;
        std::shared_ptr<sdlgf::FontSprite> fontSprite;
        std::shared_ptr<sdlgf::Texture> placeholderTex;
        std::shared_ptr<sdlgf::Texture> uiSheetTex;
        std::vector<std::shared_ptr<Demo::IButton>> uiButtons;
        std::shared_ptr<IconButton> backButton;
        std::shared_ptr<Demo::IconButton> btnResPrev;
        std::shared_ptr<Demo::IconButton> btnResNext;

        // Renderer backend selector: backendOptions[0] is Auto (""), the rest are SDL render drivers
        std::vector<std::string> backendOptions;
        std::shared_ptr<Demo::IconButton> btnRendPrev;
        std::shared_ptr<Demo::IconButton> btnRendNext;
        int CurrentBackendIndex() const;
        void SetBackendIndex(int index);

        // V-Sync Checkboxes
        std::shared_ptr<Demo::IconButton> btnVSyncOn;
        std::shared_ptr<Demo::IconButton> btnVSyncOff;

        // Display Mode Checkboxes
        std::shared_ptr<Demo::IconButton> btnWindowedCheck;
        std::shared_ptr<Demo::IconButton> btnFullscreenCheck;

        // Language Checkboxes
        std::shared_ptr<Demo::IconButton> btnLangEN;
        std::shared_ptr<Demo::IconButton> btnLangVI;

        // Movement Style Checkboxes
        std::shared_ptr<Demo::IconButton> btnMoveSprint;
        std::shared_ptr<Demo::IconButton> btnMoveSlow;

        bool isGoingBack = false;

        int lastScreenWidth;
        int lastScreenHeight;

        //volume UI
        std::shared_ptr<sdlgf::NineSliceSprite> trackMaster, trackMusic, trackSFX;
        std::shared_ptr<sdlgf::NineSliceSprite> trackMasterFill, trackMusicFill, trackSFXFill;

        std::shared_ptr<Demo::IconButton> btnMasterDec, btnMasterInc;
        std::shared_ptr<Demo::IconButton> btnMusicDec, btnMusicInc;
        std::shared_ptr<Demo::IconButton> btnSFXDec, btnSFXInc;

        //keybind UI - one row per rebindable action; add a line in Init() to add a keybind
        struct KeybindRow {
            std::string action;
            const wchar_t* label;
            std::shared_ptr<Demo::TextIconButton> button;
            bool listening = false;
        };
        std::vector<KeybindRow> keybindRows;

        // Y (top of the label text) of every row.
        struct Layout {
            float audioHeadingY, masterY, musicY, sfxY;
            float videoHeadingY, displayY, resolutionY;
            float rendererY, vsyncY;
            float generalHeadingY, languageY, moveSprintY, moveSlowY;
            float footnoteY;
            float controlsHeadingY, keybindStartY;
        };
        Layout ComputeLayout(int screenH) const;
        void DrawHeading(const std::wstring& text, float x, float y, float width);

        KeyboardNavigator keyboardNavigator;
        std::vector<KeyboardNavigator::Candidate> CollectKeyboardCandidates();
        bool IsAnyKeybindListening() const;

        //helper methods
        void DrawBackground(unsigned long long deltaTime);
        void DrawVolumeTrack(std::shared_ptr<sdlgf::NineSliceSprite> bg, std::shared_ptr<sdlgf::NineSliceSprite> fill, float vol, RECT originalRect, unsigned long long deltaTime);

    public:
        SettingsScene(Game* game, int screenWidth, int screenHeight)
            : IScene(screenWidth, screenHeight),
            game(game) {
        }

        void Init() override;
        void Update(unsigned long long deltaTime) override;
        void DrawWorld(unsigned long long deltaTime) override;
        void DrawUI(unsigned long long deltaTime) override;
        void UpdateLayout(int width, int height);
        void DrawString(std::wstring text, float x, float y, sdlgf::Color color, DWORD format = 0 /* ignored: text is always left-aligned at x,y */);
        void ResetListening();
    };
}