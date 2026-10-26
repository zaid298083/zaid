#include "main.h"
#include "settings.h"
#include "vendor/inih/cpp/INIReader.h"
#include "vendor/SimpleIni/SimpleIni.h"
#include "game/game.h"

#include <cstdio>
#include <cstring>

extern CGame *pGame;

namespace
{
    int ClampSetting(int value, int minimum, int maximum, int fallback)
    {
        return value < minimum || value > maximum ? fallback : value;
    }

    int ClampPercent(int value, int fallback)
    {
        return ClampSetting(value, 0, 100, fallback);
    }
}

CSettings::CSettings()
{
    FLog("Loading settings..");

    // Valores exclusivos do painel Nexion. -1 significa que ainda não existe
    // preferência gravada para aquele item, preservando o padrão do GTA.
    for (int index = 0; index < stSettings::NexionMobileSettingCount; ++index)
    {
        m_Settings.iNexionMobileSettings[index] = -1;
    }
    m_Settings.iNexionAspectRatio = 0;
    m_Settings.iNexionRenderScale = -1;
    m_Settings.iNexionDrawDistance = -1;
    m_Settings.iNexionVisualEffects = -1;
    m_Settings.iNexionGeneralCameraSensitivity = 50;
    m_Settings.iNexionAimCameraSensitivity = 50;
    m_Settings.iNexionMasterVolume = 100;
    m_Settings.iNexionMusicVolume = 100;
    m_Settings.iNexionRadioVolume = 100;
    m_Settings.bNexionParticleEffects = true;
    m_Settings.bNexionDistanceFog = true;
    m_Settings.bNexionSkidmarks = true;
    m_Settings.iNexionMiniMapBlipRange = 480;

    for (int context = 0; context < stSettings::NexionControlContextCount; ++context)
    {
        for (int button = 0; button < stSettings::NexionControlCount; ++button)
        {
            m_Settings.iNexionControlX[context][button] = -1;
            m_Settings.iNexionControlY[context][button] = -1;
            m_Settings.iNexionControlSize[context][button] = -1;
            m_Settings.iNexionControlOpacity[context][button] = -1;
        }
        for (int element = 0; element < stSettings::NexionHudElementCount; ++element)
        {
            m_Settings.iNexionHudX[context][element] = -1;
            m_Settings.iNexionHudY[context][element] = -1;
            m_Settings.iNexionHudSize[context][element] = -1;
            m_Settings.iNexionHudOpacity[context][element] = -1;
        }
    }

    char buff[0x7F];
    sprintf(buff, "%sSAMP/settings.ini", g_pszStorage);

    INIReader reader(buff);

    if(reader.ParseError() < 0)
    {
        FLog("Error: can't load %s - using default settings", buff);
        strcpy(m_Settings.szNickName, "__android_123");
        strcpy(m_Settings.szHost, "188.127.241.74");
        strcpy(m_Settings.szPassword, "");
        strcpy(m_Settings.szVersion, "0.3.7");
        m_Settings.iPort = 1596;
        m_Settings.bAutoAim = false;
        m_Settings.bDebug = true;
        m_Settings.bOnline = true;
        strcpy(m_Settings.szFont, "arial.ttf");
        m_Settings.fFontSize = 30.0f;
        m_Settings.iFontOutline = 2;
        m_Settings.fChatPosX = 325.0f;
        m_Settings.fChatPosY = 25.0f;
        m_Settings.fChatSizeX = 1150.0f;
        m_Settings.fChatSizeY = 220.0f;
        m_Settings.iChatMaxMessages = 6;
        m_Settings.iFPSCounter = true;
        m_Settings.iFPSCount = 120;
        return;
    }

    // client
    size_t length = 0;
    sprintf(buff, "__android_%d%d", rand() % 1000, rand() % 1000);
    length = reader.Get("client", "name", buff).copy(m_Settings.szNickName, 24);
    m_Settings.szNickName[length] = '\0';
    length = reader.Get("client", "host", "188.127.241.74").copy(m_Settings.szHost, MAX_SETTINGS_STRING);
    m_Settings.szHost[length] = '\0';
    length = reader.Get("client", "password", "").copy(m_Settings.szPassword, MAX_SETTINGS_STRING);
    m_Settings.szPassword[length] = '\0';
    length = reader.Get("client", "version", "0.3.7").copy(m_Settings.szVersion, MAX_SETTINGS_STRING);
    m_Settings.szVersion[length] = '\0';
    m_Settings.iPort = reader.GetInteger("client", "port", 1596);
    m_Settings.bAutoAim = reader.GetBoolean("client", "autoaim", false);

    // debug
    m_Settings.bDebug = reader.GetBoolean("debug", "debug", true);
    m_Settings.bOnline = reader.GetBoolean("debug", "online", true);

    // gui
    length = reader.Get("gui", "Font", "arial.ttf").copy(m_Settings.szFont, MAX_SETTINGS_STRING);
    m_Settings.szFont[length] = '\0';
    m_Settings.fFontSize = reader.GetReal("gui", "FontSize", 30.0f);
    m_Settings.iFontOutline = reader.GetInteger("gui", "FontOutline", 2);

    // chat
    m_Settings.fChatPosX = reader.GetReal("gui", "ChatPosX", 328.0f);
    m_Settings.fChatPosY = reader.GetReal("gui", "ChatPosY", 25.0f);
    m_Settings.fChatSizeX = reader.GetReal("gui", "ChatSizeX", 1150.0f);
    m_Settings.fChatSizeY = reader.GetReal("gui", "ChatSizeY", 220.0f);
    m_Settings.iChatMaxMessages = reader.GetInteger("gui", "ChatMaxMessages", 6);

    // spawnscreen
    m_Settings.fSpawnScreenPosX = reader.GetReal("gui", "SpawnScreenPosX", 660.0f);
    m_Settings.fSpawnScreenPosY = reader.GetReal("gui", "SpawnScreenPosY", 950.0f);
    m_Settings.fSpawnScreenSizeX = reader.GetReal("gui", "SpawnScreenSizeX", 600.0f);
    m_Settings.fSpawnScreenSizeY = reader.GetReal("gui", "SpawnScreenSizeY", 100.0f);

    // nametags
    m_Settings.fHealthBarWidth = reader.GetReal("gui", "HealthBarWidth", 100.0f);
    m_Settings.fHealthBarHeight = reader.GetReal("gui", "HealthBarHeight", 10.0f);

    // scoreboard
    m_Settings.fScoreBoardSizeX = reader.GetReal("gui", "ScoreBoardSizeX", 846.0f);
    m_Settings.fScoreBoardSizeY = reader.GetReal("gui", "ScoreBoardSizeY", 614.0f);

    // passenger
    m_Settings.bPassengerUseTexture = reader.GetBoolean("gui", "PassengerUseTexture", true);
    m_Settings.fPassengerTextureSize = reader.GetReal("gui", "PassengerTextureSize", 30.0f);
    m_Settings.fPassengerTextureX = reader.GetReal("gui", "PassengerTexturePosX", 120.0f);
    m_Settings.fPassengerTextureY = reader.GetReal("gui", "PassengerTexturePosY", 430.0f);

    m_Settings.iDialog = reader.GetBoolean("gui", "Dialog", true);
    m_Settings.bVoiceChatEnable = reader.GetBoolean("gui", "VoiceChatEnable", true);
    m_Settings.iVoiceChatKey = reader.GetInteger("gui", "VoiceChatKey", 66);
    m_Settings.fVoiceChatSize = reader.GetReal("gui", "VoiceChatSize", 30.0f);
    m_Settings.fVoiceChatPosX = reader.GetReal("gui", "VoiceChatPosX", 1520.0f);
    m_Settings.fVoiceChatPosY = reader.GetReal("gui", "VoiceChatPosY", 480.0f);

    m_Settings.iAndroidKeyboard = true; // Forçar teclado Android always
    m_Settings.iFirstPerson = reader.GetBoolean("gui", "firstperson", true);
    m_Settings.iCutout = reader.GetBoolean("gui", "cutout", false);
    m_Settings.iFPSCounter = reader.GetBoolean("gui", "fps", true);
    m_Settings.iFPSCount = ClampSetting(reader.GetInteger("gui", "FPSLimit", 120), 30, 240, 120);

    // Persistência completa do painel Nexion.
    m_Settings.iNexionAspectRatio = ClampSetting(reader.GetInteger("nexion", "AspectRatio", 0), 0, 4, 0);
    m_Settings.iNexionRenderScale = reader.GetInteger("nexion", "RenderScale", -1);
    if (m_Settings.iNexionRenderScale < 0 || m_Settings.iNexionRenderScale > 100) m_Settings.iNexionRenderScale = -1;
    m_Settings.iNexionDrawDistance = reader.GetInteger("nexion", "DrawDistance", -1);
    if (m_Settings.iNexionDrawDistance < 0 || m_Settings.iNexionDrawDistance > 100) m_Settings.iNexionDrawDistance = -1;
    m_Settings.iNexionVisualEffects = reader.GetInteger("nexion", "VisualEffects", -1);
    if (m_Settings.iNexionVisualEffects < 0 || m_Settings.iNexionVisualEffects > 3) m_Settings.iNexionVisualEffects = -1;

    for (int index = 0; index < stSettings::NexionMobileSettingCount; ++index)
    {
        char key[32]{};
        std::snprintf(key, sizeof(key), "MobileSetting%02d", index);
        m_Settings.iNexionMobileSettings[index] = reader.GetInteger("nexion", key, -1);
    }

    m_Settings.iNexionGeneralCameraSensitivity = ClampSetting(reader.GetInteger("nexion", "GeneralCameraSensitivity", 50), 1, 100, 50);
    m_Settings.iNexionAimCameraSensitivity = ClampSetting(reader.GetInteger("nexion", "AimCameraSensitivity", 50), 1, 100, 50);
    m_Settings.iNexionMasterVolume = ClampPercent(reader.GetInteger("nexion", "MasterVolume", 100), 100);
    m_Settings.iNexionMusicVolume = ClampPercent(reader.GetInteger("nexion", "MusicVolume", 100), 100);
    m_Settings.iNexionRadioVolume = ClampPercent(reader.GetInteger("nexion", "RadioVolume", 100), 100);
    m_Settings.bNexionParticleEffects = reader.GetBoolean("nexion", "ParticleEffects", true);
    m_Settings.bNexionDistanceFog = reader.GetBoolean("nexion", "DistanceFog", true);
    m_Settings.bNexionSkidmarks = reader.GetBoolean("nexion", "Skidmarks", true);
    m_Settings.iNexionMiniMapBlipRange = ClampSetting(reader.GetInteger("nexion", "MiniMapBlipRange", 480), 200, 1000, 480);

    for (int context = 0; context < stSettings::NexionControlContextCount; ++context)
    {
        for (int button = 0; button < stSettings::NexionControlCount; ++button)
        {
            char key[64]{};
            std::snprintf(key, sizeof(key), "Control%02d_%02d_X", context, button);
            m_Settings.iNexionControlX[context][button] = reader.GetInteger("nexion", key, -1);
            std::snprintf(key, sizeof(key), "Control%02d_%02d_Y", context, button);
            m_Settings.iNexionControlY[context][button] = reader.GetInteger("nexion", key, -1);
            std::snprintf(key, sizeof(key), "Control%02d_%02d_Size", context, button);
            m_Settings.iNexionControlSize[context][button] = reader.GetInteger("nexion", key, -1);
            std::snprintf(key, sizeof(key), "Control%02d_%02d_Opacity", context, button);
            m_Settings.iNexionControlOpacity[context][button] = reader.GetInteger("nexion", key, -1);
        }
        for (int element = 0; element < stSettings::NexionHudElementCount; ++element)
        {
            char key[64]{};
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_X", context, element);
            m_Settings.iNexionHudX[context][element] = reader.GetInteger("nexion", key, -1);
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_Y", context, element);
            m_Settings.iNexionHudY[context][element] = reader.GetInteger("nexion", key, -1);
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_Size", context, element);
            m_Settings.iNexionHudSize[context][element] = reader.GetInteger("nexion", key, -1);
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_Opacity", context, element);
            m_Settings.iNexionHudOpacity[context][element] = reader.GetInteger("nexion", key, -1);
        }
    }

    m_Settings.iHPArmourText = reader.GetBoolean("gui", "hparmourtext", false);
    m_Settings.iOutfitGuns = reader.GetBoolean("gui", "outfitguns", false);
    m_Settings.iPCMoney = reader.GetBoolean("gui", "pcmoney", false);
    m_Settings.iRadarRect = reader.GetBoolean("gui", "radarrect", false);
    m_Settings.iSkyBox = reader.GetBoolean("gui", "skybox", false);
    m_Settings.iSnow = reader.GetBoolean("gui", "snow", false);
    FLog("Settings loaded.");
}

const stSettings& CSettings::GetReadOnly()
{
    return m_Settings;
}

stSettings& CSettings::GetWrite()
{
    return m_Settings;
}

void CSettings::SaveNexionMenuSettings()
{
    char path[0x7F];
    sprintf(path, "%sSAMP/settings.ini", g_pszStorage);

    CSimpleIniA ini;
    const SI_Error loadResult = ini.LoadFile(path);
    if (loadResult < 0)
    {
        FLog("Nexion settings: can't load %s (%d), saving a new file", path, static_cast<int>(loadResult));
    }

    ini.SetLongValue("gui", "FPSLimit", ClampSetting(m_Settings.iFPSCount, 30, 240, 120), nullptr, true);
    ini.SetBoolValue("gui", "fps", m_Settings.iFPSCounter, nullptr, true);

    ini.SetLongValue("nexion", "AspectRatio", ClampSetting(m_Settings.iNexionAspectRatio, 0, 4, 0), nullptr, true);
    ini.SetLongValue("nexion", "RenderScale", ClampPercent(m_Settings.iNexionRenderScale, 65), nullptr, true);
    ini.SetLongValue("nexion", "DrawDistance", ClampPercent(m_Settings.iNexionDrawDistance, 75), nullptr, true);
    ini.SetLongValue("nexion", "VisualEffects", ClampSetting(m_Settings.iNexionVisualEffects, 0, 3, 1), nullptr, true);

    for (int index = 0; index < stSettings::NexionMobileSettingCount; ++index)
    {
        char key[32]{};
        std::snprintf(key, sizeof(key), "MobileSetting%02d", index);
        ini.SetLongValue("nexion", key, m_Settings.iNexionMobileSettings[index], nullptr, true);
    }

    ini.SetLongValue("nexion", "GeneralCameraSensitivity", ClampSetting(m_Settings.iNexionGeneralCameraSensitivity, 1, 100, 50), nullptr, true);
    ini.SetLongValue("nexion", "AimCameraSensitivity", ClampSetting(m_Settings.iNexionAimCameraSensitivity, 1, 100, 50), nullptr, true);
    ini.SetLongValue("nexion", "MasterVolume", ClampPercent(m_Settings.iNexionMasterVolume, 100), nullptr, true);
    ini.SetLongValue("nexion", "MusicVolume", ClampPercent(m_Settings.iNexionMusicVolume, 100), nullptr, true);
    ini.SetLongValue("nexion", "RadioVolume", ClampPercent(m_Settings.iNexionRadioVolume, 100), nullptr, true);
    ini.SetBoolValue("nexion", "ParticleEffects", m_Settings.bNexionParticleEffects, nullptr, true);
    ini.SetBoolValue("nexion", "DistanceFog", m_Settings.bNexionDistanceFog, nullptr, true);
    ini.SetBoolValue("nexion", "Skidmarks", m_Settings.bNexionSkidmarks, nullptr, true);
    ini.SetLongValue("nexion", "MiniMapBlipRange", ClampSetting(m_Settings.iNexionMiniMapBlipRange, 200, 1000, 480), nullptr, true);

    for (int context = 0; context < stSettings::NexionControlContextCount; ++context)
    {
        for (int button = 0; button < stSettings::NexionControlCount; ++button)
        {
            char key[64]{};
            std::snprintf(key, sizeof(key), "Control%02d_%02d_X", context, button);
            ini.SetLongValue("nexion", key, m_Settings.iNexionControlX[context][button], nullptr, true);
            std::snprintf(key, sizeof(key), "Control%02d_%02d_Y", context, button);
            ini.SetLongValue("nexion", key, m_Settings.iNexionControlY[context][button], nullptr, true);
            std::snprintf(key, sizeof(key), "Control%02d_%02d_Size", context, button);
            ini.SetLongValue("nexion", key, m_Settings.iNexionControlSize[context][button], nullptr, true);
            std::snprintf(key, sizeof(key), "Control%02d_%02d_Opacity", context, button);
            ini.SetLongValue("nexion", key, m_Settings.iNexionControlOpacity[context][button], nullptr, true);
        }
        for (int element = 0; element < stSettings::NexionHudElementCount; ++element)
        {
            char key[64]{};
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_X", context, element);
            ini.SetLongValue("nexion", key, m_Settings.iNexionHudX[context][element], nullptr, true);
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_Y", context, element);
            ini.SetLongValue("nexion", key, m_Settings.iNexionHudY[context][element], nullptr, true);
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_Size", context, element);
            ini.SetLongValue("nexion", key, m_Settings.iNexionHudSize[context][element], nullptr, true);
            std::snprintf(key, sizeof(key), "Hud%02d_%02d_Opacity", context, element);
            ini.SetLongValue("nexion", key, m_Settings.iNexionHudOpacity[context][element], nullptr, true);
        }
    }

    const SI_Error saveResult = ini.SaveFile(path);
    if (saveResult < 0)
    {
        FLog("Nexion settings: can't save %s (%d)", path, static_cast<int>(saveResult));
    }
}
