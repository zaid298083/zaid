#pragma once

#include <list>
#include <chrono>
#include <vector>
#include <cstdint>

class Chat : public ListBox
{
public:
	Chat();

	void addChatMessage(const std::string& message, const std::string& nick, const ImColor& nick_color);
	void addInfoMessage(const std::string& format, ...);
	void addDebugMessage(const std::string& format, ...);
	void addClientMessage(const std::string& message, const ImColor& color);

	// Abre o teclado pelo botao de chat e mostra o campo flutuante “Digite...”.
	void openInput();

	// Exibe somente o texto acima da cabeca do jogador indicado, sem nick.
	void addBubbleMessage(uint16_t playerId, const std::string& message);

	virtual void draw(ImGuiRenderer* renderer) override;
	// O chat antigo não possui mais área de toque; a câmera recebe os gestos nessa região.
	virtual void touchEvent(const ImVec2& pos, TouchType type) override;
	virtual void activateEvent(bool active) override;
	virtual void touchPopEvent() override;
	virtual void keyboardEvent(const std::string& input) override;
	virtual void keyboardCancelEvent() override;

private:
	void addMessage(const std::string& messsage, const ImColor& color = ImColor(1.0f, 1.0f, 1.0f));
	void addPlayerMessage(const std::string& message, const std::string& nick, const ImColor& nick_color);
	void keepVisible();
	void drawPlayerBubbles(ImGuiRenderer* renderer);

	struct PlayerBubble
	{
		uint16_t playerId;
		std::string message;
		std::chrono::steady_clock::time_point expiresAt;
	};

	bool m_inputOpen = false;
	std::chrono::steady_clock::time_point m_displayUntil{};
	std::string m_lastMessage;
	std::vector<PlayerBubble> m_playerBubbles;

	class MessageItem : public ListBoxItem
	{
	public:
		MessageItem(const std::string& message, const ImColor& color)
		{
			m_text = new Label(message, color, true, UISettings::fontSize() / 2);
			this->addChild(m_text);
		}

		void performLayout() override
		{
			m_text->performLayout();
			m_text->setPosition(ImVec2(0.0f, (height() - m_text->height()) / 2));
		}

	private:
		Label* m_text;
	};

	class PlayerMessageItem : public ListBoxItem
	{
	public:
		PlayerMessageItem(const std::string& message, const std::string& nick, const ImColor& nick_color)
		{
			m_nick = new Label(nick, nick_color, true, UISettings::fontSize() / 2);
			m_text = new Label(": " + message, ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
			this->addChild(m_nick);
			this->addChild(m_text);
		}

		void performLayout() override
		{
			m_nick->performLayout();
			m_nick->setPosition(ImVec2(0.0f, (height() - m_nick->height()) / 2));

			m_text->performLayout();
			m_text->setPosition(ImVec2(m_nick->position().x + m_nick->width(), (height() - m_text->height()) / 2));
		}

	private:
		Label* m_nick;
		Label* m_text;
	};
};