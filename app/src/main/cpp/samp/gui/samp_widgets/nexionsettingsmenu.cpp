#include "nexionsettingsmenu.h"

#include "../nexion_minimap.h"
#include "../nexioncontrols.h"
#include "../../game/game.h"
#include "../../java/jniutil.h"
#include "../../main.h"
#include "../../settings.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

extern CGame* pGame;
extern CJavaWrapper* pJavaWrapper;
extern CSettings* pSettings;

// Declarado em gui.cpp. Desenha a prévia do HUD (status/velocímetro) dentro
// do editor de controles, reutilizando o mesmo código do jogo.
extern void DrawNexionEditorHudPreview(ImDrawList* draw, const ImVec2& display, float scale, int context);

// Declarado em hooks.cpp. Mantém a proporção selecionada após o GTA recalcular
// o aspecto a partir da resolução física da tela.
extern void SetNexionAspectRatio(float aspectRatio);

// Declaradas em hooks.cpp. Todas controlam rotinas de renderização reais da
// libGTASA já confirmadas nesta build, sem recriar EGL ou a superfície Android.
extern void SetNexionParticleEffectsEnabled(bool enabled);
extern bool GetNexionParticleEffectsEnabled();
extern void SetNexionDistanceFogEnabled(bool enabled);
extern bool GetNexionDistanceFogEnabled();
extern void SetNexionWorldDrawDistance(int value);
extern void SetNexionSkidmarksEnabled(bool enabled);
extern bool GetNexionSkidmarksEnabled();

// Controles confirmados em hooks.cpp: usam as rotinas nativas de câmera e
// CAudioEngine, sem depender apenas dos textos exibidos pelo menu.
extern void SetNexionGeneralCameraSensitivity(int value);
extern int GetNexionGeneralCameraSensitivity();
extern void SetNexionAimCameraSensitivity(int value);
extern int GetNexionAimCameraSensitivity();
extern void SetNexionMasterVolume(float value);
extern float GetNexionMasterVolume();
extern void SetNexionMusicVolume(float value);
extern float GetNexionMusicVolume();
extern void SetNexionRadioVolume(float value);
extern float GetNexionRadioVolume();
extern void SetNexionRadioAutoTune(bool enabled);
extern void SetNexionRadioStation(int station);

namespace
{
    const ImColor kHeaderShade(9, 10, 12, 222);
    const ImColor kContentShade(8, 8, 10, 207);
    const ImColor kNavigationRowShade(4, 5, 7, 154);
    const ImColor kNavigationShade(242, 242, 242, 235);
    const ImColor kSelectedTab(246, 246, 246, 255);
    const ImColor kTextPrimary(238, 238, 238, 255);
    const ImColor kTextMuted(185, 185, 185, 255);
    const ImColor kTextDark(31, 31, 31, 255);
    const ImColor kBlue(51, 143, 221, 255);
    constexpr int kHudSelectionOffset = 2;

    int EncodeHudSelection(int elementId)
    {
        return -(elementId + kHudSelectionOffset);
    }

    bool IsHudSelection(int selection)
    {
        return selection <= -kHudSelectionOffset;
    }

    int DecodeHudSelection(int selection)
    {
        return -selection - kHudSelectionOffset;
    }

    // Índices oficiais da tabela MobileSettings desta versão do GTA SA Mobile.
    constexpr int kSettingSteeringMode = 3;       // MS_Steering: 0..2
    constexpr int kSettingTrails = 4;             // MS_Trails: 0..1
    constexpr int kSettingTargetingMode = 6;      // MS_Targeting: 0 (fixa), 1 (livre)
    constexpr int kSettingAccelerometer = 8;      // MS_Accelerometer: 0..1
    constexpr int kSettingTouchLayout = 9;        // MS_TouchLayout: 0..1
    constexpr int kSettingInvertLook = 15;        // MS_InvertLook: 0..1
    constexpr int kSettingBikeAnalog = 18;        // MS_BikeControl: 0..1
    constexpr int kSettingAnalogSensitivity = 23; // MS_SteerAnalogScale: 20..45
    constexpr int kSettingAutoClimb = 35;         // MS_AutoClimb: 0..1
    constexpr int kSettingVisualPreset = 0;       // MOB_VS: 0..3
    constexpr int kSettingRenderScale = 1;        // MS_Resolution: 0..100
    constexpr int kSettingCameraHeight = 10;      // MS_CamHeight: 0 (normal), 1 (alta)
    constexpr int kSettingEffectsVolume = 11;     // FEA_SFX: 0..100
    constexpr int kSettingMusicVolume = 12;       // FEA_MUS: 0..100
    constexpr int kSettingRadioAutoTune = 13;     // FEA_ART: 0..1
    constexpr int kSettingRadioStation = 14;      // FEA_RSS: 1..12
    constexpr int kSettingCameraSensitivityX = 31; // MS_MouseSensitivityX: 1..100
    constexpr int kSettingCameraSensitivityY = 32; // MS_MouseSensitivityY: 1..100
    constexpr int kSettingDrawDistance = 2;       // MOB_DD: 0..100
    constexpr int kSettingShadowQuality = 5;      // MOB_SH: 0..2
    constexpr int kSettingVehicleReflection = 7;  // CAR_REF: 0..3
    constexpr int kSettingBrightness = 16;        // FED_BRI: 0..100

#if defined(__aarch64__)
    constexpr uintptr_t kMobileSettingsOffset = 0x008BEB38;
    constexpr std::size_t kMobileSettingStride = 0x28;
    constexpr std::size_t kMobileSettingValueOffset = 0x10;
#else
    constexpr uintptr_t kMobileSettingsOffset = 0x006E03FC;
    constexpr std::size_t kMobileSettingStride = 0x20;
    constexpr std::size_t kMobileSettingValueOffset = 0x08;
#endif
    constexpr std::size_t kMobileSettingDefaultDelta = sizeof(int);

    bool ContainsPoint(const ImVec2& point, const ImVec2& min, const ImVec2& max)
    {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
    }

    const char* CategoryLabel(NexionSettingsMenu::Category category)
    {
        switch (category)
        {
            case NexionSettingsMenu::Map: return "MAPA";
            case NexionSettingsMenu::Controls: return "CONTROLES";
            case NexionSettingsMenu::Game: return "JOGO";
            case NexionSettingsMenu::Audio: return "ÁUDIO";
            case NexionSettingsMenu::Settings: return "CONFIGURAÇÕES";
            default: return "MAPA";
        }
    }

    const char* const* SectionLabels(NexionSettingsMenu::Category category, int& count)
    {
        static const char* map[] = { "Mapa", "Radar", "Legenda" };
        static const char* controls[] = { "Toque" };
        static const char* game[] = { "Câmera" };
        static const char* audio[] = { "Geral", "Rádio" };
        static const char* settings[] = { "Gráficos", "Tela" };

        count = 3;
        switch (category)
        {
            case NexionSettingsMenu::Map: return map;
            case NexionSettingsMenu::Controls:
                count = 1;
                return controls;
            case NexionSettingsMenu::Game:
                count = 1;
                return game;
            case NexionSettingsMenu::Audio:
                count = 2;
                return audio;
            case NexionSettingsMenu::Settings:
                count = 2;
                return settings;
            default: return map;
        }
    }

    bool NativeSettingsReady()
    {
        // O menu só pode ser aberto após o GTA carregar a partida; nesse momento a
        // tabela está inicializada e pode ser lida/escrita diretamente.
        return g_libGTASA != 0 && pGame != nullptr && pGame->IsGameLoaded();
    }

    struct ControlEditorLayout
    {
        ImVec2 panelMin{};
        ImVec2 panelMax{};
        ImVec2 footTabMin{};
        ImVec2 footTabMax{};
        ImVec2 vehicleTabMin{};
        ImVec2 vehicleTabMax{};
        ImVec2 backMin{};
        ImVec2 backMax{};
        ImVec2 resetMin{};
        ImVec2 resetMax{};
        ImVec2 saveMin{};
        ImVec2 saveMax{};
        ImVec2 opacityMin{};
        ImVec2 opacityMax{};
        ImVec2 sizeMin{};
        ImVec2 sizeMax{};
    };

    ControlEditorLayout BuildControlEditorLayout(const ImVec2& display)
    {
        ControlEditorLayout layout{};
        const float scale = std::max(0.52f, std::min(display.x / 2048.0f, display.y / 921.0f));
        const float width = std::max(1.0f, std::min(display.x - 112.0f * scale, 500.0f * scale));
        const float height = std::max(1.0f, std::min(display.y - 128.0f * scale, 228.0f * scale));
        const float left = (display.x - width) * 0.5f;
        const float top = std::max(34.0f * scale, (display.y - height) * 0.16f);
        layout.panelMin = ImVec2(left, top);
        layout.panelMax = ImVec2(left + width, top + height);

        const float tabWidth = width * 0.5f;
        layout.footTabMin = layout.panelMin;
        layout.footTabMax = ImVec2(left + tabWidth, top + 28.0f * scale);
        layout.vehicleTabMin = ImVec2(left + tabWidth, top);
        layout.vehicleTabMax = ImVec2(left + width, top + 28.0f * scale);

        const float toolbarTop = top + 34.0f * scale;
        const float toolbarHeight = 27.0f * scale;
        const float toolbarWidth = width / 3.0f;
        layout.backMin = ImVec2(left, toolbarTop);
        layout.backMax = ImVec2(left + toolbarWidth, toolbarTop + toolbarHeight);
        layout.resetMin = ImVec2(left + toolbarWidth, toolbarTop);
        layout.resetMax = ImVec2(left + toolbarWidth * 2.0f, toolbarTop + toolbarHeight);
        layout.saveMin = ImVec2(left + toolbarWidth * 2.0f, toolbarTop);
        layout.saveMax = ImVec2(left + width, toolbarTop + toolbarHeight);

        const float sliderLeft = left + 26.0f * scale;
        const float sliderRight = left + width - 26.0f * scale;
        const float sliderHeight = 14.0f * scale;
        layout.opacityMin = ImVec2(sliderLeft, top + height - 52.0f * scale);
        layout.opacityMax = ImVec2(sliderRight, layout.opacityMin.y + sliderHeight);
        layout.sizeMin = ImVec2(sliderLeft, top + height - 22.0f * scale);
        layout.sizeMax = ImVec2(sliderRight, layout.sizeMin.y + sliderHeight);
        return layout;
    }

    int* NativeSettingValue(int index)
    {
        if (!NativeSettingsReady() || index < 0)
        {
            return nullptr;
        }

        auto* settings = reinterpret_cast<uint8_t*>(g_libGTASA + kMobileSettingsOffset);
        return reinterpret_cast<int*>(settings + static_cast<std::size_t>(index) * kMobileSettingStride + kMobileSettingValueOffset);
    }

    int* NativeSettingDefault(int index)
    {
        int* value = NativeSettingValue(index);
        return value ? reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(value) + kMobileSettingDefaultDelta) : nullptr;
    }

    int ReadNativeSetting(int index, int minimum, int maximum, int fallback)
    {
        const int* value = NativeSettingValue(index);
        if (value == nullptr || *value < minimum || *value > maximum)
        {
            return fallback;
        }
        return *value;
    }

    bool WriteNativeSetting(int index, int minimum, int maximum, int value)
    {
        int* target = NativeSettingValue(index);
        if (target == nullptr)
        {
            return false;
        }

        *target = std::max(minimum, std::min(value, maximum));
        if (pSettings)
        {
            stSettings& saved = pSettings->GetWrite();
            if (index >= 0 && index < stSettings::NexionMobileSettingCount)
            {
                saved.iNexionMobileSettings[index] = *target;
            }
            if (index == kSettingRenderScale) saved.iNexionRenderScale = *target;
            if (index == kSettingDrawDistance) saved.iNexionDrawDistance = *target;
            if (index == kSettingVisualPreset) saved.iNexionVisualEffects = *target;
        }
        return true;
    }

    void RestoreNativeSetting(int index, int minimum, int maximum, int fallback)
    {
        const int* defaultValue = NativeSettingDefault(index);
        const int value = defaultValue ? *defaultValue : fallback;
        WriteNativeSetting(index, minimum, maximum, value);
    }

    const char* EnabledLabel(int value)
    {
        return value != 0 ? "Ativado" : "Desativado";
    }

    const char* SteeringModeLabel(int value)
    {
        switch (value)
        {
            case 0: return "Analógico";
            case 1: return "Botões";
            case 2: return "Deslizar";
            default: return "Analógico";
        }
    }

    const char* TouchLayoutLabel(int value)
    {
        return value == 0 ? "Clássico" : "Adaptado";
    }

    const char* TargetingModeLabel(int value)
    {
        return value == 0 ? "Mira fixa" : "Mira livre";
    }

    const char* CameraHeightLabel(int value)
    {
        return value == 0 ? "Normal" : "Alta";
    }

    const char* RadioStationLabel(int value)
    {
        static const char* const stations[] = {
            "", "Playback FM", "K-Rose", "K-DST", "Bounce FM", "SF-UR",
            "Radio Los Santos", "Radio X", "CSR 103.9", "K-Jah West",
            "Master Sounds", "WCTR", "Desativada"
        };
        return value >= 1 && value <= 12 ? stations[value] : "Desativada";
    }

    const char* QualityLabel(int value)
    {
        static const char* const labels[] = { "Baixa", "Normal", "Alta", "Muito alta" };
        return labels[std::max(0, std::min(value, 3))];
    }

    const char* VisualEffectsLabel(int value)
    {
        static const char* const labels[] = { "Low", "Medium", "High", "Max" };
        return labels[std::max(0, std::min(value, 3))];
    }

    const char* ShadowLabel(int value)
    {
        static const char* const labels[] = { "Desativada", "Normal", "Alta" };
        return labels[std::max(0, std::min(value, 2))];
    }

    const char* AspectRatioLabel(int index)
    {
        static constexpr const char* labels[] = {
            "16:9 (Normal)", "15:9", "5:4", "4:3", "16:10"
        };
        return labels[std::max(0, std::min(index, 4))];
    }

    float AspectRatioValue(int index)
    {
        static constexpr float values[] = {
            16.0f / 9.0f, 15.0f / 9.0f, 5.0f / 4.0f, 4.0f / 3.0f, 16.0f / 10.0f
        };
        return values[std::max(0, std::min(index, 4))];
    }

    bool ApplyAspectRatio(int index)
    {
        SetNexionAspectRatio(AspectRatioValue(index));
        return true;
    }

    const char* DistanceLabel(int value)
    {
        if (value >= 90) return "Ilimitada";
        if (value >= 65) return "Alta";
        if (value >= 35) return "Média";
        return "Baixa";
    }

    const char* FrameRateLabel(int value)
    {
        static constexpr int limits[] = { 30, 60, 90, 120, 144, 240 };
        for (int limit : limits)
        {
            if (value == limit)
            {
                static char label[16]{};
                std::snprintf(label, sizeof(label), "%d FPS", limit);
                return label;
            }
        }
        return "120 FPS";
    }

    int NextFrameRateLimit(int value)
    {
        static constexpr int limits[] = { 30, 60, 90, 120, 144, 240 };
        for (std::size_t index = 0; index < sizeof(limits) / sizeof(limits[0]); ++index)
        {
            if (value == limits[index])
            {
                return limits[(index + 1) % (sizeof(limits) / sizeof(limits[0]))];
            }
        }
        return 120;
    }

    int NextResolutionScale(int value)
    {
        static constexpr int scales[] = { 50, 65, 75, 85, 100 };
        for (std::size_t index = 0; index < sizeof(scales) / sizeof(scales[0]); ++index)
        {
            if (value <= scales[index])
            {
                return scales[(index + 1) % (sizeof(scales) / sizeof(scales[0]))];
            }
        }
        return scales[0];
    }

    bool ApplyVisualEffects(int value)
    {
        const int selected = std::max(0, std::min(value, 3));
        if (!WriteNativeSetting(kSettingVisualPreset, 0, 3, selected))
        {
            return false;
        }

        if (pSettings)
        {
            pSettings->GetWrite().iNexionVisualEffects = selected;
            pSettings->SaveNexionMenuSettings();
        }
        return true;
    }

    bool ApplyDrawDistance(int value)
    {
        // MOB_DD mantém a preferência oficial do GTA, enquanto o controlador
        // de renderização aplica o far clip/LOD correspondente no mesmo quadro.
        const int selected = std::max(0, std::min(value, 100));
        if (!WriteNativeSetting(kSettingDrawDistance, 0, 100, selected))
        {
            return false;
        }

        SetNexionWorldDrawDistance(selected);
        if (pSettings)
        {
            pSettings->GetWrite().iNexionDrawDistance = selected;
            pSettings->SaveNexionMenuSettings();
        }
        return true;
    }

    void DrawSlider(ImGuiRenderer* renderer, const ImVec2& min, const ImVec2& max, float value)
    {
        value = std::max(0.0f, std::min(value, 1.0f));
        const float centerY = (min.y + max.y) * 0.5f;
        const float thickness = std::max(2.0f, (max.y - min.y) * 0.16f);
        renderer->drawRect(ImVec2(min.x, centerY - thickness * 0.5f),
                           ImVec2(max.x, centerY + thickness * 0.5f),
                           ImColor(173, 173, 173, 175), true);
        const float knobX = min.x + (max.x - min.x) * value;
        renderer->drawRect(ImVec2(min.x, centerY - thickness * 0.5f),
                           ImVec2(knobX, centerY + thickness * 0.5f),
                           kBlue, true);
        renderer->drawRect(ImVec2(knobX - 4.0f, centerY - 5.0f),
                           ImVec2(knobX + 4.0f, centerY + 5.0f),
                           kBlue, true);
    }
}

int GetNexionTargetingMode()
{
    const int* value = NativeSettingValue(kSettingTargetingMode);
    if (value && *value >= 0 && *value <= 1)
        return *value;

    if (pSettings)
    {
        const int saved = pSettings->GetReadOnly().iNexionMobileSettings[kSettingTargetingMode];
        if (saved >= 0 && saved <= 1)
            return saved;
    }

    return 0;
}

NexionSettingsMenu::NexionSettingsMenu()
{
    setVisible(false);

    m_controlEditorOpen = false;
    m_controlEditorSelected = -1;
    m_controlEditorDragging = false;
    m_controlEditorOpacityDragging = false;
    m_controlEditorSizeDragging = false;
}

void NexionSettingsMenu::restoreSavedSettings()
{
    if (!pSettings || !NativeSettingsReady())
    {
        return;
    }

    const stSettings& saved = pSettings->GetReadOnly();
    const auto restoreNative = [&](int index, int minimum, int maximum)
    {
        const int value = saved.iNexionMobileSettings[index];
        if (value >= minimum && value <= maximum)
        {
            WriteNativeSetting(index, minimum, maximum, value);
        }
    };

    restoreNative(kSettingSteeringMode, 0, 2);
    restoreNative(kSettingTrails, 0, 1);
    restoreNative(kSettingTargetingMode, 0, 1);
    restoreNative(kSettingAccelerometer, 0, 1);
    restoreNative(kSettingTouchLayout, 0, 1);
    restoreNative(kSettingCameraHeight, 0, 1);
    restoreNative(kSettingEffectsVolume, 0, 100);
    restoreNative(kSettingMusicVolume, 0, 100);
    restoreNative(kSettingRadioAutoTune, 0, 1);
    restoreNative(kSettingRadioStation, 1, 12);
    restoreNative(kSettingInvertLook, 0, 1);
    restoreNative(kSettingBikeAnalog, 0, 1);
    restoreNative(kSettingAnalogSensitivity, 20, 45);
    restoreNative(kSettingCameraSensitivityX, 1, 100);
    restoreNative(kSettingCameraSensitivityY, 1, 100);
    restoreNative(kSettingAutoClimb, 0, 1);
    restoreNative(kSettingShadowQuality, 0, 2);
    restoreNative(kSettingVehicleReflection, 0, 3);
    restoreNative(kSettingBrightness, 0, 100);

    if (saved.iNexionRenderScale >= 0 && saved.iNexionRenderScale <= 100)
    {
        WriteNativeSetting(kSettingRenderScale, 0, 100, saved.iNexionRenderScale);
    }
    if (saved.iNexionVisualEffects >= 0 && saved.iNexionVisualEffects <= 3)
    {
        WriteNativeSetting(kSettingVisualPreset, 0, 3, saved.iNexionVisualEffects);
    }
    if (saved.iNexionDrawDistance >= 0 && saved.iNexionDrawDistance <= 100)
    {
        ApplyDrawDistance(saved.iNexionDrawDistance);
    }

    m_screenAspect = std::max(0, std::min(saved.iNexionAspectRatio, 4));
    ApplyAspectRatio(m_screenAspect);
    SetNexionGeneralCameraSensitivity(saved.iNexionGeneralCameraSensitivity);
    SetNexionAimCameraSensitivity(saved.iNexionAimCameraSensitivity);
    SetNexionMasterVolume(saved.iNexionMasterVolume / 100.0f);
    SetNexionMusicVolume(saved.iNexionMusicVolume / 100.0f);
    SetNexionRadioVolume(saved.iNexionRadioVolume / 100.0f);
    SetNexionParticleEffectsEnabled(saved.bNexionParticleEffects);
    SetNexionDistanceFogEnabled(saved.bNexionDistanceFog);
    SetNexionSkidmarksEnabled(saved.bNexionSkidmarks);
    SetNexionRadioAutoTune(ReadNativeSetting(kSettingRadioAutoTune, 0, 1, 1) != 0);
    SetNexionRadioStation(ReadNativeSetting(kSettingRadioStation, 1, 12, 12));
    NexionMiniMap::SetMiniMapBlipVisibilityRange(static_cast<float>(saved.iNexionMiniMapBlipRange));
}

void NexionSettingsMenu::captureAndSaveSettings()
{
    if (!pSettings || !NativeSettingsReady())
    {
        return;
    }

    stSettings& saved = pSettings->GetWrite();
    const auto captureNative = [&](int index, int minimum, int maximum, int fallback)
    {
        saved.iNexionMobileSettings[index] = ReadNativeSetting(index, minimum, maximum, fallback);
    };

    captureNative(kSettingSteeringMode, 0, 2, 0);
    captureNative(kSettingTrails, 0, 1, 1);
    captureNative(kSettingTargetingMode, 0, 1, 0);
    captureNative(kSettingAccelerometer, 0, 1, 0);
    captureNative(kSettingTouchLayout, 0, 1, 0);
    captureNative(kSettingCameraHeight, 0, 1, 0);
    captureNative(kSettingEffectsVolume, 0, 100, 100);
    captureNative(kSettingMusicVolume, 0, 100, 100);
    captureNative(kSettingRadioAutoTune, 0, 1, 1);
    captureNative(kSettingRadioStation, 1, 12, 12);
    captureNative(kSettingInvertLook, 0, 1, 0);
    captureNative(kSettingBikeAnalog, 0, 1, 0);
    captureNative(kSettingAnalogSensitivity, 20, 45, 27);
    captureNative(kSettingCameraSensitivityX, 1, 100, 50);
    captureNative(kSettingCameraSensitivityY, 1, 100, 50);
    captureNative(kSettingAutoClimb, 0, 1, 0);
    captureNative(kSettingShadowQuality, 0, 2, 1);
    captureNative(kSettingVehicleReflection, 0, 3, 1);
    captureNative(kSettingBrightness, 0, 100, 60);
    captureNative(kSettingRenderScale, 0, 100, 65);
    captureNative(kSettingVisualPreset, 0, 3, 1);
    captureNative(kSettingDrawDistance, 0, 100, 75);

    saved.iNexionRenderScale = saved.iNexionMobileSettings[kSettingRenderScale];
    saved.iNexionVisualEffects = saved.iNexionMobileSettings[kSettingVisualPreset];
    saved.iNexionDrawDistance = saved.iNexionMobileSettings[kSettingDrawDistance];
    saved.iNexionAspectRatio = std::max(0, std::min(m_screenAspect, 4));
    saved.iNexionGeneralCameraSensitivity = GetNexionGeneralCameraSensitivity();
    saved.iNexionAimCameraSensitivity = GetNexionAimCameraSensitivity();
    saved.iNexionMasterVolume = static_cast<int>(std::lround(GetNexionMasterVolume() * 100.0f));
    saved.iNexionMusicVolume = static_cast<int>(std::lround(GetNexionMusicVolume() * 100.0f));
    saved.iNexionRadioVolume = static_cast<int>(std::lround(GetNexionRadioVolume() * 100.0f));
    saved.bNexionParticleEffects = GetNexionParticleEffectsEnabled();
    saved.bNexionDistanceFog = GetNexionDistanceFogEnabled();
    saved.bNexionSkidmarks = GetNexionSkidmarksEnabled();
    saved.iNexionMiniMapBlipRange = static_cast<int>(std::lround(NexionMiniMap::GetMiniMapBlipVisibilityRange()));
    pSettings->SaveNexionMenuSettings();
}

void NexionSettingsMenu::applySavedSettingsOnStartup()
{
    // MobileSettings e os subsistemas de áudio/câmera só são confiáveis após o
    // jogador local existir. A UI chama este método nesse ponto e a flag impede
    // reaplicações a cada quadro.
    if (m_savedSettingsAppliedOnStartup || !NativeSettingsReady())
    {
        return;
    }

    restoreSavedSettings();
    m_savedSettingsAppliedOnStartup = true;
}

void NexionSettingsMenu::show()
{
    // O menu sempre começa em Configurações → Gráficos, como solicitado.
    m_category = Settings;
    m_selectedSection[Settings] = 0;
    m_optionScroll[Settings][0] = 0;
    m_touchActive = false;
    m_draggingNativeSensitivity = false;
    m_draggingCameraAudioSlider = false;
    // Também atualiza a tela a partir do arquivo caso ele tenha sido alterado
    // entre sessões, sem depender de valores que ficaram na memória.
    restoreSavedSettings();
    m_savedSettingsAppliedOnStartup = true;

    setVisible(true);
    if (pGame) pGame->EnableGameInput(false);
    if (pJavaWrapper) pJavaWrapper->SetNexionHudLogoVisible(false);
}

void NexionSettingsMenu::hide()
{
    if (m_controlEditorOpen)
    {
        NexionControls::SaveEditorLayout();
        m_controlEditorOpen = false;
    }
    m_touchActive = false;
    m_draggingNativeSensitivity = false;
    m_draggingCameraAudioSlider = false;
    m_activeCameraAudioSlider = -1;
    captureAndSaveSettings();
    setVisible(false);
    if (pGame) pGame->EnableGameInput(true);
    if (pJavaWrapper && !NexionMiniMap::IsLargeMapOpen()) pJavaWrapper->SetNexionHudLogoVisible(true);
}

void NexionSettingsMenu::performLayout()
{
    if (parent())
    {
        setFixedSize(parent()->size());
        setPosition(ImVec2(0.0f, 0.0f));
    }
}

NexionSettingsMenu::LayoutMetrics NexionSettingsMenu::buildLayout() const
{
    LayoutMetrics layout{};
    layout.display = size();
    const float scaleX = layout.display.x / 2048.0f;
    const float scaleY = layout.display.y / 921.0f;
    layout.scale = std::max(0.52f, std::min(scaleX, scaleY));

    const float menuWidth = std::min(layout.display.x - 48.0f * layout.scale, 1400.0f * layout.scale);
    const float left = (layout.display.x - menuWidth) * 0.5f;
    // Mantém o painel no terço superior, em vez de centralizá-lo verticalmente.
    const float top = std::max(38.0f * layout.scale, 86.0f * layout.scale);
    const float tabHeight = 44.0f * layout.scale;
    const float exitWidth = 158.0f * layout.scale;
    const float tabsWidth = menuWidth - exitWidth;

    layout.headerMin = ImVec2(left, top);
    layout.headerMax = ImVec2(left + menuWidth, top + tabHeight);
    layout.exitMin = ImVec2(left + tabsWidth, top);
    layout.exitMax = layout.headerMax;

    const float singleTabWidth = tabsWidth / static_cast<float>(CategoryCount);
    for (int index = 0; index < CategoryCount; ++index)
    {
        layout.tabMin[index] = ImVec2(left + singleTabWidth * index, top);
        layout.tabMax[index] = ImVec2(left + singleTabWidth * (index + 1), top + tabHeight);
    }

    layout.contentMin = ImVec2(left, layout.headerMax.y + 16.0f * layout.scale);

    // A altura do painel acompanha o conteúdo da seção ativa. Assim, a área
    // de opções não conserva um bloco vazio grande abaixo das configurações.
    const int selectedSection = std::max(0, m_selectedSection[static_cast<int>(m_category)]);
    int optionCount = 4;
    if (m_category == Controls) optionCount = 9;
    else if (m_category == Map) optionCount = 6;
    else if (m_category == Game) optionCount = 4;
    else if (m_category == Audio) optionCount = selectedSection == 0 ? 3 : 4;
    else if (selectedSection == 0) optionCount = 11;
    else if (selectedSection == 1) optionCount = 4;
    else optionCount = 5;

    const float optionRowHeight = (m_category == Settings && selectedSection == 0)
        ? 45.0f * layout.scale
        : 50.0f * layout.scale;
    // Sem rodapé de setas: o painel termina logo após a última opção visível.
    const float contentHeight = 6.0f * layout.scale + optionCount * optionRowHeight;
    layout.contentMax = ImVec2(left + menuWidth, layout.contentMin.y + contentHeight);
    layout.navigationMin = layout.contentMin;
    layout.navigationMax = ImVec2(left + menuWidth * 0.32f, layout.contentMax.y);
    layout.rowHeight = 50.0f * layout.scale;
    return layout;
}

void NexionSettingsMenu::selectCategory(Category category)
{
    m_category = category;
    m_draggingNativeSensitivity = false;
    m_draggingCameraAudioSlider = false;
    m_activeCameraAudioSlider = -1;
    m_trackingGraphicsScroll = false;
    m_draggingGraphicsScroll = false;
}

void NexionSettingsMenu::drawControlEditor(ImGuiRenderer* renderer)
{
    if (!renderer) return;
    const ImVec2 display = size();
    const float scale = std::max(0.52f, std::min(display.x / 2048.0f, display.y / 921.0f));
    const ControlEditorLayout layout = BuildControlEditorLayout(display);

    renderer->drawRect(ImVec2(0.0f, 0.0f), display, ImColor(0, 0, 0, 112), true);

    for (int button = 0; button < NexionControls::kEditorButtonCount; ++button)
    {
        if (!NexionControls::EditorButtonVisible(m_controlEditorContext, button))
            continue;
        ImVec2 center{};
        float buttonSize = 0.0f;
        float opacity = 1.0f;
        NexionControls::GetEditorButton(m_controlEditorContext, button, display,
                                        center, buttonSize, opacity);
        const ImTextureID texture = NexionControls::EditorButtonTexture(button);
        const ImVec2 half(buttonSize * 0.5f, buttonSize * 0.5f);
        if (texture)
        {
            renderer->drawImage(center - half, center + half, texture,
                                ImColor(255, 255, 255,
                                        static_cast<int>(std::max(0.0f, std::min(1.0f, opacity)) * 255.0f)));
        }
        else
        {
            renderer->drawRect(center - half, center + half,
                               ImColor(232, 232, 232,
                                       static_cast<int>(opacity * 220.0f)), true);
        }
    }

    // A prévia usa o mesmo desenho real do HUD do jogo; apenas o overlay e
    // o contorno de seleção são específicos do editor.
    DrawNexionEditorHudPreview(ImGui::GetBackgroundDrawList(), display, scale,
                               m_controlEditorContext);

    for (int element = 0; element < NexionControls::kEditorHudElementCount; ++element)
    {
        if (!NexionControls::EditorHudVisible(m_controlEditorContext, element))
            continue;
        ImVec2 center{};
        float elementSize = 0.0f;
        float opacity = 1.0f;
        NexionControls::GetEditorHud(m_controlEditorContext, element, display,
                                     center, elementSize, opacity);
    }

    renderer->drawRect(layout.panelMin, layout.panelMax, ImColor(0, 0, 0, 236), true);
    const bool footSelected = m_controlEditorContext == NexionControls::kEditorContextOnFoot;
    renderer->drawRect(layout.footTabMin, layout.footTabMax,
                       footSelected ? kSelectedTab : ImColor(12, 12, 12, 235), true);
    renderer->drawRect(layout.vehicleTabMin, layout.vehicleTabMax,
                       !footSelected ? kSelectedTab : ImColor(12, 12, 12, 235), true);
    renderer->drawText(layout.footTabMin + ImVec2(42.0f * scale, 8.0f * scale),
                       footSelected ? kTextDark : kTextPrimary, "Padrão", false, 12.0f * scale);
    renderer->drawText(layout.vehicleTabMin + ImVec2(38.0f * scale, 8.0f * scale),
                       !footSelected ? kTextDark : kTextPrimary, "Veículo", false, 12.0f * scale);

    renderer->drawRect(layout.backMin, layout.backMax, ImColor(244, 244, 244, 232), true);
    renderer->drawRect(layout.resetMin, layout.resetMax, ImColor(244, 244, 244, 232), true);
    renderer->drawRect(layout.saveMin, layout.saveMax, kBlue, true);
    renderer->drawText(layout.backMin + ImVec2(14.0f * scale, 8.0f * scale), kTextDark,
                       "Sair", false, 11.0f * scale);
    renderer->drawText(layout.resetMin + ImVec2(10.0f * scale, 8.0f * scale), kTextDark,
                       "Padrão", false, 11.0f * scale);
    renderer->drawText(layout.saveMin + ImVec2(16.0f * scale, 8.0f * scale), kTextDark,
                       "Salvar", false, 11.0f * scale);

    const char* selectedName = "Selecione um elemento";
    if (m_controlEditorSelected >= 0)
        selectedName = NexionControls::EditorButtonName(m_controlEditorSelected);
    else if (IsHudSelection(m_controlEditorSelected))
        selectedName = NexionControls::EditorHudName(DecodeHudSelection(m_controlEditorSelected));
    renderer->drawText(layout.panelMin + ImVec2(18.0f * scale, 72.0f * scale), kTextPrimary,
                       selectedName, false, 12.0f * scale);
    renderer->drawText(layout.panelMin + ImVec2(18.0f * scale, 94.0f * scale), kTextMuted,
                       "Arraste o elemento para mudar sua posição", false, 9.0f * scale);

    float opacity = 1.0f;
    float buttonSize = 82.0f * scale;
    ImVec2 selectedCenter{};
    if (m_controlEditorSelected >= 0)
    {
        NexionControls::GetEditorButton(m_controlEditorContext, m_controlEditorSelected,
                                        display, selectedCenter, buttonSize, opacity);
    }
    else if (IsHudSelection(m_controlEditorSelected))
    {
        NexionControls::GetEditorHud(m_controlEditorContext, DecodeHudSelection(m_controlEditorSelected),
                                     display, selectedCenter, buttonSize, opacity);
    }
    const float opacityValue = opacity * 100.0f;
    const float sizeValue = buttonSize / scale;
    const float maxSizeValue = IsHudSelection(m_controlEditorSelected) ? 640.0f : 240.0f;
    renderer->drawText(layout.panelMin + ImVec2(18.0f * scale, layout.panelMax.y - 68.0f * scale),
                       kTextPrimary, "Transparência", false, 10.0f * scale);
    renderer->drawText(ImVec2(layout.panelMax.x - 54.0f * scale,
                              layout.panelMax.y - 68.0f * scale), kTextPrimary,
                       std::to_string(static_cast<int>(std::lround(opacityValue))) + "%",
                       false, 10.0f * scale);
    renderer->drawRect(layout.opacityMin, layout.opacityMax, ImColor(120, 120, 120, 180), true);
    renderer->drawRect(layout.opacityMin,
                       ImVec2(layout.opacityMin.x + (layout.opacityMax.x - layout.opacityMin.x) * opacity,
                              layout.opacityMax.y), kBlue, true);

    renderer->drawText(layout.panelMin + ImVec2(18.0f * scale, layout.panelMax.y - 38.0f * scale),
                       kTextPrimary, "Tamanho", false, 10.0f * scale);
    renderer->drawText(ImVec2(layout.panelMax.x - 54.0f * scale,
                              layout.panelMax.y - 38.0f * scale), kTextPrimary,
                       std::to_string(static_cast<int>(std::lround(sizeValue))),
                       false, 10.0f * scale);
    const float sizeFraction = std::max(0.0f, std::min(1.0f,
        (sizeValue - 36.0f) / std::max(1.0f, maxSizeValue - 36.0f)));
    renderer->drawRect(layout.sizeMin, layout.sizeMax, ImColor(120, 120, 120, 180), true);
    renderer->drawRect(layout.sizeMin,
                       ImVec2(layout.sizeMin.x + (layout.sizeMax.x - layout.sizeMin.x) * sizeFraction,
                              layout.sizeMax.y), kBlue, true);
}

void NexionSettingsMenu::touchControlEditor(const ImVec2& pos, TouchType type)
{
    const ImVec2 display = size();
    const ControlEditorLayout layout = BuildControlEditorLayout(display);
    const float scale = std::max(0.52f, std::min(display.x / 2048.0f, display.y / 921.0f));
    auto hasSelection = [&]()
    {
        return m_controlEditorSelected >= 0 || IsHudSelection(m_controlEditorSelected);
    };
    auto updateSlider = [&](const ImVec2& point)
    {
        if (!hasSelection()) return;
        ImVec2 center{};
        float elementSize = 0.0f;
        float opacity = 1.0f;
        const bool hud = IsHudSelection(m_controlEditorSelected);
        const int id = hud ? DecodeHudSelection(m_controlEditorSelected) : m_controlEditorSelected;
        if (hud)
            NexionControls::GetEditorHud(m_controlEditorContext, id, display, center, elementSize, opacity);
        else
            NexionControls::GetEditorButton(m_controlEditorContext, id, display, center, elementSize, opacity);
        if (m_controlEditorOpacityDragging)
        {
            opacity = std::max(0.0f, std::min(1.0f,
                (point.x - layout.opacityMin.x) /
                std::max(1.0f, layout.opacityMax.x - layout.opacityMin.x)));
        }
        if (m_controlEditorSizeDragging)
        {
            const float maxValue = hud ? 640.0f : 240.0f;
            const float fraction = std::max(0.0f, std::min(1.0f,
                (point.x - layout.sizeMin.x) /
                std::max(1.0f, layout.sizeMax.x - layout.sizeMin.x)));
            elementSize = (36.0f + fraction * (maxValue - 36.0f)) * scale;
        }
        if (hud)
            NexionControls::SetEditorHud(m_controlEditorContext, id, display, center, elementSize, opacity);
        else
            NexionControls::SetEditorButton(m_controlEditorContext, id, display, center, elementSize, opacity);
    };

    if (type == TouchType::push)
    {
        const ImVec2 backHitMin(layout.backMin.x, layout.backMin.y - 12.0f * scale);
        const ImVec2 backHitMax(layout.backMax.x, layout.backMax.y + 12.0f * scale);
        if (ContainsPoint(pos, backHitMin, backHitMax))
        {
            NexionControls::SaveEditorLayout();
            m_controlEditorOpen = false;
            m_controlEditorSelected = -1;
            m_controlEditorDragging = false;
            m_controlEditorOpacityDragging = false;
            m_controlEditorSizeDragging = false;
            return;
        }
        if (ContainsPoint(pos, layout.footTabMin, layout.footTabMax))
        {
            m_controlEditorContext = NexionControls::kEditorContextOnFoot;
            m_controlEditorSelected = -1;
            return;
        }
        if (ContainsPoint(pos, layout.vehicleTabMin, layout.vehicleTabMax))
        {
            m_controlEditorContext = NexionControls::kEditorContextVehicle;
            m_controlEditorSelected = -1;
            return;
        }
        if (ContainsPoint(pos, layout.resetMin, layout.resetMax))
        {
            NexionControls::ResetEditorContext(m_controlEditorContext);
            NexionControls::ResetEditorHudContext(m_controlEditorContext);
            NexionControls::SaveEditorLayout();
            m_controlEditorSelected = -1;
            return;
        }
        if (ContainsPoint(pos, layout.saveMin, layout.saveMax))
        {
            NexionControls::SaveEditorLayout();
            return;
        }
        if (hasSelection() && ContainsPoint(pos, layout.opacityMin, layout.opacityMax))
        {
            m_controlEditorOpacityDragging = true;
            updateSlider(pos);
            return;
        }
        if (hasSelection() && ContainsPoint(pos, layout.sizeMin, layout.sizeMax))
        {
            m_controlEditorSizeDragging = true;
            updateSlider(pos);
            return;
        }
        for (int button = 0; button < NexionControls::kEditorButtonCount; ++button)
        {
            if (!NexionControls::EditorButtonVisible(m_controlEditorContext, button)) continue;
            ImVec2 center{};
            float buttonSize = 0.0f;
            float opacity = 1.0f;
            NexionControls::GetEditorButton(m_controlEditorContext, button, display,
                                            center, buttonSize, opacity);
            if (ContainsPoint(pos, center - ImVec2(buttonSize * 0.5f, buttonSize * 0.5f),
                              center + ImVec2(buttonSize * 0.5f, buttonSize * 0.5f)))
            {
                m_controlEditorSelected = button;
                m_controlEditorDragging = true;
                m_controlEditorDragOffset = center - pos;
                return;
            }
        }
        for (int element = 0; element < NexionControls::kEditorHudElementCount; ++element)
        {
            if (!NexionControls::EditorHudVisible(m_controlEditorContext, element)) continue;
            ImVec2 center{};
            float elementSize = 0.0f;
            float opacity = 1.0f;
            NexionControls::GetEditorHud(m_controlEditorContext, element, display,
                                         center, elementSize, opacity);
            if (ContainsPoint(pos, center - ImVec2(elementSize * 0.5f, elementSize * 0.5f),
                              center + ImVec2(elementSize * 0.5f, elementSize * 0.5f)))
            {
                m_controlEditorSelected = EncodeHudSelection(element);
                m_controlEditorDragging = true;
                m_controlEditorDragOffset = center - pos;
                return;
            }
        }
        return;
    }

    if (type == TouchType::move && hasSelection())
    {
        if (m_controlEditorDragging)
        {
            const bool hud = IsHudSelection(m_controlEditorSelected);
            const int id = hud ? DecodeHudSelection(m_controlEditorSelected) : m_controlEditorSelected;
            ImVec2 center{};
            float elementSize = 0.0f;
            float opacity = 1.0f;
            if (hud)
                NexionControls::GetEditorHud(m_controlEditorContext, id, display, center, elementSize, opacity);
            else
                NexionControls::GetEditorButton(m_controlEditorContext, id, display, center, elementSize, opacity);
            center = pos + m_controlEditorDragOffset;

            // Os HUDs não são quadrados: Status é uma faixa horizontal e o
            // velocímetro é um painel 340x124. Usar elementSize como metade
            // dos dois eixos criava uma margem vertical artificial enorme.
            float halfX = elementSize * 0.5f;
            float halfY = halfX;
            if (hud)
            {
                if (id == 0)
                {
                    const bool vehicleContext =
                        m_controlEditorContext == NexionControls::kEditorContextVehicle;
                    const float cell = (vehicleContext ? 51.0f : 56.0f) * scale;
                    const float gap = 8.0f * scale;
                    const float count = vehicleContext ? 6.0f : 4.0f;
                    const float baseWidth = cell * count + gap * (count - 1.0f);
                    const float hudScale = elementSize / std::max(baseWidth, 1.0f);
                    halfY = cell * hudScale * 0.5f;
                }
                else if (id == 1)
                {
                    halfY = elementSize * (124.0f / 340.0f) * 0.5f;
                }
            }
            center.x = std::max(halfX, std::min(display.x - halfX, center.x));
            center.y = std::max(halfY, std::min(display.y - halfY, center.y));
            if (hud)
                NexionControls::SetEditorHud(m_controlEditorContext, id, display, center, elementSize, opacity);
            else
                NexionControls::SetEditorButton(m_controlEditorContext, id, display, center, elementSize, opacity);
        }
        else if (m_controlEditorOpacityDragging || m_controlEditorSizeDragging)
        {
            updateSlider(pos);
        }
        return;
    }

    if (type == TouchType::pop)
    {
        if (m_controlEditorDragging || m_controlEditorOpacityDragging || m_controlEditorSizeDragging)
        {
            if (m_controlEditorOpacityDragging || m_controlEditorSizeDragging)
                updateSlider(pos);
            NexionControls::SaveEditorLayout();
        }
        m_controlEditorDragging = false;
        m_controlEditorOpacityDragging = false;
        m_controlEditorSizeDragging = false;
    }
}

void NexionSettingsMenu::draw(ImGuiRenderer* renderer)
{
    if (!visible() || !renderer) return;
    if (m_controlEditorOpen)
    {
        drawControlEditor(renderer);
        return;
    }

    const LayoutMetrics layout = buildLayout();
    const float scale = layout.scale;
    const int activeCategory = static_cast<int>(m_category);

    renderer->drawText(ImVec2(layout.headerMin.x, layout.headerMin.y - 40.0f * scale),
                       kTextPrimary, "Nexion Online", false, 27.0f * scale);
    renderer->drawRect(layout.headerMin, layout.headerMax, kHeaderShade, true);

    for (int index = 0; index < CategoryCount; ++index)
    {
        const bool selected = index == activeCategory;
        if (selected)
        {
            renderer->drawRect(layout.tabMin[index], layout.tabMax[index], kSelectedTab, true);
        }

        const char* label = CategoryLabel(static_cast<Category>(index));
        const ImVec2 textSize = renderer->calculateTextSize(label, 13.0f * scale);
        const ImVec2 textPos((layout.tabMin[index].x + layout.tabMax[index].x - textSize.x) * 0.5f,
                             layout.tabMin[index].y + 11.0f * scale);
        renderer->drawText(textPos, selected ? kTextDark : kTextMuted, label, false, 13.0f * scale);
    }

    renderer->drawRect(layout.exitMin, layout.exitMax, ImColor(0, 0, 0, 72), true);
    const ImVec2 exitTextSize = renderer->calculateTextSize("SAIR", 13.0f * scale);
    renderer->drawText(ImVec2((layout.exitMin.x + layout.exitMax.x - exitTextSize.x) * 0.5f,
                              layout.exitMin.y + 11.0f * scale),
                       kTextMuted, "SAIR", false, 13.0f * scale);

    int sectionCount = 0;
    const char* const* sections = SectionLabels(m_category, sectionCount);
    const int currentSection = std::max(0, std::min(m_selectedSection[activeCategory], sectionCount - 1));
    // Os botões laterais são independentes, com uma folga entre eles; não
    // formam uma coluna contínua nem ficam visualmente colados.
    const float navTop = layout.navigationMin.y + 2.0f * scale;
    const float navigationRowHeight = 44.0f * scale;
    const float navigationGap = 7.0f * scale;
    for (int section = 0; section < sectionCount; ++section)
    {
        const ImVec2 rowMin(layout.navigationMin.x,
                            navTop + section * (navigationRowHeight + navigationGap));
        const ImVec2 rowMax(layout.navigationMax.x, rowMin.y + navigationRowHeight);
        const bool selected = section == currentSection;
        // Mesmo as opções não selecionadas recebem um fundo escuro sutil, como
        // na referência, preservando o branco apenas para a seleção atual.
        renderer->drawRect(rowMin, rowMax, selected ? kNavigationShade : kNavigationRowShade, true);
        renderer->drawText(rowMin + ImVec2(12.0f * scale, 10.0f * scale),
                           selected ? kTextDark : kTextPrimary,
                           sections[section], false, 15.0f * scale);
    }

    const ImVec2 optionMin(layout.navigationMax.x + 1.0f * scale, layout.contentMin.y);
    const ImVec2 optionMax(layout.contentMax.x, layout.contentMax.y);
    // Como na referência, somente as configurações à direita recebem o painel escuro.
    // A coluna de seções fica diretamente sobre a cena do jogo.
    renderer->drawRect(optionMin, optionMax, kContentShade, true);
    const float labelX = optionMin.x + 15.0f * scale;
    const float valueX = optionMax.x - 15.0f * scale;
    const float optionsBottom = optionMax.y;
    const bool compactGraphicsRows = m_category == Settings && currentSection == 0;
    const float optionRowHeight = compactGraphicsRows ? 45.0f * scale : layout.rowHeight;
    float rowY = optionMin.y + 2.0f * scale;

    const auto drawOption = [&](const char* label, const char* value, bool slider, float sliderValue)
    {
        const ImVec2 rowMin(optionMin.x, rowY);
        const ImVec2 rowMax(optionMax.x, rowY + optionRowHeight);
        const bool rowVisible = rowMin.y >= optionMin.y && rowMax.y <= optionsBottom;
        if (rowVisible)
        {
            renderer->drawText(ImVec2(labelX, rowMin.y + 11.5f * scale), kTextPrimary, label, false, 15.0f * scale);
            if (slider)
            {
                DrawSlider(renderer, ImVec2(valueX - 130.0f * scale, rowMin.y + 6.0f * scale),
                           ImVec2(valueX, rowMax.y - 6.0f * scale), sliderValue);
            }
            else if (value && value[0] != '\0')
            {
                const ImVec2 valueSize = renderer->calculateTextSize(value, 15.0f * scale);
                renderer->drawText(ImVec2(valueX - valueSize.x, rowMin.y + 11.5f * scale),
                                   kTextPrimary, value, false, 15.0f * scale);
            }
        }
        rowY += optionRowHeight;
    };

    if (m_category == Controls)
    {
        const int steering = ReadNativeSetting(kSettingSteeringMode, 0, 2, 0);
        const int accelerometer = ReadNativeSetting(kSettingAccelerometer, 0, 1, 0);
        const int invertLook = ReadNativeSetting(kSettingInvertLook, 0, 1, 0);
        const int bikeAnalog = ReadNativeSetting(kSettingBikeAnalog, 0, 1, 0);
        const int sensitivity = ReadNativeSetting(kSettingAnalogSensitivity, 20, 45, 27);
        const int autoClimb = ReadNativeSetting(kSettingAutoClimb, 0, 1, 0);
        const int targeting = ReadNativeSetting(kSettingTargetingMode, 0, 1, 0);

        drawOption("Editar controle", "Abrir editor", false, 0.0f);
        drawOption("Modo de direção", SteeringModeLabel(steering), false, 0.0f);
        drawOption("Acelerômetro", EnabledLabel(accelerometer), false, 0.0f);
        drawOption("Inverter visualização", EnabledLabel(invertLook), false, 0.0f);
        drawOption("Direção analógica da bicicleta", EnabledLabel(bikeAnalog), false, 0.0f);
        drawOption("Sensibilidade do controle analógico", "", true, (sensitivity - 20.0f) / 25.0f);
        drawOption("Subida automática", EnabledLabel(autoClimb), false, 0.0f);
        drawOption("Modo de mira", TargetingModeLabel(targeting), false, 0.0f);
        drawOption("Restaurar padrões", "", false, 0.0f);
    }
    else if (m_category == Map)
    {
        char rangeValue[32]{};
        std::snprintf(rangeValue, sizeof(rangeValue), "%.0f m", NexionMiniMap::GetMiniMapBlipVisibilityRange());
        drawOption("Exibir minimapa", "Ativado", false, 0.0f);
        drawOption("Alcance de blips", rangeValue, true,
                   (NexionMiniMap::GetMiniMapBlipVisibilityRange() - 200.0f) / 800.0f);
        drawOption("Rota GPS por ruas", "Ativada", false, 0.0f);
        drawOption("Zoom ao entrar em veículo", "Ativado", false, 0.0f);
        drawOption("Legenda de locais", "Ativada", false, 0.0f);
        drawOption("Mapa grande livre", "Ativado", false, 0.0f);
    }
    else if (m_category == Game)
    {
        const int cameraHeight = ReadNativeSetting(kSettingCameraHeight, 0, 1, 0);
        drawOption("Altura da câmera do veículo", CameraHeightLabel(cameraHeight), false, 0.0f);
        drawOption("Sensibilidade geral", "", true, GetNexionGeneralCameraSensitivity() / 100.0f);
        drawOption("Sensibilidade em mira", "", true, GetNexionAimCameraSensitivity() / 100.0f);
        drawOption("Restaurar padrões", "", false, 0.0f);
    }
    else if (m_category == Audio)
    {
        const int section = std::max(0, std::min(m_selectedSection[Audio], 1));
        if (section == 0)
        {
            drawOption("Volume geral", "", true, GetNexionMasterVolume());
            drawOption("Volume da música", "", true, GetNexionMusicVolume());
            drawOption("Restaurar padrões", "", false, 0.0f);
        }
        else
        {
            const int autoTune = ReadNativeSetting(kSettingRadioAutoTune, 0, 1, 1);
            const int station = ReadNativeSetting(kSettingRadioStation, 1, 12, 12);
            drawOption("Rádio auto-tune", EnabledLabel(autoTune), false, 0.0f);
            drawOption("Volume", "", true, GetNexionRadioVolume());
            drawOption("Estação de rádio", RadioStationLabel(station), false, 0.0f);
            drawOption("Restaurar padrões", "", false, 0.0f);
        }
    }
    else
    {
        const int section = std::max(0, std::min(m_selectedSection[Settings], 1));
        if (section == 0)
        {
            const int textureQuality = ReadNativeSetting(kSettingVisualPreset, 0, 3, 1);
            const int brightness = ReadNativeSetting(kSettingBrightness, 0, 100, 60);
            const int distance = ReadNativeSetting(kSettingDrawDistance, 0, 100, 70);
            const int shadows = ReadNativeSetting(kSettingShadowQuality, 0, 2, 1);
            const int reflection = ReadNativeSetting(kSettingVehicleReflection, 0, 3, 1);
            const int renderScale = ReadNativeSetting(kSettingRenderScale, 0, 100, 65);
            const int frameLimit = pSettings ? pSettings->Get().iFPSCount : 120;
            drawOption("Brilho da tela", "", true, brightness / 100.0f);
            drawOption("Efeitos visuais", VisualEffectsLabel(textureQuality), false, 0.0f);
            drawOption("Taxa de quadros", FrameRateLabel(frameLimit), false, 0.0f);
            drawOption("Efeitos de partículas", EnabledLabel(GetNexionParticleEffectsEnabled()), false, 0.0f);
            drawOption("Névoa", EnabledLabel(GetNexionDistanceFogEnabled()), false, 0.0f);
            const int trails = ReadNativeSetting(kSettingTrails, 0, 1, GetNexionSkidmarksEnabled() ? 1 : 0);
            drawOption("Marcas de derrapagem", EnabledLabel(trails), false, 0.0f);
            drawOption("Distância de desenho", DistanceLabel(distance), false, 0.0f);
            drawOption("Qualidade das sombras", ShadowLabel(shadows), false, 0.0f);
            drawOption("Qualidade do reflexo dos veículos", QualityLabel(reflection), false, 0.0f);
            drawOption("Escala de renderização", "", true, renderScale / 100.0f);
            drawOption("Restaurar padrões", "", false, 0.0f);

        }
        else if (section == 1)
        {
            const int renderScale = ReadNativeSetting(kSettingRenderScale, 0, 100, 65);
            char resolution[40]{};
            std::snprintf(resolution, sizeof(resolution), "%dx%d (%d%%)",
                          static_cast<int>(std::lround(layout.display.x * renderScale / 100.0f)),
                          static_cast<int>(std::lround(layout.display.y * renderScale / 100.0f)),
                          renderScale);
            const bool showFps = pSettings && pSettings->Get().iFPSCounter;
            drawOption("Proporção", AspectRatioLabel(m_screenAspect), false, 0.0f);
            drawOption("Resolução", resolution, false, 0.0f);
            drawOption("Mostrar FPS", showFps ? "Ativado" : "Desativado", false, 0.0f);
            drawOption("Restaurar padrões", "", false, 0.0f);
        }
        else
        {
            drawOption("Exibir bate-papo", "Ativado", false, 0.0f);
            drawOption("Tamanho do texto", "Normal", false, 0.0f);
            drawOption("Opacidade", "100%", false, 0.0f);
            drawOption("Tempo das mensagens", "5 segundos", false, 0.0f);
            drawOption("Restaurar padrões", "", false, 0.0f);
        }
    }
}

bool NexionSettingsMenu::updateNativeSensitivity(const ImVec2& pos)
{
    if (m_category != Controls)
    {
        return false;
    }

    const LayoutMetrics layout = buildLayout();
    const float optionMinX = layout.navigationMax.x + 1.0f * layout.scale;
    const float sensitivityRowMin = layout.contentMin.y + 2.0f * layout.scale + 5.0f * layout.rowHeight;
    const float sensitivityRowMax = sensitivityRowMin + layout.rowHeight;
    const float sliderMin = layout.contentMax.x - 145.0f * layout.scale;
    const float sliderMax = layout.contentMax.x - 15.0f * layout.scale;

    if (pos.x < optionMinX || pos.y < sensitivityRowMin || pos.y > sensitivityRowMax ||
        pos.x < sliderMin || pos.x > sliderMax)
    {
        return false;
    }

    const float fraction = std::max(0.0f, std::min((pos.x - sliderMin) / (sliderMax - sliderMin), 1.0f));
    const int sensitivity = static_cast<int>(std::lround(20.0f + fraction * 25.0f));
    return WriteNativeSetting(kSettingAnalogSensitivity, 20, 45, sensitivity);
}

bool NexionSettingsMenu::updateCameraAudioSlider(const ImVec2& pos)
{
    if (m_category != Game && m_category != Audio)
    {
        return false;
    }

    const LayoutMetrics layout = buildLayout();
    const float optionMinX = layout.navigationMax.x + layout.scale;
    const float sliderMin = layout.contentMax.x - 145.0f * layout.scale;
    const float sliderMax = layout.contentMax.x - 15.0f * layout.scale;
    if (pos.x < optionMinX || pos.x < sliderMin || pos.x > sliderMax)
    {
        return false;
    }

    const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / layout.rowHeight);
    const float fraction = std::max(0.0f, std::min((pos.x - sliderMin) / (sliderMax - sliderMin), 1.0f));
    const int percent = std::max(1, std::min(100, static_cast<int>(std::lround(fraction * 100.0f))));

    if (m_category == Game && (row == 1 || row == 2))
    {
        m_activeCameraAudioSlider = row;
        if (row == 1)
        {
            WriteNativeSetting(kSettingCameraSensitivityX, 1, 100, percent);
            WriteNativeSetting(kSettingCameraSensitivityY, 1, 100, percent);
            SetNexionGeneralCameraSensitivity(percent);
        }
        else
        {
            SetNexionAimCameraSensitivity(percent);
        }
        return true;
    }

    if (m_category == Audio)
    {
        const int section = m_selectedSection[Audio];
        if (section == 0 && (row == 0 || row == 1))
        {
            m_activeCameraAudioSlider = row;
            if (row == 0)
            {
                WriteNativeSetting(kSettingEffectsVolume, 0, 100, percent);
                SetNexionMasterVolume(percent / 100.0f);
            }
            else
            {
                WriteNativeSetting(kSettingMusicVolume, 0, 100, percent);
                SetNexionMusicVolume(percent / 100.0f);
            }
            return true;
        }

        if (section == 1 && row == 1)
        {
            m_activeCameraAudioSlider = row;
            SetNexionRadioVolume(percent / 100.0f);
            return true;
        }
    }

    return false;
}

bool NexionSettingsMenu::updateGraphicsValueSlider(const ImVec2& pos)
{
    if (m_category != Settings || m_selectedSection[Settings] != 0)
    {
        return false;
    }

    const LayoutMetrics layout = buildLayout();
    const float optionMinX = layout.navigationMax.x + layout.scale;
    const float sliderMin = layout.contentMax.x - 145.0f * layout.scale;
    const float sliderMax = layout.contentMax.x - 15.0f * layout.scale;
    if (pos.x < optionMinX || pos.x < sliderMin || pos.x > sliderMax)
    {
        return false;
    }

    const float graphicsRowHeight = 45.0f * layout.scale;
    const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / graphicsRowHeight);
    if (row != 0 && row != 9)
    {
        return false;
    }

    const float fraction = std::max(0.0f, std::min((pos.x - sliderMin) / (sliderMax - sliderMin), 1.0f));
    m_activeGraphicsSlider = row;
    return row == 0
        ? WriteNativeSetting(kSettingBrightness, 0, 100, static_cast<int>(std::lround(fraction * 100.0f)))
        : WriteNativeSetting(kSettingRenderScale, 0, 100, static_cast<int>(std::lround(fraction * 100.0f)));
}

void NexionSettingsMenu::updateGraphicsScroll(const ImVec2& pos)
{
    if (m_category != Settings || m_selectedSection[Settings] != 0)
    {
        return;
    }

    const LayoutMetrics layout = buildLayout();
    const float movement = m_touchStart.y - pos.y;
    const int rowDelta = static_cast<int>(std::lround(movement / layout.rowHeight));
    // Doze opções e espaço para sete linhas visíveis: sobram cinco passos.
    m_optionScroll[Settings][0] = std::max(0, std::min(m_graphicsScrollStart + rowDelta, 5));
}

void NexionSettingsMenu::handleTap(const ImVec2& pos)
{
    const LayoutMetrics layout = buildLayout();
    const int activeCategory = static_cast<int>(m_category);

    if (ContainsPoint(pos, layout.exitMin, layout.exitMax))
    {
        hide();
        return;
    }

    for (int index = 0; index < CategoryCount; ++index)
    {
        if (ContainsPoint(pos, layout.tabMin[index], layout.tabMax[index]))
        {
            const Category selected = static_cast<Category>(index);
            if (selected == Map)
            {
                hide();
                NexionMiniMap::OpenLargeMap();
                return;
            }
            selectCategory(selected);
            return;
        }
    }

    int sectionCount = 0;
    SectionLabels(m_category, sectionCount);
    const float navTop = layout.navigationMin.y + 2.0f * layout.scale;
    const float navigationRowHeight = 44.0f * layout.scale;
    const float navigationGap = 7.0f * layout.scale;
    for (int section = 0; section < sectionCount; ++section)
    {
        const ImVec2 rowMin(layout.navigationMin.x,
                            navTop + section * (navigationRowHeight + navigationGap));
        const ImVec2 rowMax(layout.navigationMax.x, rowMin.y + navigationRowHeight);
        if (ContainsPoint(pos, rowMin, rowMax))
        {
            m_selectedSection[activeCategory] = section;
            return;
        }
    }

    const ImVec2 optionMin(layout.navigationMax.x + 1.0f * layout.scale, layout.contentMin.y);
    const ImVec2 optionMax(layout.contentMax.x, layout.contentMax.y);
    const bool insideOptions = ContainsPoint(pos, optionMin, optionMax);

    if (m_category == Settings && m_selectedSection[Settings] == 0 && insideOptions)
    {

        const float graphicsRowHeight = 45.0f * layout.scale;
        const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / graphicsRowHeight);
        const float sliderMin = layout.contentMax.x - 145.0f * layout.scale;
        const float sliderMax = layout.contentMax.x - 15.0f * layout.scale;
        const float fraction = std::max(0.0f, std::min((pos.x - sliderMin) / (sliderMax - sliderMin), 1.0f));
        switch (row)
        {
            case 0:
                if (pos.x >= sliderMin) WriteNativeSetting(kSettingBrightness, 0, 100, static_cast<int>(std::lround(fraction * 100.0f)));
                break;
            case 1:
                ApplyVisualEffects((ReadNativeSetting(kSettingVisualPreset, 0, 3, 1) + 1) % 4);
                break;
            case 2:
                if (pSettings) pSettings->GetWrite().iFPSCount = NextFrameRateLimit(pSettings->Get().iFPSCount);
                break;
            case 3:
                SetNexionParticleEffectsEnabled(!GetNexionParticleEffectsEnabled());
                break;
            case 4:
                SetNexionDistanceFogEnabled(!GetNexionDistanceFogEnabled());
                break;
            case 5:
            {
                const int next = 1 - ReadNativeSetting(kSettingTrails, 0, 1, GetNexionSkidmarksEnabled() ? 1 : 0);
                WriteNativeSetting(kSettingTrails, 0, 1, next);
                SetNexionSkidmarksEnabled(next != 0);
                break;
            }
            case 6:
            {
                const int current = ReadNativeSetting(kSettingDrawDistance, 0, 100, 70);
                const int next = current < 35 ? 50 : current < 65 ? 75 : current < 90 ? 100 : 25;
                ApplyDrawDistance(next);
                break;
            }
            case 7:
                WriteNativeSetting(kSettingShadowQuality, 0, 2, (ReadNativeSetting(kSettingShadowQuality, 0, 2, 1) + 1) % 3);
                break;
            case 8:
                WriteNativeSetting(kSettingVehicleReflection, 0, 3, (ReadNativeSetting(kSettingVehicleReflection, 0, 3, 1) + 1) % 4);
                break;
            case 9:
                if (pos.x >= sliderMin) WriteNativeSetting(kSettingRenderScale, 0, 100, static_cast<int>(std::lround(fraction * 100.0f)));
                break;
            case 10:
                ApplyVisualEffects(1);
                RestoreNativeSetting(kSettingRenderScale, 0, 100, 65);
                RestoreNativeSetting(kSettingDrawDistance, 0, 100, 70);
                RestoreNativeSetting(kSettingShadowQuality, 0, 2, 1);
                RestoreNativeSetting(kSettingVehicleReflection, 0, 3, 1);
                RestoreNativeSetting(kSettingBrightness, 0, 100, 60);
                if (pSettings) pSettings->GetWrite().iFPSCount = 120;
                SetNexionParticleEffectsEnabled(true);
                SetNexionDistanceFogEnabled(true);
                WriteNativeSetting(kSettingTrails, 0, 1, 1);
                SetNexionSkidmarksEnabled(true);
                break;
            default:
                break;
        }
        return;
    }

    if (m_category == Settings && m_selectedSection[Settings] == 1 && ContainsPoint(pos, optionMin, optionMax))
    {
        const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / layout.rowHeight);
        if (row == 0)
        {
            m_screenAspect = (m_screenAspect + 1) % 5;
            ApplyAspectRatio(m_screenAspect);
        }
        else if (row == 1)
        {
            const int current = ReadNativeSetting(kSettingRenderScale, 0, 100, 65);
            const int selectedScale = NextResolutionScale(current);
            if (WriteNativeSetting(kSettingRenderScale, 0, 100, selectedScale) && pSettings)
            {
                // A resolução é persistida no momento da escolha, sem depender
                // do fechamento do menu ou do encerramento do aplicativo.
                pSettings->SaveNexionMenuSettings();
            }
        }
        else if (row == 2 && pSettings)
        {
            stSettings& settings = pSettings->GetWrite();
            settings.iFPSCounter = !settings.iFPSCounter;
        }
        else if (row == 3)
        {
            m_screenAspect = 0;
            ApplyAspectRatio(m_screenAspect);
            RestoreNativeSetting(kSettingRenderScale, 0, 100, 65);
            if (pSettings)
            {
                pSettings->GetWrite().iFPSCounter = false;
            }
        }
        return;
    }

    if (m_category == Game && ContainsPoint(pos, optionMin, optionMax))
    {
        const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / layout.rowHeight);
        if (row == 0)
        {
            const int current = ReadNativeSetting(kSettingCameraHeight, 0, 1, 0);
            WriteNativeSetting(kSettingCameraHeight, 0, 1, 1 - current);
        }
        else if (row == 3)
        {
            RestoreNativeSetting(kSettingCameraHeight, 0, 1, 0);
            RestoreNativeSetting(kSettingCameraSensitivityX, 1, 100, 40);
            RestoreNativeSetting(kSettingCameraSensitivityY, 1, 100, 40);
            SetNexionGeneralCameraSensitivity(40);
            SetNexionAimCameraSensitivity(40);
        }
        return;
    }

    if (m_category == Audio && ContainsPoint(pos, optionMin, optionMax))
    {
        const int section = std::max(0, std::min(m_selectedSection[Audio], 1));
        const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / layout.rowHeight);
        if (section == 0)
        {
            if (row == 2)
            {
                RestoreNativeSetting(kSettingEffectsVolume, 0, 100, 80);
                RestoreNativeSetting(kSettingMusicVolume, 0, 100, 80);
                SetNexionMasterVolume(0.80f);
                SetNexionMusicVolume(0.80f);
            }
        }
        else
        {
            if (row == 0)
            {
                const int autoTune = 1 - ReadNativeSetting(kSettingRadioAutoTune, 0, 1, 1);
                WriteNativeSetting(kSettingRadioAutoTune, 0, 1, autoTune);
                SetNexionRadioAutoTune(autoTune != 0);
            }
            else if (row == 2)
            {
                const int current = ReadNativeSetting(kSettingRadioStation, 1, 12, 12);
                const int next = current >= 12 ? 1 : current + 1;
                WriteNativeSetting(kSettingRadioStation, 1, 12, next);
                SetNexionRadioStation(next);
            }
            else if (row == 3)
            {
                RestoreNativeSetting(kSettingRadioAutoTune, 0, 1, 1);
                RestoreNativeSetting(kSettingRadioStation, 1, 12, 12);
                SetNexionRadioAutoTune(true);
                SetNexionRadioVolume(0.80f);
                SetNexionRadioStation(12);
            }
        }
        return;
    }

    if (m_category == Controls && ContainsPoint(pos, optionMin, optionMax))
    {
        const int row = static_cast<int>((pos.y - (layout.contentMin.y + 2.0f * layout.scale)) / layout.rowHeight);
        switch (row)
        {
            case 0:
                m_controlEditorOpen = true;
                m_controlEditorContext = NexionControls::kEditorContextOnFoot;
                m_controlEditorSelected = -1;
                m_controlEditorDragging = false;
                m_controlEditorOpacityDragging = false;
                m_controlEditorSizeDragging = false;
                return;
            case 1:
            {
                const int next = (ReadNativeSetting(kSettingSteeringMode, 0, 2, 0) + 1) % 3;
                WriteNativeSetting(kSettingSteeringMode, 0, 2, next);
                break;
            }
            case 2:
                WriteNativeSetting(kSettingAccelerometer, 0, 1,
                                   1 - ReadNativeSetting(kSettingAccelerometer, 0, 1, 0));
                break;
            case 3:
                WriteNativeSetting(kSettingInvertLook, 0, 1,
                                   1 - ReadNativeSetting(kSettingInvertLook, 0, 1, 0));
                break;
            case 4:
                WriteNativeSetting(kSettingBikeAnalog, 0, 1,
                                   1 - ReadNativeSetting(kSettingBikeAnalog, 0, 1, 0));
                break;
            case 6:
                WriteNativeSetting(kSettingAutoClimb, 0, 1,
                                   1 - ReadNativeSetting(kSettingAutoClimb, 0, 1, 0));
                break;
            case 7:
                WriteNativeSetting(kSettingTargetingMode, 0, 1,
                                   1 - ReadNativeSetting(kSettingTargetingMode, 0, 1, 0));
                break;
            case 8:
                RestoreNativeSetting(kSettingTouchLayout, 0, 1, 0);
                RestoreNativeSetting(kSettingSteeringMode, 0, 2, 0);
                RestoreNativeSetting(kSettingAccelerometer, 0, 1, 0);
                RestoreNativeSetting(kSettingInvertLook, 0, 1, 0);
                RestoreNativeSetting(kSettingBikeAnalog, 0, 1, 0);
                RestoreNativeSetting(kSettingAnalogSensitivity, 20, 45, 27);
                RestoreNativeSetting(kSettingAutoClimb, 0, 1, 0);
                RestoreNativeSetting(kSettingTargetingMode, 0, 1, 0);
                break;
            default:
                break;
        }
        return;
    }

    if (m_category == Map)
    {
        const float secondRowMin = layout.contentMin.y + 2.0f * layout.scale + layout.rowHeight;
        const float secondRowMax = secondRowMin + layout.rowHeight;
        const float sliderMin = layout.contentMax.x - 145.0f * layout.scale;
        const float sliderMax = layout.contentMax.x - 15.0f * layout.scale;
        if (pos.y >= secondRowMin && pos.y <= secondRowMax && pos.x >= sliderMin && pos.x <= sliderMax)
        {
            const float percentage = std::max(0.0f, std::min((pos.x - sliderMin) / (sliderMax - sliderMin), 1.0f));
            NexionMiniMap::SetMiniMapBlipVisibilityRange(200.0f + percentage * 800.0f);
        }
    }
}

void NexionSettingsMenu::touchEvent(const ImVec2& pos, TouchType type)
{
    if (!visible()) return;
    if (m_controlEditorOpen)
    {
        touchControlEditor(pos, type);
        return;
    }

    if (type == TouchType::push)
    {
        m_touchActive = true;
        m_touchStart = pos;
        m_draggingNativeSensitivity = updateNativeSensitivity(pos);
        m_draggingCameraAudioSlider = updateCameraAudioSlider(pos);
        m_draggingGraphicsValue = updateGraphicsValueSlider(pos);
        m_activeCameraAudioSlider = m_draggingCameraAudioSlider ? m_activeCameraAudioSlider : -1;
        m_activeGraphicsSlider = m_draggingGraphicsValue ? m_activeGraphicsSlider : -1;
        m_trackingGraphicsScroll = false;
        m_draggingGraphicsScroll = false;

        return;
    }

    if (type == TouchType::move)
    {
        if (m_draggingNativeSensitivity)
        {
            updateNativeSensitivity(pos);
            return;
        }
        if (m_draggingCameraAudioSlider)
        {
            updateCameraAudioSlider(pos);
            return;
        }
        if (m_draggingGraphicsValue)
        {
            updateGraphicsValueSlider(pos);
            return;
        }
        if (m_trackingGraphicsScroll)
        {
            const ImVec2 delta = pos - m_touchStart;
            if (delta.x * delta.x + delta.y * delta.y > 400.0f)
            {
                m_draggingGraphicsScroll = true;
            }
            updateGraphicsScroll(pos);
            return;
        }
    }

    if (type == TouchType::pop)
    {
        if (m_draggingNativeSensitivity)
        {
            updateNativeSensitivity(pos);
            m_draggingNativeSensitivity = false;
            m_touchActive = false;
            captureAndSaveSettings();
            return;
        }
        if (m_draggingCameraAudioSlider)
        {
            updateCameraAudioSlider(pos);
            m_draggingCameraAudioSlider = false;
            m_activeCameraAudioSlider = -1;
            m_touchActive = false;
            captureAndSaveSettings();
            return;
        }
        if (m_draggingGraphicsValue)
        {
            updateGraphicsValueSlider(pos);
            m_draggingGraphicsValue = false;
            m_activeGraphicsSlider = -1;
            m_touchActive = false;
            captureAndSaveSettings();
            return;
        }

        if (m_trackingGraphicsScroll)
        {
            if (m_draggingGraphicsScroll)
            {
                updateGraphicsScroll(pos);
            }
            m_trackingGraphicsScroll = false;
            m_draggingGraphicsScroll = false;
        }

        const ImVec2 delta = pos - m_touchStart;
        const float travelSquared = delta.x * delta.x + delta.y * delta.y;
        const bool wasTap = m_touchActive && travelSquared <= 400.0f;
        m_touchActive = false;
        if (wasTap)
        {
            handleTap(pos);
            captureAndSaveSettings();
        }
    }
}
