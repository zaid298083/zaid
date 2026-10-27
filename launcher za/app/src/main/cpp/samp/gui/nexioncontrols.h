#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../vendor/imgui/imgui.h"

namespace NexionControls
{
    constexpr int kEditorContextOnFoot = 0;
    constexpr int kEditorContextVehicle = 1;
    constexpr int kEditorButtonCount = 17;
    constexpr int kEditorHudElementCount = 2;

    // Desenha o joystick virtual, a região de câmera e os botões contextuais.
    void Draw(ImDrawList* draw, const ImVec2& displaySize, float scale);

    // Recebe o evento bruto com o índice do ponteiro. Retorna true quando o
    // evento pertence à camada Nexion e não deve chegar aos widgets do GTA.
    bool HandleTouch(int type, int pointerIndex, int x, int y);

    // Identifica o gesto livre de câmera e mantém seu pointerIndex até o pop.
    bool ShouldForwardCameraTouch(int type, int pointerIndex, int x, int y);

    // Informa se o último evento já foi encaminhado ao widget cam_toggle original.
    bool ConsumeNativeCameraEvent();

    // Cancela dedos/ações presos ao sair do aplicativo ou ao trocar de tela.
    void Reset();

    // Mantém LocalPlayerKeys coerente com o estado dos controles customizados.
    void SyncPadState();

    // Consultas usadas pelos hooks do CPad para substituir o input nativo.
    bool Enabled();
    int16_t MoveLeftRight();
    int16_t MoveUpDown();
    int16_t VehicleLeftRight();
    int16_t VehicleUpDown();
    bool ActionHeld(int action);

    // API usada pela tela Editar controle do painel.
    const char* EditorButtonName(int buttonId);
    ImTextureID EditorButtonTexture(int buttonId);
    bool EditorButtonVisible(int context, int buttonId);
    void GetEditorButton(int context, int buttonId, const ImVec2& display,
                         ImVec2& center, float& size, float& opacity);
    void SetEditorButton(int context, int buttonId, const ImVec2& display,
                         const ImVec2& center, float size, float opacity);
    void ResetEditorContext(int context);
    void SaveEditorLayout();

    const char* EditorHudName(int elementId);
    bool EditorHudVisible(int context, int elementId);
    void GetEditorHud(int context, int elementId, const ImVec2& display,
                      ImVec2& center, float& size, float& opacity);
    void SetEditorHud(int context, int elementId, const ImVec2& display,
                      const ImVec2& center, float size, float opacity);
    void ResetEditorHudContext(int context);
}
