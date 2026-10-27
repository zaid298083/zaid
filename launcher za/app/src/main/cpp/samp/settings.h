#pragma once

#define MAX_SETTINGS_STRING	0x7F

struct stSettings
{
	// client
	char szNickName[24+1];
	char szHost[MAX_SETTINGS_STRING+1];
	int iPort;
	char szPassword[MAX_SETTINGS_STRING+1];
    char szVersion[MAX_SETTINGS_STRING+1];

	// debug
	bool bDebug;
	bool bOnline;
	bool bAutoAim;

	// gui
	char szFont[40];
	float fFontSize;
	int iFontOutline;
	float fChatPosX;
	float fChatPosY;
	float fChatSizeX;
	float fChatSizeY;
	int iChatMaxMessages;
	float fSpawnScreenPosX;
	float fSpawnScreenPosY;
	float fSpawnScreenSizeX;
	float fSpawnScreenSizeY;
	float fHealthBarWidth;
	float fHealthBarHeight;
	float fScoreBoardSizeX;
	float fScoreBoardSizeY;
	bool bPassengerUseTexture;
	float fPassengerTextureSize;
	float fPassengerTextureX;
	float fPassengerTextureY;
	bool bVoiceChatEnable;
	int iVoiceChatKey;
	float fVoiceChatSize;
	float fVoiceChatPosX;
	float fVoiceChatPosY;

	bool iAndroidKeyboard;
	bool iCutout;
	bool iFPSCounter;
	bool iHPArmourText;
	bool iOutfitGuns;
	bool iPCMoney;
	bool iRadarRect;
	bool iSkyBox;
	bool iSnow;
	bool iFirstPerson;
	int iFPSCount;

	// Ajustes exclusivos do painel nativo Nexion. O vetor espelha os 36 campos
	// MobileSettings da versão atual do GTA e permite restaurar cada controle
	// nativo que o jogador tiver alterado no menu.
	static constexpr int NexionMobileSettingCount = 36;
	int iNexionMobileSettings[NexionMobileSettingCount];
	int iNexionAspectRatio;
	int iNexionRenderScale;
	int iNexionDrawDistance;
	int iNexionVisualEffects;
	int iNexionGeneralCameraSensitivity;
	int iNexionAimCameraSensitivity;
	int iNexionMasterVolume;
	int iNexionMusicVolume;
	int iNexionRadioVolume;
	bool bNexionParticleEffects;
	bool bNexionDistanceFog;
	bool bNexionSkidmarks;
	int iNexionMiniMapBlipRange;

	// Editor dos controles Nexion: contexto 0 = a pé, 1 = veículo.
	static constexpr int NexionControlContextCount = 2;
	static constexpr int NexionControlCount = 17;
	static constexpr int NexionHudElementCount = 2;
	int iNexionControlX[NexionControlContextCount][NexionControlCount];
	int iNexionControlY[NexionControlContextCount][NexionControlCount];
	int iNexionControlSize[NexionControlContextCount][NexionControlCount];
	int iNexionControlOpacity[NexionControlContextCount][NexionControlCount];
	int iNexionHudX[NexionControlContextCount][NexionHudElementCount];
	int iNexionHudY[NexionControlContextCount][NexionHudElementCount];
	int iNexionHudSize[NexionControlContextCount][NexionHudElementCount];
	int iNexionHudOpacity[NexionControlContextCount][NexionHudElementCount];

	bool iDialog;
};

class CSettings
{
public:
	CSettings();
	~CSettings();

	stSettings& Get() { return m_Settings; }

    const stSettings& GetReadOnly();
    stSettings& GetWrite();

    // Persiste todos os controles editáveis do menu Nexion, preservando as
    // demais seções já existentes em SAMP/settings.ini.
    void SaveNexionMenuSettings();

private:
	struct stSettings m_Settings;
};
