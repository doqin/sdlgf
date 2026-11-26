#pragma once
#include <Extras\ICommand.h>
#include <Sprites.h>
#include <GraphicsDevice.h>
#include <Camera.h>
#include <queue>
#include "DialogueLine.h"

namespace Demo {
	class IConversation : public sdlgf::ICommand {
	private:
		std::queue<DialogueLine> dialogueQueue;
		std::shared_ptr<sdlgf::FontSprite> fontSprite;
		float virtualWidth;
		float virtualHeight;
		std::wstring displayedContent;
		size_t currentCharIndex = 0;
		unsigned long long timer = 0;
		const unsigned long long MS_PER_CHAR = 30;
		bool isTyping = false;
		// currentLine.content with some spaces swapped for '\n' so it fits the box. Same length
		// as the source, so the typewriter's character index maps straight onto it.
		std::wstring wrappedSource;
		std::wstring wrappedContent;
		void ResetAnimation();
		std::wstring WrapToWidth(const std::wstring& text, float maxWidth);
	public:
		IConversation(std::shared_ptr<sdlgf::FontSprite> fontSprite, int screenWidth, int screenHeight);

		void AddLine(const DialogueLine& line);

		void Execute(unsigned long long deltaTime) override;

		void Draw(sdlgf::GraphicsDevice* gd, sdlgf::Camera* uiCamera, unsigned long long deltaTime);
	};
}

