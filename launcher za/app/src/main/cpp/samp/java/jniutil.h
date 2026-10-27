#pragma once

#include "main.h"

#define EXCEPTION_CHECK(env) \
	if ((env)->ExceptionCheck()) \
	{ \
		(env)->ExceptionDescribe(); \
		(env)->ExceptionClear(); \
		return; \
	}

class CJavaWrapper
{
public:
    CJavaWrapper(JNIEnv *env, jobject activity);
    ~CJavaWrapper() {};

    const char* GetClipboardString();
    void SetPauseState(bool pause);
    void ShowLoadingScreen();
    void HideLoadingScreen();
    void UpdateLoadingStatus(const char* status);
    void SkipToAuthenticating();
    void StartLoadingSequence();
    void OnGameReady();
    void OnNetworkReady();
    void SetMusicPlaylist(const char* tracks[], int trackCount, const char* title);
    void StopLoadingMusic();
    void ShowTab();
    void HideTab();
    void SetTab(int id, char* names, int score, int pings);
    void ClearTab();

    void ShowKeyboard();
    void HideKeyboard();
    void SetKeyboardOffset(int offsetY);

    // Controla a ImageView da logo Android durante a visualização limpa do mapa.
    void SetNexionHudLogoVisible(bool visible);

    // WebUI: banco NEXION (interface HTML em WebView)
    void ShowBank(const char* jsonData);
    void UpdateBank(const char* jsonData);
    void HideBank();
    void ShowLogin(const char* jsonData);
    void AuthResult(const char* jsonData);
    void HideLogin();

    // Flutter Phone
    void ShowPhone();
    void HidePhone();
    void TogglePhone();

    // Botão flutuante de interação (proximidade do banco)
    void ShowBankInteract(int alpha);
    void HideBankInteract();

    // Dados do jogo (localização + horário) -> Flutter
    void SendGameDataToFlutter(const char* location, const char* time);

	void exitGame();
    void GoToLauncher();

    jbyteArray as_byte_array(unsigned char* buf, int len) {
        JNIEnv* p;
        javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
		jbyteArray array = p->NewByteArray (len);
		p->SetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));
		return array;
	}

    void ShowDialog(int dialogStyle, int dialogID, char* title, char* text, char* button1, char* button2);

	void ShowEditObject();
	void HideEditObject();

    jobject activity;
    jmethodID s_setPauseState;
    jmethodID s_showLoadingScreen;
    jmethodID s_hideLoadingScreen;
    jmethodID s_updateLoadingStatus;
    jmethodID s_skipToAuthenticating;
    jmethodID s_startLoadingSequence;
    jmethodID s_onGameReady;
    jmethodID s_onNetworkReady;
    jmethodID s_setMusicPlaylist;
    jmethodID s_stopLoadingMusic;
    jmethodID s_ShowDialog;
    jmethodID s_showTab;
    jmethodID s_setTab;
    jmethodID s_clearTab;
    jmethodID s_hideTab;
        jmethodID s_showInputLayout;
    jmethodID s_hideInputLayout;
    jmethodID s_setNexionHudLogoVisible;
	jmethodID s_exitGame;
    jmethodID s_goToLauncher;    jmethodID s_showEditObject;
	jmethodID s_hideEditObject;

    jmethodID s_showBank;
    jmethodID s_updateBank;
        jmethodID s_hideBank;
    jmethodID s_showLogin;
    jmethodID s_authResult;
    jmethodID s_hideLogin;
    jmethodID s_showPhone;
    jmethodID s_hidePhone;
    jmethodID s_togglePhone;

    jmethodID s_showBankInteract;
    jmethodID s_hideBankInteract;
    jmethodID s_sendGameDataToFlutter;

};
