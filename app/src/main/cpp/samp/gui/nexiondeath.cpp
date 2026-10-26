#include "nexiondeath.h"

#include "../main.h"
#include "../game/RW/RenderWare.h"
#include "../game/game.h"
#include "../game/Camera.h"
#include "../game/playerped.h"
#include "../net/netgame.h"
#include "../net/playerpool.h"
#include "../net/localplayer.h"
#include "../vendor/imgui/imgui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

extern CGame* pGame;
extern CNetGame* pNetGame;

namespace
{
    // O GM (GmNexion/gamemodes/GM.pwn) reanima o jogador 3 segundos após a morte
    // (OnPlayerDeath -> SetTimerEx("RespawnPlayer", 3000)). A contagem da tela
    // acompanha esse tempo para o jogador ver a reanimação chegando.
    constexpr uint32_t kDeathRespawnMs = 3000;

    // Sprite do desmaio (pato tonto 3D) — imagem RGBA 1024x1024 com fundo
    // transparente, copiada para SAMP/death/death_monkey.rgba (o nome do arquivo
    // é mantido por compatibilidade, mas o conteúdo é o sprite atual).
    constexpr int kMonkeyTextureWidth = 1024;
    constexpr int kMonkeyTextureHeight = 1024;

    // Azul de destaque da tela de morte (DeepSkyBlue #00BFFF = 0, 191, 255).
    constexpr unsigned char kAccentR = 0;
    constexpr unsigned char kAccentG = 191;
    constexpr unsigned char kAccentB = 255;

    uint32_t g_deathStartTick = 0;
    bool g_deathActive = false;
    bool g_respawnRequested = false;
    bool g_hasKiller = false;
    char g_reasonText[96] = {0};
    char g_killerName[32] = {0};

    // Textura do sprite de desmaio carregada sob demanda do storage (SAMP/death),
    // no mesmo pipeline RenderWare usado pelos botões do HUD (nexioncontrols.cpp).
    RwTexture* g_monkeyTexture = nullptr;
    uint32_t g_monkeyNextAttempt = 0;

    // Câmera de morte fixa: de cima do jogador (visual clássico), olhando direto
    // para baixo, com altura confortável (zoom) para não ficar colada no corpo
    // caído. A pose é calculada no LockCamera — estática, sem órbita/balanço.

    // Último displaySize/scale usados no desenho — o hit-test do toque usa os mesmos.
    ImVec2 g_lastDisplaySize(0.0f, 0.0f);
    float g_lastScale = 1.0f;

    // Regiões dos botões "Renascer" e "Chamar resgate" (toque).
    ImVec2 g_renascerMin(0.0f, 0.0f);
    ImVec2 g_renascerMax(0.0f, 0.0f);
    ImVec2 g_resgateMin(0.0f, 0.0f);
    ImVec2 g_resgateMax(0.0f, 0.0f);

    // Mapeia o byte de motivo de morte do SA-MP (FindDeathReasonAndResponsiblePlayer)
    // para o nome da arma/causa em português.
    const char* DeathReasonName(uint8_t reason)
    {
        switch (reason)
        {
            case 0: return "Socos";
            case 1: return "Soco Inglês";
            case 2: return "Taco de Golfe";
            case 3: return "Cassetete";
            case 4: return "Faca";
            case 5: return "Taco de Beisebol";
            case 6: return "Pá";
            case 7: return "Taco de Sinuca";
            case 8: return "Katana";
            case 9: return "Motosserra";
            case 10:
            case 11: return "Dildo";
            case 12:
            case 13: return "Vibrador";
            case 14: return "Flores";
            case 15: return "Bengala";
            case 16: return "Granada";
            case 17: return "Gás Lacrimogêneo";
            case 18: return "Coquetel Molotov";
            case 22: return "Colt 45";
            case 23: return "Pistola Silenciada";
            case 24: return "Desert Eagle";
            case 25: return "Escopeta";
            case 26: return "Escopeta Serrada";
            case 27: return "Escopeta SPAS-12";
            case 28: return "Micro Uzi";
            case 29: return "MP5";
            case 30: return "AK-47";
            case 31: return "M4";
            case 32: return "Tec-9";
            case 33: return "Rifle";
            case 34: return "Sniper";
            case 35: return "Lança-Foguetes";
            case 36: return "Míssil Teleguiado";
            case 37: return "Lança-Chamas";
            case 38: return "Minigun";
            case 39: return "Explosivo";
            case 40: return "Detonador";
            case 41: return "Spray";
            case 42: return "Extintor";
            case 43: return "Câmera";
            case 44: return "Visão Noturna";
            case 45: return "Infravermelho";
            case 46: return "Paraquedas";
            case 49: return "Veículo";
            case 50: return "Atropelamento";
            case 51: return "Explosão";
            case 52: return "Drive-by";
            case 53: return "Afogamento";
            case 54: return "Queda";
            default: return "Desconhecida";
        }
    }

    bool EnsureMonkeyTexture()
    {
        if (g_monkeyTexture) return true;

        const uint32_t now = GetTickCount();
        if (now < g_monkeyNextAttempt) return false;
        g_monkeyNextAttempt = now + 2500;

        if (!g_pszStorage || !RwImageCreate || !RwImageAllocatePixels || !RwImageDestroy ||
            !RwRasterCreate || !RwRasterSetFromImage || !RwRasterDestroy || !RwTextureCreate)
            return false;

        char path[512]{};
        std::snprintf(path, sizeof(path), "%sSAMP/death/death_monkey.rgba", g_pszStorage);
        std::FILE* file = std::fopen(path, "rb");
        if (!file)
        {
            FLog("NexionDeath: asset ausente: %s", path);
            return false;
        }

        RwImage* image = RwImageCreate(kMonkeyTextureWidth, kMonkeyTextureHeight, 32);
        if (!image || !RwImageAllocatePixels(image) || !image->cpPixels)
        {
            if (image) RwImageDestroy(image);
            std::fclose(file);
            return false;
        }

        bool ok = true;
        const size_t rowBytes = static_cast<size_t>(kMonkeyTextureWidth) * 4;
        for (int row = 0; row < kMonkeyTextureHeight; ++row)
        {
            uint8_t* pixels = image->cpPixels + static_cast<size_t>(row) * image->stride;
            if (std::fread(pixels, 1, rowBytes, file) != rowBytes)
            {
                ok = false;
                break;
            }
        }
        std::fclose(file);

        if (!ok)
        {
            RwImageDestroy(image);
            FLog("NexionDeath: asset incompleto: %s", path);
            return false;
        }

        RwRaster* raster = RwRasterCreate(kMonkeyTextureWidth, kMonkeyTextureHeight, 32,
                                           rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
        if (!raster || !RwRasterSetFromImage(raster, image))
        {
            if (raster && RwRasterDestroy) RwRasterDestroy(raster);
            RwImageDestroy(image);
            return false;
        }
        RwImageDestroy(image);

        g_monkeyTexture = RwTextureCreate(raster);
        if (!g_monkeyTexture)
        {
            if (RwRasterDestroy) RwRasterDestroy(raster);
            return false;
        }

        RwTextureSetFilterMode(g_monkeyTexture, rwFILTERLINEAR);
        FLog("NexionDeath: textura do macaco carregada");
        return true;
    }

    ImVec2 TextSize(const char* text, float fontSize)
    {
        ImFont* font = ImGui::GetFont();
        if (!font || !text) return ImVec2(0.0f, 0.0f);
        return font->CalcTextSizeA(fontSize, 10000.0f, 0.0f, text);
    }

    void DrawCentered(ImDrawList* draw, const ImVec2& center, const char* text, float fontSize, ImU32 color)
    {
        const ImVec2 size = TextSize(text, fontSize);
        draw->AddText(ImGui::GetFont(), fontSize,
                      ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f), color, text);
    }

    // Linha de batimento cardíaco (ECG) que flanqueia o título "DESMAIADO".
    void DrawHeartbeat(ImDrawList* draw, const ImVec2& origin, float width, float height,
                       float thickness, ImU32 color, bool mirror)
    {
        ImVec2 points[8];
        points[0] = ImVec2(0.00f, 0.50f);
        points[1] = ImVec2(0.30f, 0.50f);
        points[2] = ImVec2(0.36f, 0.40f);
        points[3] = ImVec2(0.46f, 0.10f);   // pico para cima
        points[4] = ImVec2(0.54f, 0.90f);   // pico para baixo
        points[5] = ImVec2(0.62f, 0.42f);
        points[6] = ImVec2(0.68f, 0.50f);
        points[7] = ImVec2(1.00f, 0.50f);

        for (int i = 0; i < 8; ++i)
        {
            const float px = mirror ? 1.0f - points[i].x : points[i].x;
            points[i] = ImVec2(origin.x + px * width, origin.y + points[i].y * height);
        }
        draw->AddPolyline(points, 8, color, 0, thickness);
    }

    // Ícone de relógio (círculo + ponteiros de 12h e 3h).
    void DrawClockIcon(ImDrawList* draw, const ImVec2& center, float size, ImU32 color, float thickness)
    {
        draw->AddCircle(center, size * 0.5f, color, 24, thickness);
        draw->AddLine(center, ImVec2(center.x, center.y - size * 0.28f), color, thickness);
        draw->AddLine(center, ImVec2(center.x + size * 0.24f, center.y), color, thickness);
    }

    // Ícone de pessoa (cabeça + ombros) usado antes do "Morto por:".
    void DrawUserIcon(ImDrawList* draw, const ImVec2& center, float size, ImU32 color, float thickness)
    {
        // Cabeca: circulo.
        draw->AddCircle(ImVec2(center.x, center.y - size * 0.40f), size * 0.20f, color, 24, thickness);

        // Ombros: arco simplificado usando PathArcTo (1 chamada em vez de 18 linhas).
        const float domeRadius = size * 0.46f;
        const float domeCenterY = center.y + size * 0.26f;
        const float rx = domeRadius;
        const float ry = domeRadius * 0.7f;
        // Desenha o semiarco (ombros) com 10 segmentos.
        constexpr int kDomeSegments = 10;
        const float kPi = 3.14159265f;
        for (int i = 0; i < kDomeSegments; ++i)
        {
            const float a0 = kPi + kPi * static_cast<float>(i) / static_cast<float>(kDomeSegments);
            const float a1 = kPi + kPi * static_cast<float>(i + 1) / static_cast<float>(kDomeSegments);
            const ImVec2 p0(center.x + std::cos(a0) * rx, domeCenterY + std::sin(a0) * ry);
            const ImVec2 p1(center.x + std::cos(a1) * rx, domeCenterY + std::sin(a1) * ry);
            draw->AddLine(p0, p1, color, thickness);
        }
    }

    // Ícone de recarregar/renascer (arco circular com seta), usado no botão "Renascer".
    void DrawRefreshIcon(ImDrawList* draw, const ImVec2& center, float size, ImU32 color, float thickness)
    {
        const float radius = size * 0.5f;
        const int segments = 20;
        const float start = -3.14159265f * 0.5f;   // topo
        const float sweep = 3.14159265f * 1.7f;    // ~306° no sentido horário
        ImVec2 last;
        for (int i = 0; i <= segments; ++i)
        {
            const float a = start + sweep * i / segments;
            const ImVec2 point(center.x + std::cos(a) * radius, center.y + std::sin(a) * radius);
            if (i > 0) draw->AddLine(last, point, color, thickness);
            last = point;
        }

        // Seta no fim do arco.
        const float tipAngle = start + sweep;
        const ImVec2 tip(center.x + std::cos(tipAngle) * radius, center.y + std::sin(tipAngle) * radius);
        const float tang = tipAngle + 3.14159265f * 0.5f;
        const ImVec2 dir(std::cos(tang), std::sin(tang));
        const ImVec2 perp(-dir.y, dir.x);
        const float arrowLen = size * 0.18f;
        draw->AddLine(tip, tip - dir * arrowLen + perp * arrowLen * 0.5f, color, thickness);
        draw->AddLine(tip, tip - dir * arrowLen - perp * arrowLen * 0.5f, color, thickness);
    }

    // Ícone de telefone (fone de ouvido + bocal + arco), usado no "Chamar resgate".
    void DrawPhoneIcon(ImDrawList* draw, const ImVec2& center, float size, ImU32 color, float thickness)
    {
        const float r = size * 0.5f;
        draw->AddCircle(ImVec2(center.x - r * 0.40f, center.y - r * 0.28f), r * 0.15f, color, 16, thickness);
        draw->AddCircle(ImVec2(center.x + r * 0.40f, center.y + r * 0.28f), r * 0.15f, color, 16, thickness);

        const float arcRadius = r * 0.80f;
        const int segments = 14;
        const float startAngle = 3.14159265f * 1.04f;
        const float sweep = 3.14159265f * 0.92f;
        for (int i = 0; i < segments; ++i)
        {
            const float a0 = startAngle + i * sweep / segments;
            const float a1 = startAngle + (i + 1) * sweep / segments;
            const ImVec2 p0(center.x + std::cos(a0) * arcRadius, center.y + std::sin(a0) * arcRadius);
            const ImVec2 p1(center.x + std::cos(a1) * arcRadius, center.y + std::sin(a1) * arcRadius);
            draw->AddLine(p0, p1, color, thickness);
        }
    }

    // Fallback procedural caso a textura do sprite ainda não tenha carregado.
    void DrawMonkeyFallback(ImDrawList* draw, const ImVec2& center, float size, ImU32 accent, ImU32 dark)
    {
        draw->AddCircleFilled(center, size * 0.5f, accent, 48);
        const float eyeY = center.y - size * 0.06f;
        draw->AddCircleFilled(ImVec2(center.x - size * 0.18f, eyeY), size * 0.105f, dark, 24);
        draw->AddCircleFilled(ImVec2(center.x + size * 0.18f, eyeY), size * 0.105f, dark, 24);

        const float jawW = size * 0.40f;
        const float jawH = size * 0.14f;
        const float jawY = center.y + size * 0.20f;
        draw->AddRectFilled(ImVec2(center.x - jawW * 0.5f, jawY),
                            ImVec2(center.x + jawW * 0.5f, jawY + jawH), dark, size * 0.06f);
        const float toothThickness = std::max(1.0f, size * 0.025f);
        for (int i = -1; i <= 1; ++i)
        {
            const float tx = center.x + i * jawW * 0.26f;
            draw->AddLine(ImVec2(tx, jawY), ImVec2(tx, jawY + jawH), accent, toothThickness);
        }
    }

}

namespace NexionDeath
{
    void OnPlayerDied()
    {
        if (g_deathActive) return;
        g_deathActive = true;
        g_deathStartTick = GetTickCount();

        // Captura o motivo/killer no momento da morte. O FindPlayerPed é seguro
        // aqui porque OnPlayerDied só é chamado dentro do jogo (CLocalPlayer::Process).
        CPlayerPed* ped = pGame ? pGame->FindPlayerPed() : nullptr;
        if (ped)
        {
            uint16_t killer = INVALID_PLAYER_ID;
            const uint8_t reason = ped->FindDeathReasonAndResponsiblePlayer(&killer);
            if (killer != INVALID_PLAYER_ID && pNetGame && pNetGame->GetPlayerPool())
            {
                const char* name = pNetGame->GetPlayerPool()->GetPlayerName(killer);
                if (name && name[0])
                {
                    snprintf(g_killerName, sizeof(g_killerName), "%s", name);
                    g_hasKiller = true;
                }
            }
            snprintf(g_reasonText, sizeof(g_reasonText), "%s", DeathReasonName(reason));
        }
    }

    void LockCamera()
    {
        if (!g_deathActive) return;

        CPlayerPed* ped = pGame ? pGame->FindPlayerPed() : nullptr;
        if (!ped || !ped->m_pPed) return;

        // Câmera de morte fixa: de cima do jogador (visual clássico), olhando
        // para o corpo caído, com altura confortável (zoom). A pose é escrita em
        // todos os pontos que o render usa (CCam, matriz do CCamera e frame da
        // RwCamera) com uma base ortonormal DESTRÓGIRA (right x up = at). Uma
        // base espelhada (left-handed, right x up = -at) inverte a imagem e faz
        // a vista parecer "de baixo para cima".
        const CVector pos = ped->m_pPed->GetPosition();
        const float camHeight = 8.0f;
        const CVector source(pos.x, pos.y, pos.z + camHeight);
        const CVector right(-1.0f, 0.0f, 0.0f);
        const CVector up(0.0f, 1.0f, 0.0f);
        const CVector at(0.0f, 0.0f, -1.0f);   // direção da visão: para baixo

        CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
        CMatrix& mat = TheCamera.m_mCameraMatrix;

        // 1) Câmera ativa (CCam): modo fixo + valores fixos, caso o jogo releia daqui.
        CCam& cam = TheCamera.GetActiveCamera();
        cam.m_nMode = MODE_FIXED;
        cam.Source = source;
        cam.Front  = at;
        cam.Up     = up;

        // 2) Matriz do CCamera (a usada no cálculo do render).
        mat.m_right   = right;
        mat.m_forward = up;
        mat.m_up      = at;
        mat.m_pos     = source;

        // 3) Frame da RwCamera — a view final do render (modelling e ltm).
        if (TheCamera.m_pRwCamera)
        {
            RwFrame* frame = RwCameraGetFrame(TheCamera.m_pRwCamera);
            if (frame)
            {
                frame->modelling.right = right;
                frame->modelling.up    = up;
                frame->modelling.at    = at;
                frame->modelling.pos   = source;
                frame->ltm.right = right;
                frame->ltm.up    = up;
                frame->ltm.at    = at;
                frame->ltm.pos   = source;
            }
        }
    }

    bool IsPlayerDead()
    {
        // A tela fica ativa do momento da morte até o respawn concluído (Spawn()).
        // Não consulta o ped aqui: o GTA restaura a vida do ped no fim da sequência
        // de morte e isso não pode esconder a tela nem liberar o respawn automático.
        if (!pGame || !pNetGame || !pNetGame->GetPlayerPool()) return false;
        return g_deathActive;
    }

    bool ConsumeRespawnRequest()
    {
        const bool requested = g_respawnRequested;
        g_respawnRequested = false;
        return requested;
    }

    void Reset()
    {
        g_deathActive = false;
        g_deathStartTick = 0;
        g_respawnRequested = false;
        g_hasKiller = false;
        g_reasonText[0] = '\0';
        g_killerName[0] = '\0';
        g_renascerMin = ImVec2(0.0f, 0.0f);
        g_renascerMax = ImVec2(0.0f, 0.0f);
        g_resgateMin = ImVec2(0.0f, 0.0f);
        g_resgateMax = ImVec2(0.0f, 0.0f);

        // A câmera de morte é apenas sobrescrita por frame (LockCamera); ao
        // parar de escrever, o jogo retoma o controle normal sozinho no Spawn().
    }

    void HandleTouch(int type, int x, int y)
    {
        const ImVec2 point(static_cast<float>(x), static_cast<float>(y));

        // Botão "Renascer": só existe depois que a contagem termina.
        const bool canRespawn = g_deathActive &&
                                (GetTickCount() - g_deathStartTick) >= kDeathRespawnMs;
        if (type == 2 && canRespawn && g_renascerMax.x > g_renascerMin.x &&
            point.x >= g_renascerMin.x && point.x <= g_renascerMax.x &&
            point.y >= g_renascerMin.y && point.y <= g_renascerMax.y)
        {
            // Marca o pedido de respawn (liberado no fluxo de morte do cliente)
            // e avisa a gamemode. O comando via RPC_ServerCommand não aparece no chat.
            g_respawnRequested = true;
            if (pNetGame) pNetGame->SendChatCommand("/renascer");
            return;
        }

        // Botão "Chamar resgate".
        if (type == 2 && g_resgateMax.x > g_resgateMin.x &&
            point.x >= g_resgateMin.x && point.x <= g_resgateMax.x &&
            point.y >= g_resgateMin.y && point.y <= g_resgateMax.y)
        {
            // Envia o comando via RPC_ServerCommand: não aparece no chat.
            if (pNetGame) pNetGame->SendChatCommand("/resgate");
            return;
        }

    }

    void Draw(ImDrawList* draw, const ImVec2& displaySize)
    {
        // Só desenha enquanto a tela estiver ativa (morte detectada por
        // OnPlayerDied). A tela permanece até o respawn concluído em Spawn().
        if (!draw || displaySize.x <= 0.0f || displaySize.y <= 0.0f || !pGame) return;
        if (!g_deathActive) return;

        const float scale = std::min(displaySize.x / 1920.0f, displaySize.y / 1080.0f);
        if (scale <= 0.0f) return;
        g_lastDisplaySize = displaySize;
        g_lastScale = scale;

        const ImU32 accent = ImColor(kAccentR, kAccentG, kAccentB);
        const ImU32 accentSoft = ImColor(kAccentR, kAccentG, kAccentB, 60);
        const ImU32 dark = ImColor(18, 21, 26, 255);
        const ImU32 white = ImColor(255, 255, 255, 255);
        const ImU32 softWhite = ImColor(226, 230, 236, 255);

        // Overlay fullscreen translúcido: escuro com leve tom roxo no topo
        // (referência FiveM). Translúcido de propósito: com o restart automático
        // do GTA desativado (CGameLogic::ForceDeathRestart) e sem fade
        // (CCamera::Fade), o jogador vê o personagem caído atrás da tela e a
        // tela de morte por cima — sem apagão e sem o jogador "spawnando".
        draw->AddRectFilledMultiColor(
            ImVec2(0.0f, 0.0f), displaySize,
            ImColor(38, 28, 55, 172),
            ImColor(26, 21, 38, 180),
            ImColor(14, 16, 22, 186),
            ImColor(30, 24, 44, 178));

        const ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.40f);

        // Ícone do desmaio (textura, com fallback procedural).
        const float monkeySize = 330.0f * scale;
        EnsureMonkeyTexture();
        if (g_monkeyTexture && g_monkeyTexture->raster)
        {
            draw->AddImage(reinterpret_cast<ImTextureID>(g_monkeyTexture->raster),
                           ImVec2(center.x - monkeySize * 0.5f, center.y - monkeySize * 0.5f),
                           ImVec2(center.x + monkeySize * 0.5f, center.y + monkeySize * 0.5f));
        }
        else
        {
            DrawMonkeyFallback(draw, center, monkeySize, accent, dark);
        }

        // Cápsula "Você está" (fundo escuro, borda e texto verde-limão).
        const float capsuleFont = 20.0f * scale;
        const char* capsuleText = "Você está";
        const ImVec2 capsuleSize = TextSize(capsuleText, capsuleFont);
        const float capsuleW = capsuleSize.x + 44.0f * scale;
        const float capsuleH = capsuleSize.y + 16.0f * scale;
        const ImVec2 capsuleCenter(center.x, center.y + monkeySize * 0.5f + 40.0f * scale);
        const ImVec2 capsuleMin(capsuleCenter.x - capsuleW * 0.5f, capsuleCenter.y - capsuleH * 0.5f);
        const ImVec2 capsuleMax(capsuleCenter.x + capsuleW * 0.5f, capsuleCenter.y + capsuleH * 0.5f);
        draw->AddRectFilled(capsuleMin, capsuleMax, ImColor(10, 14, 10, 200), capsuleH * 0.5f);
        draw->AddRect(capsuleMin, capsuleMax, accent, capsuleH * 0.5f, 0, 1.5f * scale);
        DrawCentered(draw, capsuleCenter, capsuleText, capsuleFont, accent);

        // Título "DESMAIADO" com batimentos cardíacos dos dois lados.
        const float titleFont = 66.0f * scale;
        const ImVec2 titleCenter(center.x, capsuleMax.y + 52.0f * scale);
        const ImVec2 titleSize = TextSize("DESMAIADO", titleFont);
        DrawCentered(draw, titleCenter, "DESMAIADO", titleFont, accent);

        const float heartW = 92.0f * scale;
        const float heartH = 26.0f * scale;
        const float heartGap = 20.0f * scale;
        const float heartY = titleCenter.y - heartH * 0.5f;
        DrawHeartbeat(draw, ImVec2(titleCenter.x - titleSize.x * 0.5f - heartGap - heartW, heartY),
                      heartW, heartH, 2.5f * scale, accent, false);
        DrawHeartbeat(draw, ImVec2(titleCenter.x + titleSize.x * 0.5f + heartGap, heartY),
                      heartW, heartH, 2.5f * scale, accent, true);

        // "Morto por: <killer ou causa>" com ícone de pessoa à esquerda.
        char motivo[160];
        if (g_hasKiller)
            snprintf(motivo, sizeof(motivo), "Morto por: %s", g_killerName);
        else
            snprintf(motivo, sizeof(motivo), "Morto por: %s", g_reasonText);

        const float motivoFont = 24.0f * scale;
        const ImVec2 motivoSize = TextSize(motivo, motivoFont);
        const float motivoIconSize = 24.0f * scale;
        const float motivoGap = 14.0f * scale;
        const float motivoGroupW = motivoIconSize + motivoGap + motivoSize.x;
        const ImVec2 motivoCenter(center.x, titleCenter.y + 66.0f * scale);
        const float motivoGroupLeft = motivoCenter.x - motivoGroupW * 0.5f;
        DrawUserIcon(draw, ImVec2(motivoGroupLeft + motivoIconSize * 0.5f, motivoCenter.y),
                     motivoIconSize, softWhite, 2.0f * scale);
        draw->AddText(ImGui::GetFont(), motivoFont,
                      ImVec2(motivoGroupLeft + motivoIconSize + motivoGap,
                             motivoCenter.y - motivoSize.y * 0.5f),
                      softWhite, motivo);

        // Contagem regressiva antes de liberar o respawn manual. Quando acaba,
        // o contador vira o botão "Renascer" (a gamemode NÃO respawna sozinha).
        const uint32_t elapsed = GetTickCount() - g_deathStartTick;
        const float remainingMs = std::max(0.0f, static_cast<float>(kDeathRespawnMs) - static_cast<float>(elapsed));
        const float progress = std::clamp(remainingMs / static_cast<float>(kDeathRespawnMs), 0.0f, 1.0f);
        const int seconds = static_cast<int>(std::ceil(std::max(remainingMs, 0.0f) / 1000.0f));
        const bool canRespawn = remainingMs <= 0.0f;

        const float btnH = 68.0f * scale;
        const float btnFont = 24.0f * scale;
        const float btnGap = 14.0f * scale;
        const float btnIconSize = 26.0f * scale;
        const ImVec2 btnCenter(center.x, motivoCenter.y + 80.0f * scale);

        if (!canRespawn)
        {
            // Botão da contagem (fundo escuro, borda verde, relógio + texto).
            char secondsText[48];
            snprintf(secondsText, sizeof(secondsText), "%d Segundos restantes", seconds);
            const ImVec2 btnTextSize = TextSize(secondsText, btnFont);
            const float btnW = btnIconSize + btnGap + btnTextSize.x + 72.0f * scale;
            const ImVec2 btnMin(btnCenter.x - btnW * 0.5f, btnCenter.y - btnH * 0.5f);
            const ImVec2 btnMax(btnCenter.x + btnW * 0.5f, btnCenter.y + btnH * 0.5f);
            draw->AddRectFilled(btnMin, btnMax, ImColor(14, 17, 22, 235), 14.0f * scale);
            draw->AddRect(btnMin, btnMax, accent, 14.0f * scale, 0, 2.0f * scale);
            if (progress > 0.0f)
                draw->AddRectFilled(btnMin, ImVec2(btnMin.x + btnW * progress, btnMax.y),
                                    accentSoft, 14.0f * scale);

            const float btnGroupW = btnIconSize + btnGap + btnTextSize.x;
            const float btnGroupLeft = btnCenter.x - btnGroupW * 0.5f;
            DrawClockIcon(draw, ImVec2(btnGroupLeft + btnIconSize * 0.5f, btnCenter.y),
                          btnIconSize, accent, 2.0f * scale);
            draw->AddText(ImGui::GetFont(), btnFont,
                          ImVec2(btnGroupLeft + btnIconSize + btnGap, btnCenter.y - btnTextSize.y * 0.5f),
                          white, secondsText);
        }
        else
        {
            // Botão "Renascer" (verde cheio) no lugar do contador.
            const char* renascerText = "Renascer";
            const ImVec2 btnTextSize = TextSize(renascerText, btnFont);
            const float btnW = btnIconSize + btnGap + btnTextSize.x + 72.0f * scale;
            const ImVec2 btnMin(btnCenter.x - btnW * 0.5f, btnCenter.y - btnH * 0.5f);
            const ImVec2 btnMax(btnCenter.x + btnW * 0.5f, btnCenter.y + btnH * 0.5f);
            g_renascerMin = btnMin;
            g_renascerMax = btnMax;
            draw->AddRectFilled(btnMin, btnMax, accent, 14.0f * scale);

            const float btnGroupW = btnIconSize + btnGap + btnTextSize.x;
            const float btnGroupLeft = btnCenter.x - btnGroupW * 0.5f;
            DrawRefreshIcon(draw, ImVec2(btnGroupLeft + btnIconSize * 0.5f, btnCenter.y),
                            btnIconSize, dark, 2.5f * scale);
            draw->AddText(ImGui::GetFont(), btnFont,
                          ImVec2(btnGroupLeft + btnIconSize + btnGap, btnCenter.y - btnTextSize.y * 0.5f),
                          dark, renascerText);
        }

        // Botão "Chamar resgate" (canto inferior direito, verde-limão).
        const float resH = 80.0f * scale;
        const float margin = 44.0f * scale;
        const char* resText = "Chamar resgate";
        const float resFont = 26.0f * scale;
        const float resIconSize = 30.0f * scale;
        const float resGap = 12.0f * scale;
        const ImVec2 resTextSize = TextSize(resText, resFont);
        const float resW = resIconSize + resGap + resTextSize.x + 56.0f * scale;
        const ImVec2 resMin(displaySize.x - resW - margin, displaySize.y - resH - margin);
        const ImVec2 resMax(displaySize.x - margin, displaySize.y - margin);
        g_resgateMin = resMin;
        g_resgateMax = resMax;
        draw->AddRectFilled(resMin, resMax, accent, 16.0f * scale);

        const float resGroupW = resIconSize + resGap + resTextSize.x;
        const float resGroupLeft = (resMin.x + resMax.x) * 0.5f - resGroupW * 0.5f;
        const float resCenterY = (resMin.y + resMax.y) * 0.5f;
        DrawPhoneIcon(draw, ImVec2(resGroupLeft + resIconSize * 0.5f, resCenterY),
                      resIconSize, dark, 2.5f * scale);
        draw->AddText(ImGui::GetFont(), resFont,
                      ImVec2(resGroupLeft + resIconSize + resGap, resCenterY - resTextSize.y * 0.5f),
                      dark, resText);

    }
}
