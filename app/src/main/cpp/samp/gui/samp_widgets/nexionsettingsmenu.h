#pragma once

#include "../imguirenderer.h"
#include "../widget.h"

// 0 = mira fixa; 1 = mira livre, conforme MobileSettings[6].
int GetNexionTargetingMode();

// Menu modal nativo inspirado na tela de configurações do GTA: abas horizontais,
// jogo visível ao fundo e opções exibidas em painel translúcido.
class NexionSettingsMenu : public Widget
{
public:
    NexionSettingsMenu();

    void show();
    void hide();
    // Aplica as preferências salvas uma única vez após o GTA/SAMP concluir a inicialização.
    // Não abre nem exibe o painel.
    void applySavedSettingsOnStartup();
    bool open() { return visible(); }

    void performLayout() override;
    void draw(ImGuiRenderer* renderer) override;
    void touchEvent(const ImVec2& pos, TouchType type) override;

    void drawControlEditor(ImGuiRenderer* renderer);
    void touchControlEditor(const ImVec2& pos, TouchType type);

    enum Category
    {
        Map = 0,
        Controls,
        Game,
        Audio,
        Settings,
        CategoryCount
    };

private:
    struct LayoutMetrics
    {
        ImVec2 display{};
        ImVec2 headerMin{};
        ImVec2 headerMax{};
        ImVec2 contentMin{};
        ImVec2 contentMax{};
        ImVec2 navigationMin{};
        ImVec2 navigationMax{};
        ImVec2 exitMin{};
        ImVec2 exitMax{};
        ImVec2 tabMin[CategoryCount]{};
        ImVec2 tabMax[CategoryCount]{};
        float scale = 1.0f;
        float rowHeight = 0.0f;
    };

    LayoutMetrics buildLayout() const;
    void selectCategory(Category category);
    void handleTap(const ImVec2& pos);
    bool updateNativeSensitivity(const ImVec2& pos);
    bool updateCameraAudioSlider(const ImVec2& pos);
    bool updateGraphicsValueSlider(const ImVec2& pos);
    void updateGraphicsScroll(const ImVec2& pos);
    void restoreSavedSettings();
    void captureAndSaveSettings();

    Category m_category = Controls;
    int m_selectedSection[CategoryCount]{};
    int m_optionScroll[CategoryCount][3]{};
    bool m_touchActive = false;
    bool m_draggingNativeSensitivity = false;
    bool m_draggingCameraAudioSlider = false;
    bool m_trackingGraphicsScroll = false;
    bool m_draggingGraphicsScroll = false;
    bool m_draggingGraphicsValue = false;
    int m_activeGraphicsSlider = -1;
    int m_activeCameraAudioSlider = -1;
    int m_screenAspect = 0;
    int m_graphicsScrollStart = 0;
    bool m_savedSettingsAppliedOnStartup = false;
    ImVec2 m_touchStart{};
    bool m_controlEditorOpen = false;
    int m_controlEditorContext = 0;
    int m_controlEditorSelected = -1;
    bool m_controlEditorDragging = false;
    bool m_controlEditorOpacityDragging = false;
    bool m_controlEditorSizeDragging = false;
    ImVec2 m_controlEditorDragOffset{};
};
