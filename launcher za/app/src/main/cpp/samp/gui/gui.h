#pragma once

#define DEBUG_GUI 0

#include "../vendor/encoding/encoding.h"

#include "imguiwrapper.h"
#include "uisettings.h"

#include "widget.h"
#include "widgets/layout.h"
#include "widgets/label.h"
#include "widgets/button.h"
#include "widgets/image.h"
#include "widgets/progressbar.h"
#include "widgets/scrollpanel.h"
#include "widgets/listbox.h"
#include "widgets/editbox.h"

#include "samp_widgets/keyboard.h"
#include "samp_widgets/splashscreen.h"
#include "samp_widgets/chat.h"
#include "samp_widgets/spawn.h"
#include "samp_widgets/buttonpanel.h"
#include "samp_widgets/playertablist.h"
#include "samp_widgets/voicebutton.h"
#include "samp_widgets/nexionsettingsmenu.h"

#include "samp_widgets/dialogs/dialog.h"

#include "..//vendor/raknet/SingleProducerConsumer.h"

#pragma pack(push, 1)
struct BUFFERED_COMMAND_TEXTDRAW
{
    uint16_t textdrawId;
};
#pragma pack(pop)

class UI : public Widget, public ImGuiWrapper
{
public:
	UI(const ImVec2& display_size, const std::string& font_path);

	bool initialize() override;
	void render() override;
	void shutdown() override;


	SplashScreen* splashscreen() const { return m_splashScreen; }
	Chat* chat() const { return m_chat; }
	Spawn* spawn() const { return m_spawn; }
	Keyboard* keyboard() const { return m_keyboard; }
	Dialog* dialog() const { return m_dialog; }
	ButtonPanel* buttonpanel() const { return m_buttonPanel; }
	PlayerTabList* playertablist() const { return m_playerTabList; }
	VoiceButton* voicebutton() const { return m_voiceButton; }
	NexionSettingsMenu* settingsmenu() const { return m_settingsMenu; }

	static const ImColor fixcolor(uint32_t color) {
		return ImColor(
			(int)((color & 0xFF000000) >> 24),
			(int)((color & 0x00FF0000) >> 16),
			(int)((color & 0x0000FF00) >> 8));
	}

	float ScaleX(float x) {
		return x * displaySize().x * (1.0f / 1920.0f);
	}

	float ScaleY(float y) {
		return y * displaySize().y * (1.0f / 1080.0f);
	}

	float GetFontSize() {
		return UISettings::fontSize();
	}

	virtual void touchEvent(const ImVec2& pos, TouchType type) override;

	bool OnTouchEvent(int type, bool multi, int x, int y);

    // Tela modal de "CONEXÃO PERDIDA" exibida quando a conexão cai.
    void ShowConnectionLost();
    void HideConnectionLost();
    bool IsConnectionLost() const { return m_bConnectionLost; }

    void renderDebug();
    void SetEngine(int engine);
    int bEngine;
    void SetLights(int lights);
    int bLights;
    void SetDoor(int door);
    int bDoor;
    void SetMeliage(float meliage);
    int bMeliage = 0;
    void SetFuel(float fuel);
    uint8_t m_fuel;
    // Valores usados pelo HUD vertical; o servidor pode atualiza-los por RPC.
    void SetHunger(float hunger) { m_hunger = (uint8_t)hunger; }
    void SetThirst(float thirst) { m_thirst = (uint8_t)thirst; }
    uint8_t m_hunger = 100;
    uint8_t m_thirst = 100;
    void SetHealth(float fHealth);
    int GetHealth();
    int bHealth;

    void setKeyboardOffset(int offset) { m_keyboardOffsetY = offset; }
    int getKeyboardOffset() const { return m_keyboardOffsetY; }
    int m_keyboardOffsetY = 0;

    void ProcessPushedTextdraws();
    void PushToBufferedQueueTextDrawPressed(uint16_t textdrawId);

protected:
	void drawList() override;

private:
    void DrawConnectionLost();
    void RestartConnectionAfterLoss();

	Keyboard* m_keyboard;
	SplashScreen* m_splashScreen;
	Chat* m_chat;
	Spawn* m_spawn;
	Dialog* m_dialog;
	ButtonPanel* m_buttonPanel;
	PlayerTabList* m_playerTabList;
	VoiceButton* m_voiceButton;
	NexionSettingsMenu* m_settingsMenu = nullptr;

    bool m_bConnectionLost = false;
    ImVec2 m_connLostBtnMin = ImVec2(0, 0);
    ImVec2 m_connLostBtnMax = ImVec2(0, 0);
	Label* label;
    // Otimizacao: labels 2-8 removidos — nao eram usados e consumiam traversal por frame.

    float 		m_fFontSize;

	bool m_bNeedClearMousePos = false;

    // Botão 3D "Acessar" — bounds calculados no render
    ImVec2 m_bankBtnMin = ImVec2(0, 0);
    ImVec2 m_bankBtnMax = ImVec2(0, 0);
    bool   m_bankBtnVisible = false;
    bool   m_bankBtnPressed = false;

public:
	    // Botão CEF/HUD de arma: guarda apenas o tipo da arma atual para
	    // permitir alternar entre arma na mão e mãos vazias sem removê-la.
	    ImVec2 m_fistBtnMin = ImVec2(0, 0);
	    ImVec2 m_fistBtnMax = ImVec2(0, 0);
	    bool   m_fistBtnVisible = false;
	    bool   m_fistBtnPressed = false;
	    uint8_t m_savedWeapon = 0;
	    bool   m_weaponHolstered = false;
public:
    bool IsBankButtonAt(int x, int y) const;

    DataStructures::SingleProducerConsumer<BUFFERED_COMMAND_TEXTDRAW> m_BufferedCommandTextdraws;
};
