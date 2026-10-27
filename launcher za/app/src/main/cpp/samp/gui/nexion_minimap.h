#pragma once

#include <cstdint>

struct ImDrawList;
struct ImVec2;

namespace NexionMiniMap
{
    // Atualiza o cache dos ícones vindos diretamente dos RPCs da gamemode.
    void SetServerBlip(uint8_t id, float x, float y, float z, uint8_t type, uint32_t color, uint8_t style);
    void RemoveServerBlip(uint8_t id);
    void ClearServerBlips();

    // Desenha no mesmo ciclo de interface do GTA. Não cria View, SurfaceView ou thread Android.
    void Draw(ImDrawList* draw, const ImVec2& displaySize, float scale);

    // Mantém o radar original oculto quando o HUD Nexion está ativo.
    bool ShouldReplaceNativeRadar();

    // Abre o mapa grande nativo, centralizado e pronto para arrasto/zoom.
    void OpenLargeMap();

    // Permite ocultar os demais elementos do HUD enquanto o mapa livre estiver aberto.
    bool IsLargeMapOpen();

    // Consome o toque no minimapa e no mapa grande antes que o GTA possa abrir o
    // mapa nativo. Os valores de type seguem o hook Android: 1=soltar, 2=pressionar, 3=mover.
    // pointerIndex identifica o dedo e permite zoom por pinça com dois toques.
    bool HandleTouch(int type, int pointerIndex, int x, int y);

    // Alcance configurável da proximidade dos blips exibidos no minimapa.
    float GetMiniMapBlipVisibilityRange();
    void SetMiniMapBlipVisibilityRange(float range);
}
