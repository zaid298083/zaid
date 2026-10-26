#pragma once

struct ImDrawList;
struct ImVec2;

namespace NexionDeath
{
    // Avisa o módulo que o jogador local morreu (chamado pelo CLocalPlayer::Process
    // quando detecta a morte). Ativa a tela e captura o motivo/killer na hora.
    void OnPlayerDied();

    // Desenha a tela de morte (overlay, motivo, contagem e botões) enquanto o
    // jogador estiver desmaiado. A tela permanece ativa até o respawn de verdade
    // (Spawn()) — não depende do estado interno do ped, que o GTA restaura
    // sozinho no fim da sequência de morte.
    void Draw(ImDrawList* draw, const ImVec2& displaySize);

    // True enquanto a tela de morte estiver ativa. Usado pelo HUD e pelo hook de
    // toque para bloquear controles, minimapa e widgets enquanto a tela estiver aberta.
    bool IsPlayerDead();

    // Fixa a câmera de cima do jogador durante a morte (chamado pelo CGame::Process
    // logo DEPOIS do CCamera::Process do jogo). Usa os opcodes de câmera do jogo
    // (lock_camera_position + set_camera_position + point_camera) para posicionar
    // e apontar a câmera — o mesmo mecanismo do SA-MP. A órbita da câmera de morte
    // e a orientação da câmera com valores fixos, mantendo a cena parada atrás da
    // tela de morte até o respawn.
    void LockCamera();

    // Trata o toque enquanto a tela de morte está aberta: tocar em "Renascer"
    // envia /renascer e marca o pedido de respawn; "Chamar resgate" envia /resgate.
    void HandleTouch(int type, int x, int y);

    // True (uma única vez) quando o jogador clicou em "Renascer", liberando o
    // respawn no fluxo de morte do CLocalPlayer. O flag é consumido e zerado aqui.
    bool ConsumeRespawnRequest();

    // Zera o estado interno (desconexão, reset da UI, spawn concluído).
    void Reset();
}
