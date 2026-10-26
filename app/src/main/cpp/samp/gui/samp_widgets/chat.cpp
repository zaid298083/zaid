#include "../gui.h"
#include "../../main.h"
#include "../../game/game.h"
#include "../../net/netgame.h"
#include "../../net/playerpool.h"
#include "../../net/localplayer.h"
#include "../../net/remoteplayer.h"
#include "../../game/playerped.h"
#include <algorithm>
#include "../settings.h"
#include "java/jniutil.h"

extern UI* pUI;
extern CGame* pGame;
extern CNetGame* pNetGame;
extern CSettings* pSettings;
extern CJavaWrapper *pJavaWrapper;

Chat::Chat()
        : ListBox()
{
}

void Chat::addChatMessage(const std::string& message, const std::string& nick, const ImColor& nick_color)
{
    // O chat legado foi removido do HUD. Falas de jogadores usam addBubbleMessage().
    (void)message;
    (void)nick;
    (void)nick_color;
}

void Chat::addBubbleMessage(uint16_t playerId, const std::string& message)
{
    if (message.empty()) return;

    // Empilha as mensagens: cada fala nova aparece abaixo das anteriores.
    // Mantém no máximo 4 bolhas por jogador (remove a mais antiga).
    constexpr size_t kMaxPerPlayer = 4;
    size_t count = 0;
    for (auto it = m_playerBubbles.begin(); it != m_playerBubbles.end();)
    {
        if (it->playerId != playerId) { ++it; continue; }
        if (++count >= kMaxPerPlayer)
        {
            it = m_playerBubbles.erase(it);
            continue;
        }
        ++it;
    }

    const auto now = std::chrono::steady_clock::now();
    m_playerBubbles.push_back({ playerId, message, now + std::chrono::seconds(5) });
    this->setVisible(true);
}

void Chat::addInfoMessage(const std::string& format, ...)
{
    // Mensagens informativas do cliente e da gamemode não são exibidas no novo chat.
    (void)format;
}

void Chat::addDebugMessage(const std::string& format, ...)
{
    // Mensagens de depuração não são exibidas no novo chat.
    (void)format;
}

void Chat::addClientMessage(const std::string& message, const ImColor& color)
{
    // SendClientMessage da gamemode pertence ao chat antigo e não cria bolhas.
    (void)message;
    (void)color;
}

void Chat::keepVisible()
{
    // Duracao fixa para que a mensagem nao suma mais rapido em aparelhos com FPS maior.
    m_displayUntil = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    this->setVisible(true);
}

void Chat::addMessage(const std::string& message, const ImColor& color)
{
    // Caminho interno do chat antigo: não exibe mais avisos, textos de sistema ou da gamemode.
    (void)message;
    (void)color;
}

void Chat::addPlayerMessage(const std::string& message, const std::string& nick, const ImColor& nick_color)
{
    // O formato legado com nick não é mais usado. Falas normais usam bolhas sem nome.
    (void)message;
    (void)nick;
    (void)nick_color;
}

void Chat::openInput()
{
    if (!pUI || !pUI->keyboard()) return;

    m_inputOpen = true;
    this->setVisible(true);
    pUI->keyboard()->show(this);
}

void Chat::drawPlayerBubbles(ImGuiRenderer* renderer)
{
    if (!pUI || !pGame || !pNetGame) return;

    const auto now = std::chrono::steady_clock::now();
    m_playerBubbles.erase(std::remove_if(m_playerBubbles.begin(), m_playerBubbles.end(),
        [now](const PlayerBubble& bubble) { return now >= bubble.expiresAt; }),
        m_playerBubbles.end());

    CPlayerPool* playerPool = pNetGame->GetPlayerPool();
    if (!playerPool) return;

    const PLAYERID localPlayerId = playerPool->GetLocalPlayerID();
    const float fontSize = UISettings::fontSize() * 0.64f;
    const float paddingX = UISettings::fontSize() * 0.38f;
    const float paddingY = UISettings::fontSize() * 0.22f;

    // Quantas bolhas do mesmo jogador vieram antes desta, para empilhar abaixo.
    for (size_t i = 0; i < m_playerBubbles.size(); ++i)
    {
        const PlayerBubble& bubble = m_playerBubbles[i];
        CPlayerPed* playerPed = nullptr;
        if (bubble.playerId == localPlayerId)
        {
            CLocalPlayer* localPlayer = playerPool->GetLocalPlayer();
            if (localPlayer) playerPed = localPlayer->GetPlayerPed();
        }
        else if (playerPool->GetSlotState(bubble.playerId))
        {
            CRemotePlayer* remotePlayer = playerPool->GetAt(bubble.playerId);
            if (remotePlayer) playerPed = remotePlayer->GetPlayerPed();
        }

        // Durante spawn, teleporte ou desconexão o wrapper pode existir antes do ped GTA.
        // Só projeta a bolha quando a entidade nativa está válida e presente no mundo.
        if (!playerPed || !playerPed->m_pPed || !playerPed->m_pPed->IsAdded()) continue;

        // A matriz do ped é mais segura do que consultar um osso durante animações/teleportes.
        // A origem fica nos pés; 0.35 posiciona o centro da bolha no tronco/cintura.
        CVector waistPosition = playerPed->m_pPed->GetMatrix().ToRwMatrix().pos;
        waistPosition.z += 0.25f;

        CVector screenPosition;
        ((void (*)(CVector*, CVector*, float*, float*, bool, bool))(g_libGTASA +
            (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(&waistPosition, &screenPosition, 0, 0, false, false);
        if (screenPosition.z < 1.0f) continue;

        const ImVec2 textSize = renderer->calculateTextSize(bubble.message, fontSize);
        const float bubbleWidth = textSize.x + (paddingX * 2.0f);
        const float bubbleHeight = textSize.y + (paddingY * 2.0f);
        const float bubbleX = std::max(4.0f, std::min(screenPosition.x - (bubbleWidth * 0.5f),
                                                        pUI->width() - bubbleWidth - 4.0f));

        // Conta as bolhas do mesmo jogador já desenhadas antes desta: a primeira
        // fica na cintura e as seguintes descem, empilhadas uma abaixo da outra.
        int stackIndex = 0;
        for (size_t j = 0; j < i; ++j)
        {
            if (m_playerBubbles[j].playerId == bubble.playerId) ++stackIndex;
        }
        const float stackOffset = stackIndex * (bubbleHeight + paddingY * 0.5f);

        // Centraliza a bolha na projeção da cintura, sem deslocá-la para cima da cabeça.
        const float bubbleY = std::max(4.0f, screenPosition.y - (bubbleHeight * 0.5f) + stackOffset);

        renderer->drawRoundedRect(ImVec2(bubbleX, bubbleY), ImVec2(bubbleX + bubbleWidth, bubbleY + bubbleHeight),
                                  ImColor(0.05f, 0.06f, 0.08f, 0.84f), bubbleHeight * 0.28f, true);
        renderer->drawText(ImVec2(bubbleX + paddingX, bubbleY + paddingY),
                           ImColor(1.0f, 1.0f, 1.0f, 1.0f), bubble.message, false, fontSize);
    }
}

void Chat::draw(ImGuiRenderer* renderer)
{
    if (!pUI) return;

    drawPlayerBubbles(renderer);

    const float screenWidth = pUI->width();
    const float screenHeight = pUI->height();
    const float fontSize = UISettings::fontSize() * 0.72f;
    const float boxHeight = UISettings::fontSize() * 1.72f;
    const float boxWidth = ImMax(UISettings::fontSize() * 8.2f, screenWidth * 0.18f);
    const float boxX = (screenWidth - boxWidth) * 0.50f;
    const float keyboardOffset = (float)pUI->getKeyboardOffset();
    const float boxY = (keyboardOffset > 0)
        ? (screenHeight - keyboardOffset - boxHeight - 10.0f)
        : (screenHeight * 0.43f);

    if (m_inputOpen)
    {
        // Otimizacao: strings staticas reutilizam a capacidade da heap
        // entre frames, eliminando alocações repetidas que causam GC pauses.
        static std::string s_rawInput;
        static std::string s_caption;
        s_rawInput = pUI->keyboard()->input()->inputString();
        if (s_rawInput.empty())
        {
            s_caption = "Digite...";
        }
        else
        {
            s_caption = Encoding::cp2utf(s_rawInput);
        }
        const ImColor captionColor = s_rawInput.empty()
                ? ImColor(0.72f, 0.74f, 0.78f, 0.92f)
                : ImColor(1.0f, 1.0f, 1.0f, 1.0f);
        const ImVec2 captionSize = renderer->calculateTextSize(s_caption, fontSize);
        const ImVec2 boxMin(boxX, boxY);
        const ImVec2 boxMax(boxX + boxWidth, boxY + boxHeight);

        renderer->drawRoundedRect(boxMin, boxMax, ImColor(0.05f, 0.06f, 0.08f, 0.88f), boxHeight * 0.25f, true);
        renderer->drawText(ImVec2(boxX + UISettings::fontSize() * 0.55f,
                                  boxY + (boxHeight - captionSize.y) * 0.50f),
                           captionColor, s_caption, false, fontSize);
        return;
    }

    if (!m_lastMessage.empty() && std::chrono::steady_clock::now() < m_displayUntil)
    {
        const ImVec2 messageSize = renderer->calculateTextSize(m_lastMessage, fontSize);
        const float messageWidth = ImMax(boxWidth, messageSize.x + UISettings::fontSize() * 1.20f);
        const ImVec2 boxMin((screenWidth - messageWidth) * 0.50f, boxY);
        const ImVec2 boxMax(boxMin.x + messageWidth, boxY + boxHeight);

        renderer->drawRoundedRect(boxMin, boxMax, ImColor(0.05f, 0.06f, 0.08f, 0.82f), boxHeight * 0.25f, true);
        renderer->drawText(ImVec2(boxMin.x + (messageWidth - messageSize.x) * 0.50f,
                                  boxY + (boxHeight - messageSize.y) * 0.50f),
                           ImColor(1.0f, 1.0f, 1.0f, 1.0f), m_lastMessage, false, fontSize);
    }
    else if (m_playerBubbles.empty())
    {
        this->setVisible(false);
    }
}

void Chat::touchEvent(const ImVec2& pos, TouchType type)
{
    // As bolhas são somente visuais. A região do antigo ListBox não deve
    // receber foco, rolagem ou gestos que possam interferir na câmera.
    (void)pos;
    (void)type;
}

void Chat::activateEvent(bool active)
{
    if (!active && !m_inputOpen && m_playerBubbles.empty() &&
        (m_lastMessage.empty() || std::chrono::steady_clock::now() >= m_displayUntil))
    {
        this->setVisible(false);
    }
}

void Chat::touchPopEvent()
{
    // A área antiga do ListBox não deve mais abrir o teclado.
    // O teclado é aberto exclusivamente pelo novo botão de chat do painel.
}

void Chat::keyboardEvent(const std::string& input)
{
    m_inputOpen = false;

    if (input.length() > 0 && pNetGame)
    {
        if (input[0] == '/')
        {
            pNetGame->SendChatCommand(input.c_str());
        }
        else
        {
            CPlayerPool* playerPool = pNetGame->GetPlayerPool();
            if (playerPool)
            {
                addBubbleMessage(playerPool->GetLocalPlayerID(), Encoding::cp2utf(input));
            }
            pNetGame->SendChatMessage(input.c_str());
        }
    }
    else if (m_lastMessage.empty() && m_playerBubbles.empty())
    {
        this->setVisible(false);
    }
}

void Chat::keyboardCancelEvent()
{
    m_inputOpen = false;
    if (m_playerBubbles.empty() && (m_lastMessage.empty() || std::chrono::steady_clock::now() >= m_displayUntil))
    {
        this->setVisible(false);
    }
}
