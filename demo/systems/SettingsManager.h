#pragma once
#include "sdlgf.h"

namespace Demo
{
	struct Resolution {
		int width;
		int height;
		std::string label;
	};

	enum class Language {
		EN,
		VI
	};

	enum class MovementStyle {
		ShiftToSprint,
		ShiftToSlow
	};

	class SettingsManager
	{
	private:
		static SettingsManager* instance;
		SettingsManager();

		float masterVolume;
		float musicVolume;
		float sfxVolume;

		std::map<std::string, int> keybinds;

		std::string configFilePath;
		bool isFullscreen;

		std::vector<Resolution> supportedResolutions;
		int currentResIndex;

		std::string rendererBackend;   // SDL render driver name; empty = Auto
		std::string launchedRendererBackend;  // what the running renderer was created with
		bool vsync = true;

		Language currentLanguage;
		MovementStyle movementStyle = MovementStyle::ShiftToSprint;
	public:
		static SettingsManager* GetInstance()
		{
			if (!instance)
			{
				instance = new SettingsManager();
				instance->LoadSettings();
			}
			return instance;
		}

		void SetDefaultSettings();

		bool LoadSettings();
		bool SaveSettings();

		void SetMusicVolume(float vol);
		float GetMusicVolume() { return musicVolume; }

		void SetMasterVolume(float vol);
		float GetMasterVolume() { return masterVolume; }

		void SetSfxVolume(float vol);
		float GetSfxVolume() { return sfxVolume; }

		void SetKeybind(std::string actionName, int keyCode);
		int GetKeybind(std::string actionName);

		void SetFullscreen(bool fs);
		bool GetFullscreen() { return isFullscreen; }

		const std::vector<Resolution>& GetSupportedResolutions() const { return supportedResolutions; }
		int GetCurrentResolutionIndex() const { return currentResIndex; }
		void SetResolutionIndex(int index);

		std::wstring GetKeybindDisplayName(std::string actionName);

		// Helper function to apply immediately
		void ApplyResolution();

		// Render driver ("" = Auto). Only read when the renderer is created, so a change applies on the next launch.
		void SetRendererBackend(const std::string& name) { this->rendererBackend = name; }
		const std::string& GetRendererBackend() const { return this->rendererBackend; }

		void SetLaunchedRendererBackend(const std::string& name) { this->launchedRendererBackend = name; }
		// True when the saved backend differs from the one the running renderer uses.
		bool IsRendererRestartPending() const { return rendererBackend != launchedRendererBackend; }

		// Stored only; the caller pushes it to the GraphicsDevice so it applies immediately.
		void SetVSync(bool enabled) { this->vsync = enabled; }
		bool GetVSync() const { return this->vsync; }

		void SetLanguage(Language lang) { this->currentLanguage = lang; }
		Language GetLanguage() const { return this->currentLanguage; }

		void SetMovementStyle(MovementStyle style) { this->movementStyle = style; }
		MovementStyle GetMovementStyle() const { return this->movementStyle; }
	};
}
