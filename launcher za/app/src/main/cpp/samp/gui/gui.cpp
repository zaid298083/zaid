#include "../main.h"
#include "../main.h"
#include "../game/game.h"
#include "../game/RW/rwcore.h"
#include "../game/RW/RenderWare.h"
#include "../net/netgame.h"
#include "gui.h"
#include "../playertags.h"
#include "../net/playerbubblepool.h"
#include "vendor/str_obfuscator/str_obfuscator.hpp"
// voice
#include "../voice_new/Plugin.h"
#include "../voice_new/MicroIcon.h"
#include "../voice_new/SpeakerList.h"
#include <cmath>
#include <algorithm>
#include <ctime>
#include "../voice_new/Network.h"

#include "../gui/samp_widgets/voicebutton.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "game/Streaming.h"
#include "game/Pools.h"
#include "nexion_minimap.h"
#include "nexioncontrols.h"
#include "nexiondeath.h"
#include "../java/jniutil.h"
#include "bank_icon_data.h"
#include "CharacterCreator.h"

// RwRaster do ícone do banco (100x100, embutido como array RGBA)
static RwRaster* g_bankIconRaster = nullptr;
static bool g_bankIconLoaded = false;

static void EnsureBankIconTexture()
{
    if (g_bankIconLoaded) return;
    g_bankIconLoaded = true;

    RwImage* img = RwImageCreate(BANK_ICON_W, BANK_ICON_H, 32);
    if (!img) return;
    RwImageAllocatePixels(img);
    if (!img->cpPixels) { RwImageDestroy(img); return; }

    // Copiar pixels RGBA embutidos pro RwImage
    memcpy(img->cpPixels, BANK_ICON_RGBA, BANK_ICON_W * BANK_ICON_H * 4);

    RwInt32 w, h, d, flags;
    RwImageFindRasterFormat(img, rwRASTERTYPETEXTURE, &w, &h, &d, &flags);
    g_bankIconRaster = RwRasterCreate(w, h, d, flags);
    if (g_bankIconRaster)
        g_bankIconRaster = RwRasterSetFromImage(g_bankIconRaster, img);

    RwImageDestroy(img);
}

extern CNetGame* pNetGame;
extern CPlayerTags* pPlayerTags;
extern UI* pUI;
extern CGame* pGame;
extern CJavaWrapper* pJavaWrapper;
//RwTexture* logo = nullptr;
UI::UI(const ImVec2& display_size, const std::string& font_path)
        : Widget(), ImGuiWrapper(display_size, font_path)
{
    UISettings::Initialize(display_size);
    this->setFixedSize(display_size);

    // Valores seguros ate o primeiro estado de veiculo ser recebido.
    bEngine = 0;
    bLights = 0;
    bDoor = 0;
    bMeliage = 0;
    m_fuel = 0;
    m_hunger = 100;
    m_thirst = 100;
    bHealth = 0;

    //logo = (RwTexture*)CUtil::LoadTextureFromDB("mobile", "logo");
}

bool UI::initialize()
{
    if (!ImGuiWrapper::initialize()) return false;

    m_splashScreen = new SplashScreen();
    this->addChild(m_splashScreen);
    m_splashScreen->setFixedSize(size());
    m_splashScreen->setPosition(ImVec2(0.0f, 0.0f));
    m_splashScreen->setVisible(true);

    m_chat = new Chat();
    this->addChild(m_chat);
    m_chat->setFixedSize(UISettings::chatSize());
    m_chat->setPosition(UISettings::chatPos());
    m_chat->setItemSize(UISettings::chatItemSize());
    m_chat->setVisible(false);

    m_buttonPanel = new ButtonPanel();
    this->addChild(m_buttonPanel);
    m_buttonPanel->setFixedSize(UISettings::buttonPanelSize());
    m_buttonPanel->setPosition(UISettings::buttonPanelPos());
    m_buttonPanel->setVisible(false);

    m_voiceButton = new VoiceButton();
    this->addChild(m_voiceButton);
    m_voiceButton->setFixedSize(UISettings::buttonVoiceSize());
    m_voiceButton->setPosition(UISettings::buttonVoicePos());
    m_voiceButton->setVisible(false);

    m_spawn = new Spawn();
    this->addChild(m_spawn);
    m_spawn->setFixedSize(UISettings::spawnSize());
    m_spawn->setPosition(UISettings::spawnPos());
    m_spawn->setVisible(false);

    m_dialog = new Dialog();
    this->addChild(m_dialog);
    m_dialog->setVisible(false);
    m_dialog->setMinSize(UISettings::dialogMinSize());
    m_dialog->setMaxSize(UISettings::dialogMaxSize());

    m_keyboard = new Keyboard();
    this->addChild(m_keyboard);
    m_keyboard->setFixedSize(UISettings::keyboardSize());
    m_keyboard->setPosition(UISettings::keyboardPos());
    m_keyboard->setVisible(false);

    m_playerTabList = new PlayerTabList();
    this->addChild(m_playerTabList);
    m_playerTabList->setMinSize(UISettings::dialogMinSize());
    m_playerTabList->setMaxSize(UISettings::dialogMaxSize());
    m_playerTabList->setVisible(false);

    label = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label);

    // Adicionado por último para sempre ficar acima do HUD, chat e widgets.
    m_settingsMenu = new NexionSettingsMenu();
    this->addChild(m_settingsMenu);
    m_settingsMenu->setFixedSize(size());
    m_settingsMenu->setPosition(ImVec2(0.0f, 0.0f));
    m_settingsMenu->setVisible(false);

    // A sobreposição de diagnóstico (versão, arquitetura, pacote e horário)
    // foi removida para não ser exibida durante o jogo.

    //InitHudLogo();
    // ==== version ==== //
    //d_label = new Label("", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    //this->addChild(d_label);
    // d_label->setPosition(ImVec2(3.0, 55.0));

    return true;
}

void UI::render()
{
    ImGuiWrapper::render();

    if(pNetGame && pNetGame->GetPlayerPool() && pNetGame->GetPlayerPool()->GetLocalPlayer())
    {		bool isPlayerActive = pNetGame->GetPlayerPool()->GetLocalPlayer()->m_bIsActive;
		// Morto: os controles não atualizam o pad nem respondem ao toque;
		// a tela de morte assume a tela até a gamemode reanimar o jogador.
		// Criação de personagem: jogador congelado, sem joystick/controles.
		if (isPlayerActive && !NexionDeath::IsPlayerDead() && !CharacterCreator::IsVisible())
			NexionControls::SyncPadState();
		else
			NexionControls::Reset();
        if(m_buttonPanel->visible() != isPlayerActive)
        {
            m_buttonPanel->setVisible(isPlayerActive);
        }

        // Todas as preferências devem entrar em vigor antes de o jogador ver o
        // HUD. O menu permanece fechado; esta chamada ocorre uma vez por sessão
        // depois que GTA/SAMP terminaram de criar o jogador local.
        if (isPlayerActive && m_settingsMenu)
        {
            m_settingsMenu->applySavedSettingsOnStartup();
        }

        // O botao de voz ficava sobre o minimapa. Ele permanece oculto para
        // deixar o HUD limpo, sem alterar a inicializacao do recurso de voz.
        if (m_voiceButton->visible())
        {
            m_voiceButton->setVisible(false);
        }
    }

    // O HUD original do GTA (radar quadrado, relogio e barras) precisa ficar
    // desativado continuamente: alguns fluxos de spawn o reativam internamente.
    if (pGame && pNetGame)
    {
        pGame->DisplayHUD(false);
    }

    else
    {
        NexionControls::Reset();
        NexionDeath::Reset();
    }

    ProcessPushedTextdraws();

    if (m_bNeedClearMousePos) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(-1, -1);
        m_bNeedClearMousePos = false;
    }

    //DrawServerTexture(); // render sprite
}

void UI::shutdown()
{
    ImGuiWrapper::shutdown();
}

void UI::ShowConnectionLost()
{
    m_bConnectionLost = true;

    // NÃO pausamos o mundo com CTimer::m_UserPause: isso faz o GTA desenhar o
    // menu de pausa nativo por trás (painel cinza "quadrado" atrás dos textos,
    // visível através do overlay translúcido). O fade de morte que ocorria
    // antes era causado pelo ShutdownForGameModeRestart (SetDead), que já
    // removemos dos caminhos de perda de conexão. O overlay é opaco e o botão
    // leva ao launcher, então não há nada para congelar.
}

void UI::HideConnectionLost()
{
    m_bConnectionLost = false;
}

void UI::RestartConnectionAfterLoss()
{
    RestartGameConnection();
}

namespace
{
    enum class NexionHudIcon
    {
        Pin,
        Clock,
        Coins,
        Wallet,
        Heart,
        Shield,
        Drop,
        Engine,
        Bolt,
        Lock,
        Vehicle,
        Food
    };

    static ImU32 NexionColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
    {
        return IM_COL32(r, g, b, a);
    }

    static ImU32 NexionWithOpacity(ImU32 color, float opacity)
    {
        opacity = std::max(0.0f, std::min(1.0f, opacity));
        const unsigned char r = static_cast<unsigned char>((color >> IM_COL32_R_SHIFT) & 0xFF);
        const unsigned char g = static_cast<unsigned char>((color >> IM_COL32_G_SHIFT) & 0xFF);
        const unsigned char b = static_cast<unsigned char>((color >> IM_COL32_B_SHIFT) & 0xFF);
        const unsigned char a = static_cast<unsigned char>(((color >> IM_COL32_A_SHIFT) & 0xFF) * opacity);
        return IM_COL32(r, g, b, a);
    }

    static ImVec2 NexionTextSize(const char* text, float fontSize)
    {
        return ImGui::GetFont()->CalcTextSizeA(fontSize, 100000.0f, 0.0f, text);
    }

    static void DrawNexionCenteredText(ImDrawList* draw, const ImVec2& center, const char* text, float fontSize, ImU32 color)
    {
        const ImVec2 textSize = NexionTextSize(text, fontSize);
        draw->AddText(ImGui::GetFont(), fontSize,
                      ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
                      color, text);
    }

    static void FormatNexionMoney(int amount, char* out, size_t outSize)
    {
        const bool negative = amount < 0;
        const unsigned int value = negative ? (unsigned int)(-(long long)amount) : (unsigned int)amount;

        char digits[24];
        snprintf(digits, sizeof(digits), "%u", value);

        char grouped[32];
        int length = 0;
        while (digits[length] != '\0') ++length;

        int write = 0;
        for (int i = 0; i < length && write < (int)sizeof(grouped) - 1; ++i)
        {
            if (i > 0 && (length - i) % 3 == 0 && write < (int)sizeof(grouped) - 1)
            {
                grouped[write++] = '.';
            }
            grouped[write++] = digits[i];
        }
        grouped[write] = '\0';

        snprintf(out, outSize, "%sR$ %s,00", negative ? "-" : "", grouped);
    }

    // O texto do dinheiro so muda quando o valor do jogador muda.
    static const char* GetNexionCachedMoneyText(int amount)
    {
        static bool initialized = false;
        static int cachedAmount = 0;
        static char cachedText[48] = "R$ 0,00";

        if (!initialized || cachedAmount != amount)
        {
            cachedAmount = amount;
            initialized = true;
            FormatNexionMoney(amount, cachedText, sizeof(cachedText));
        }
        return cachedText;
    }

    // Horario do relogio do HUD em formato 24h (HH:MM).
    // Usa o horario real do dispositivo.
    static const char* GetNexionTimeText()
    {
        static char cachedText[8] = "00:00";
        static time_t lastUpdate = 0;

        time_t now = time(nullptr);
        if (now != lastUpdate)
        {
            lastUpdate = now;
            struct tm* t = localtime(&now);
            if (t)
            {
                snprintf(cachedText, sizeof(cachedText), "%02d:%02d", t->tm_hour, t->tm_min);
            }
        }
        return cachedText;
    }

    static void DrawNexionIcon(ImDrawList* draw, const ImVec2& center, float size, NexionHudIcon icon, ImU32 color)
    {
        const float h = size * 0.5f;
        const float stroke = size * 0.095f;
        const ImU32 cutout = NexionColor(18, 21, 26, 230);

        switch (icon)
        {
            case NexionHudIcon::Pin:
                draw->AddCircleFilled(ImVec2(center.x, center.y - h * 0.20f), h * 0.43f, color, 12);
                draw->AddTriangleFilled(ImVec2(center.x - h * 0.30f, center.y),
                                        ImVec2(center.x + h * 0.30f, center.y),
                                        ImVec2(center.x, center.y + h * 0.72f), color);
                draw->AddCircleFilled(ImVec2(center.x, center.y - h * 0.20f), h * 0.16f, cutout, 10);
                break;

            case NexionHudIcon::Clock:
                draw->AddCircle(center, h * 0.48f, color, 12, stroke);
                draw->AddLine(center, ImVec2(center.x, center.y - h * 0.28f), color, stroke);
                draw->AddLine(center, ImVec2(center.x + h * 0.22f, center.y + h * 0.15f), color, stroke);
                break;

            case NexionHudIcon::Coins:
                draw->AddCircleFilled(ImVec2(center.x - h * 0.20f, center.y + h * 0.18f), h * 0.30f, color, 10);
                draw->AddCircleFilled(ImVec2(center.x + h * 0.20f, center.y - h * 0.04f), h * 0.30f, color, 10);
                draw->AddCircleFilled(ImVec2(center.x + h * 0.03f, center.y - h * 0.32f), h * 0.30f, color, 10);
                draw->AddCircle(ImVec2(center.x + h * 0.03f, center.y - h * 0.32f), h * 0.13f, cutout, 8, stroke);
                break;

            case NexionHudIcon::Wallet:
                draw->AddRectFilled(ImVec2(center.x - h * 0.58f, center.y - h * 0.38f),
                                    ImVec2(center.x + h * 0.58f, center.y + h * 0.38f), color, h * 0.11f);
                draw->AddRectFilled(ImVec2(center.x + h * 0.04f, center.y - h * 0.12f),
                                    ImVec2(center.x + h * 0.72f, center.y + h * 0.24f), color, h * 0.09f);
                draw->AddCircleFilled(ImVec2(center.x + h * 0.42f, center.y + h * 0.06f), h * 0.055f, cutout, 6);
                break;

            case NexionHudIcon::Heart:
                draw->AddCircleFilled(ImVec2(center.x - h * 0.25f, center.y - h * 0.15f), h * 0.27f, color, 10);
                draw->AddCircleFilled(ImVec2(center.x + h * 0.25f, center.y - h * 0.15f), h * 0.27f, color, 10);
                draw->AddTriangleFilled(ImVec2(center.x - h * 0.50f, center.y - h * 0.04f),
                                        ImVec2(center.x + h * 0.50f, center.y - h * 0.04f),
                                        ImVec2(center.x, center.y + h * 0.58f), color);
                break;

            case NexionHudIcon::Shield:
                draw->PathLineTo(ImVec2(center.x, center.y - h * 0.57f));
                draw->PathLineTo(ImVec2(center.x + h * 0.52f, center.y - h * 0.32f));
                draw->PathLineTo(ImVec2(center.x + h * 0.37f, center.y + h * 0.36f));
                draw->PathLineTo(ImVec2(center.x, center.y + h * 0.62f));
                draw->PathLineTo(ImVec2(center.x - h * 0.37f, center.y + h * 0.36f));
                draw->PathLineTo(ImVec2(center.x - h * 0.52f, center.y - h * 0.32f));
                draw->PathFillConvex(color);
                break;

            case NexionHudIcon::Drop:
                draw->AddCircleFilled(ImVec2(center.x, center.y + h * 0.18f), h * 0.40f, color, 12);
                draw->AddTriangleFilled(ImVec2(center.x - h * 0.34f, center.y),
                                        ImVec2(center.x + h * 0.34f, center.y),
                                        ImVec2(center.x, center.y - h * 0.67f), color);
                break;

            case NexionHudIcon::Engine:
                draw->AddRectFilled(ImVec2(center.x - h * 0.56f, center.y - h * 0.30f),
                                    ImVec2(center.x + h * 0.48f, center.y + h * 0.30f), color, h * 0.08f);
                draw->AddRectFilled(ImVec2(center.x + h * 0.48f, center.y - h * 0.14f),
                                    ImVec2(center.x + h * 0.66f, center.y + h * 0.14f), color, h * 0.04f);
                draw->AddLine(ImVec2(center.x - h * 0.26f, center.y - h * 0.48f),
                              ImVec2(center.x - h * 0.26f, center.y - h * 0.30f), color, stroke);
                break;

            case NexionHudIcon::Bolt:
                draw->AddQuadFilled(ImVec2(center.x - h * 0.10f, center.y - h * 0.62f),
                                    ImVec2(center.x + h * 0.43f, center.y - h * 0.62f),
                                    ImVec2(center.x + h * 0.06f, center.y - h * 0.04f),
                                    ImVec2(center.x - h * 0.34f, center.y - h * 0.04f), color);
                draw->AddTriangleFilled(ImVec2(center.x - h * 0.06f, center.y - h * 0.02f),
                                        ImVec2(center.x + h * 0.28f, center.y - h * 0.02f),
                                        ImVec2(center.x - h * 0.34f, center.y + h * 0.63f), color);
                break;

            case NexionHudIcon::Lock:
                draw->AddRectFilled(ImVec2(center.x - h * 0.46f, center.y - h * 0.03f),
                                    ImVec2(center.x + h * 0.46f, center.y + h * 0.54f), color, h * 0.10f);
                draw->PathArcTo(ImVec2(center.x, center.y - h * 0.02f), h * 0.36f, IM_PI, 0.0f, 10);
                draw->PathStroke(color, false, stroke * 1.55f);
                draw->AddCircleFilled(ImVec2(center.x, center.y + h * 0.20f), h * 0.06f, cutout, 6);
                break;

            case NexionHudIcon::Vehicle:
                draw->AddRectFilled(ImVec2(center.x - h * 0.62f, center.y - h * 0.14f),
                                    ImVec2(center.x + h * 0.62f, center.y + h * 0.32f), color, h * 0.12f);
                draw->AddQuadFilled(ImVec2(center.x - h * 0.34f, center.y - h * 0.14f),
                                    ImVec2(center.x - h * 0.12f, center.y - h * 0.47f),
                                    ImVec2(center.x + h * 0.34f, center.y - h * 0.47f),
                                    ImVec2(center.x + h * 0.54f, center.y - h * 0.14f), color);
                draw->AddCircleFilled(ImVec2(center.x - h * 0.38f, center.y + h * 0.32f), h * 0.16f, cutout, 8);
                draw->AddCircleFilled(ImVec2(center.x + h * 0.38f, center.y + h * 0.32f), h * 0.16f, cutout, 8);
                break;

            case NexionHudIcon::Food:
                draw->AddCircleFilled(ImVec2(center.x - h * 0.08f, center.y + h * 0.05f), h * 0.45f, color, 12);
                draw->AddCircleFilled(ImVec2(center.x + h * 0.34f, center.y - h * 0.24f), h * 0.16f, color, 10);
                draw->AddLine(ImVec2(center.x - h * 0.04f, center.y - h * 0.51f),
                              ImVec2(center.x + h * 0.14f, center.y - h * 0.72f), color, stroke * 1.2f);
                break;
        }
    }

    static const char* GetNexionLocationName()
    {
        // A posicao vem da matriz do ped local. A base nao exporta o texto de
        // zona do GTA, entao o HUD usa as regioes/cidades reais do mapa SA.
        if (!pGame) return "Los Santos";
        CPlayerPed* localPed = pGame->FindPlayerPed();
        if (!localPed || !localPed->m_pPed) return "Los Santos";

        const RwMatrix matrix = localPed->m_pPed->GetMatrix().ToRwMatrix();
        const float x = matrix.pos.x;
        const float y = matrix.pos.y;

        if (x >= 44.0f && x <= 2997.0f && y >= -2893.0f && y <= -768.0f) return "Los Santos";
        if (x >= -2997.0f && x <= -1213.0f && y >= -1115.0f && y <= 1659.0f) return "San Fierro";
        if (x >= 869.0f && x <= 2997.0f && y >= 596.0f && y <= 2997.0f) return "Las Venturas";
        if (x >= -1213.0f && x <= 44.0f && y >= -768.0f && y <= 1659.0f) return "Red County";
        if (x >= -1213.0f && x <= 44.0f && y >= -2893.0f && y < -768.0f) return "Flint County";
        if (x >= -2997.0f && x < -1213.0f && y < -1115.0f) return "Whetstone";
        if (x >= -2997.0f && x < -1213.0f && y > 1659.0f) return "Tierra Robada";
        if (x >= 869.0f && x <= 2997.0f) return "Bone County";
        return "San Andreas";
    }

    // A zona e recalculada a cada oito quadros, pois ela nao muda com rapidez.
    // Isso evita leituras de matriz e classificacao geografica em todo quadro.
    static const char* GetNexionCachedLocationName()
    {
        static const char* cachedLocation = "Los Santos";
        static unsigned char framesUntilUpdate = 0;

        if (framesUntilUpdate == 0)
        {
            cachedLocation = GetNexionLocationName();
            framesUntilUpdate = 8;
        }
        --framesUntilUpdate;
        return cachedLocation;
    }

    static void DrawNexionTopPill(ImDrawList* draw, const ImVec2& pos, float width, float height,
                                  NexionHudIcon icon, const char* value, float scale)
    {
        const ImVec2 end(pos.x + width, pos.y + height);
        // Mesmo raio de borda do cartao de dinheiro, para padronizar todo o topo.
        draw->AddRectFilled(pos, end, NexionColor(17, 20, 25, 190), 9.0f * scale);
        DrawNexionIcon(draw, ImVec2(pos.x + height * 0.48f, pos.y + height * 0.5f), height * 0.42f,
                       icon, NexionColor(248, 248, 248, 255));
        draw->AddText(ImGui::GetFont(), 20.0f * scale,
                      ImVec2(pos.x + height * 0.84f, pos.y + height * 0.28f),
                      NexionColor(244, 246, 249, 255), value);
    }

    static void DrawNexionStatusCell(ImDrawList* draw, const ImVec2& pos, float size,
                                     NexionHudIcon icon, ImU32 accent, float ratio,
                                     float opacity, float /*scale*/)
    {
        const ImVec2 end(pos.x + size, pos.y + size);
        const float clamped = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);

        // O progresso preenche o cartao de baixo para cima: conforme o valor
        // cai, a parte vazia aparece no topo e desce visualmente pelo cartao.
        draw->AddRectFilled(pos, end, NexionWithOpacity(NexionColor(18, 21, 26, 207), opacity), size * 0.18f);
        if (clamped > 0.0f)
        {
            const ImVec2 fillStart(pos.x, end.y - size * clamped);
            draw->AddRectFilled(fillStart, end, NexionWithOpacity(accent, opacity * 0.74f), size * 0.18f);
        }
        // Sem percentual: o icone maior fica centralizado no proprio cartao.
        DrawNexionIcon(draw, ImVec2(pos.x + size * 0.50f, pos.y + size * 0.50f), size * 0.52f,
                       icon, NexionWithOpacity(NexionColor(250, 251, 252, 255), opacity));
    }

    static void DrawNexionPlayerStatusRow(ImDrawList* draw, const ImVec2& display, float scale,
                                          int health, int armour, int hunger, int thirst, float /*y*/)
    {
        ImVec2 groupCenter{};
        float groupSize = 0.0f;
        float groupOpacity = 1.0f;
        NexionControls::GetEditorHud(NexionControls::kEditorContextOnFoot, 0,
                                     display, groupCenter, groupSize, groupOpacity);
        const float baseCell = 56.0f * scale;
        const float baseGap = 8.0f * scale;
        const float baseWidth = baseCell * 4.0f + baseGap * 3.0f;
        const float groupScale = groupSize > 0.0f ? groupSize / baseWidth : 1.0f;
        const float cell = baseCell * groupScale;
        const float gap = baseGap * groupScale;

        // O colete (escudo) só aparece quando o jogador está com colete; sem ele,
        // os demais cartões refluem e o grupo continua centralizado (sem buraco).
        struct NexionStatusCellDef
        {
            NexionHudIcon icon;
            ImU32 accent;
            float ratio;
        };
        NexionStatusCellDef cells[4];
        int count = 0;
        cells[count++] = { NexionHudIcon::Heart, NexionColor(242, 74, 87, 255), health / 100.0f };
        if (armour > 0)
            cells[count++] = { NexionHudIcon::Shield, NexionColor(86, 145, 238, 255), armour / 100.0f };
        cells[count++] = { NexionHudIcon::Food, NexionColor(247, 177, 66, 255), hunger / 100.0f };
        cells[count++] = { NexionHudIcon::Drop, NexionColor(66, 182, 242, 255), thirst / 100.0f };

        const float rowWidth = cell * count + gap * (count - 1);
        const float startX = groupCenter.x - rowWidth * 0.5f;
        const float rowY = groupCenter.y - cell * 0.5f;
        for (int i = 0; i < count; ++i)
        {
            DrawNexionStatusCell(draw, ImVec2(startX + (cell + gap) * i, rowY), cell,
                                 cells[i].icon, cells[i].accent, cells[i].ratio, groupOpacity, scale);
        }
    }

    static float NexionVehicleRatio(int value)
    {
        if (value <= 0) return 0.0f;
        // A energia fisica do GTA costuma vir em 0..1000; blindagem personalizada em 0..100.
        const float divisor = value > 100 ? 1000.0f : 100.0f;
        const float ratio = value / divisor;
        return ratio > 1.0f ? 1.0f : ratio;
    }

    static void DrawNexionVehicleStatusRow(ImDrawList* draw, const ImVec2& display, float scale,
                                           int health, int armour, int hunger, int thirst,
                                           int vehicleEnergy, int vehicleArmour, float /*y*/)
    {
        ImVec2 groupCenter{};
        float groupSize = 0.0f;
        float groupOpacity = 1.0f;
        NexionControls::GetEditorHud(NexionControls::kEditorContextVehicle, 0,
                                     display, groupCenter, groupSize, groupOpacity);
        const float baseCell = 51.0f * scale;
        const float baseGap = 8.0f * scale;
        const float baseWidth = baseCell * 6.0f + baseGap * 5.0f;
        const float groupScale = groupSize > 0.0f ? groupSize / baseWidth : 1.0f;
        const float cell = baseCell * groupScale;
        const float gap = baseGap * groupScale;

        // Mesmo comportamento da linha a pé: o colete (escudo do jogador) só
        // aparece quando ele está com colete; sem ele, os cartões refluem e o
        // grupo continua centralizado. A blindagem do veículo (ícone de carro)
        // é independente do colete e permanece sempre visível.
        struct NexionStatusCellDef
        {
            NexionHudIcon icon;
            ImU32 accent;
            float ratio;
        };
        NexionStatusCellDef cells[6];
        int count = 0;
        cells[count++] = { NexionHudIcon::Heart, NexionColor(242, 74, 87, 255), health / 100.0f };
        if (armour > 0)
            cells[count++] = { NexionHudIcon::Shield, NexionColor(86, 145, 238, 255), armour / 100.0f };
        cells[count++] = { NexionHudIcon::Food, NexionColor(247, 177, 66, 255), hunger / 100.0f };
        cells[count++] = { NexionHudIcon::Drop, NexionColor(66, 182, 242, 255), thirst / 100.0f };
        cells[count++] = { NexionHudIcon::Bolt, NexionColor(250, 185, 54, 255), NexionVehicleRatio(vehicleEnergy) };
        cells[count++] = { NexionHudIcon::Vehicle, NexionColor(192, 202, 215, 255), NexionVehicleRatio(vehicleArmour) };

        const float rowWidth = cell * count + gap * (count - 1);
        const float startX = groupCenter.x - rowWidth * 0.5f;
        const float rowY = groupCenter.y - cell * 0.5f;
        for (int i = 0; i < count; ++i)
        {
            DrawNexionStatusCell(draw, ImVec2(startX + (cell + gap) * i, rowY), cell,
                                 cells[i].icon, cells[i].accent, cells[i].ratio, groupOpacity, scale);
        }
    }

    static void DrawNexionVehicleHud(ImDrawList* draw, const ImVec2& display, float scale,
                                     int speed, int fuel, int health, int armour, int hunger, int thirst,
                                     int vehicleEnergy, int vehicleArmour,
                                     bool engineOn, bool lightsOn, bool doorLocked)
    {
        ImVec2 panelCenter{};
        float panelWidth = 340.0f * scale;
        float panelOpacity = 1.0f;
        NexionControls::GetEditorHud(NexionControls::kEditorContextVehicle, 1, display,
                                     panelCenter, panelWidth, panelOpacity);
        if (panelWidth <= 0.0f)
        {
            panelWidth = 340.0f * scale;
            panelCenter = ImVec2(display.x - 38.0f * scale - panelWidth * 0.5f,
                                 display.y - 38.0f * scale - 124.0f * scale * 0.5f);
        }
        const float panelScale = panelWidth / 340.0f;
        const float panelHeight = 124.0f * panelScale;
        const float cellGap = 8.0f * panelScale;
        const ImVec2 panel(panelCenter.x - panelWidth * 0.5f, panelCenter.y - panelHeight * 0.5f);
        const ImVec2 panelEnd(panel.x + panelWidth, panel.y + panelHeight);

        draw->AddRectFilled(panel, panelEnd, NexionWithOpacity(NexionColor(14, 17, 22, 156), panelOpacity),
                            18.0f * panelScale);

        const ImVec2 dialCenter(panel.x + 93.0f * panelScale, panel.y + 97.0f * panelScale);
        const float dialRadius = 74.0f * panelScale;
        const float arcStart = IM_PI + 0.30f;
        const float arcEnd = 2.0f * IM_PI - 0.30f;
        const float normalisedSpeed = speed <= 0 ? 0.0f : (speed >= 240 ? 1.0f : speed / 240.0f);

        draw->PathArcTo(dialCenter, dialRadius, arcStart, arcEnd, 24);
        draw->PathStroke(NexionWithOpacity(NexionColor(226, 232, 239, 145), panelOpacity), false, 4.0f * panelScale);
        if (normalisedSpeed > 0.0f)
        {
            draw->PathArcTo(dialCenter, dialRadius, arcStart, arcStart + (arcEnd - arcStart) * normalisedSpeed, 24);
            draw->PathStroke(NexionWithOpacity(NexionColor(45, 153, 255, 255), panelOpacity), false, 4.8f * panelScale);
        }

        char speedText[8];
        snprintf(speedText, sizeof(speedText), "%03d", speed);
        DrawNexionCenteredText(draw, ImVec2(panel.x + 91.0f * panelScale, panel.y + 68.0f * panelScale),
                                speedText, 43.0f * panelScale,
                                NexionWithOpacity(NexionColor(249, 250, 252, 255), panelOpacity));
        draw->AddText(ImGui::GetFont(), 15.0f * panelScale,
                      ImVec2(panel.x + 146.0f * panelScale, panel.y + 72.0f * panelScale),
                      NexionWithOpacity(NexionColor(226, 231, 238, 235), panelOpacity), "KM/H");

        DrawNexionIcon(draw, ImVec2(panel.x + panelWidth - 34.0f * panelScale, panel.y + 30.0f * panelScale),
                       26.0f * panelScale, NexionHudIcon::Vehicle,
                       NexionWithOpacity(NexionColor(250, 250, 250, 235), panelOpacity));
        DrawNexionIcon(draw, ImVec2(panel.x + panelWidth - 72.0f * panelScale, panel.y + 28.0f * panelScale),
                       20.0f * panelScale, NexionHudIcon::Drop,
                       NexionWithOpacity(NexionColor(118, 207, 255, 255), panelOpacity));
        char fuelText[16];
        snprintf(fuelText, sizeof(fuelText), "%d%%", fuel);
        draw->AddText(ImGui::GetFont(), 17.0f * panelScale,
                      ImVec2(panel.x + panelWidth - 104.0f * panelScale, panel.y + 48.0f * panelScale),
                      NexionWithOpacity(NexionColor(245, 247, 250, 245), panelOpacity), fuelText);

        const float indicatorY = panel.y + panelHeight - 24.0f * panelScale;
        const float indicatorX = panel.x + panelWidth - 118.0f * panelScale;
        DrawNexionIcon(draw, ImVec2(indicatorX, indicatorY), 18.0f * panelScale, NexionHudIcon::Engine,
                       NexionWithOpacity(engineOn ? NexionColor(249, 185, 54, 255) : NexionColor(110, 117, 126, 255), panelOpacity));
        DrawNexionIcon(draw, ImVec2(indicatorX + 33.0f * panelScale, indicatorY), 18.0f * panelScale, NexionHudIcon::Bolt,
                       NexionWithOpacity(lightsOn ? NexionColor(187, 88, 238, 255) : NexionColor(110, 117, 126, 255), panelOpacity));
        DrawNexionIcon(draw, ImVec2(indicatorX + 66.0f * panelScale, indicatorY), 18.0f * panelScale, NexionHudIcon::Lock,
                       NexionWithOpacity(doorLocked ? NexionColor(220, 228, 237, 255) : NexionColor(110, 117, 126, 255), panelOpacity));

        DrawNexionVehicleStatusRow(draw, display, scale, health, armour, hunger, thirst,
                                   vehicleEnergy, vehicleArmour, panel.y + panelHeight + cellGap);
    }

    static void DrawNexionFistButton(ImDrawList* draw, UI* ui, const ImVec2& display, float scale,
                                     const ImVec2& anchor)
    {
        if (!draw || !ui) return;
        const float size = 56.0f * scale;
        const ImVec2 min(anchor.x, anchor.y);
        const ImVec2 max(anchor.x + size, anchor.y + size);
        const bool pressed = ui->m_fistBtnPressed;
        draw->AddRectFilled(min, max, pressed ? NexionColor(50, 56, 68, 230)
                                              : NexionColor(17, 20, 25, 205), 13.0f * scale);
        draw->AddRect(min, max, NexionColor(120, 132, 148, 180), 13.0f * scale, 0, 1.5f * scale);

        const bool holstered = ui->m_weaponHolstered && ui->m_savedWeapon != WEAPON_UNARMED;
        // Ícone de arma minimalista, desenhado no HUD para ficar nítido em
        // qualquer resolução e não depender de um PNG externo.
        const ImVec2 c(anchor.x + size * 0.5f, anchor.y + size * 0.51f);
        const float r = size * 0.20f;
        const ImU32 icon = holstered ? NexionColor(76, 213, 151, 255)
                                     : NexionColor(245, 247, 250, 255);
        draw->AddRectFilled(ImVec2(c.x - r * 1.25f, c.y - r * 0.52f),
                            ImVec2(c.x + r * 0.95f, c.y + r * 0.20f), icon, r * 0.16f);
        draw->AddRectFilled(ImVec2(c.x - r * 1.25f, c.y - r * 0.25f),
                            ImVec2(c.x + r * 1.55f, c.y - r * 0.02f), icon, r * 0.12f);
        draw->AddTriangleFilled(ImVec2(c.x - r * 0.05f, c.y + r * 0.18f),
                                ImVec2(c.x + r * 0.62f, c.y + r * 0.18f),
                                ImVec2(c.x + r * 0.28f, c.y + r * 1.30f), icon);
        draw->AddLine(ImVec2(c.x - r * 0.98f, c.y + r * 0.22f),
                      ImVec2(c.x - r * 1.48f, c.y + r * 0.75f), icon, r * 0.34f);
        draw->AddText(ImGui::GetFont(), 10.0f * scale,
                      ImVec2(anchor.x + 5.0f * scale, anchor.y + size - 15.0f * scale),
                      holstered ? NexionColor(76, 213, 151, 255)
                                : NexionColor(188, 198, 210, 255),
                      holstered ? "PEGAR" : "GUARDAR");

        ui->m_fistBtnMin = min;
        ui->m_fistBtnMax = max;
        ui->m_fistBtnVisible = true;
    }

    static void DrawNexionHud()
    {
        if (pUI) pUI->m_fistBtnVisible = false;
        if (!pUI || !pGame || !pNetGame || !pNetGame->GetPlayerPool()) return;

        // O HUD so aparece com o jogador ativo (spawnado): durante a conexao,
        // carregamento ou selecao de classe, nada de minimapa, status, dinheiro
        // ou botoes na tela.
        auto* localPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
        if (!localPlayer || !localPlayer->m_bIsActive) return;

        ImGuiIO& io = ImGui::GetIO();
        if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) return;

        float scale = io.DisplaySize.x / 1920.0f;
        const float verticalScale = io.DisplaySize.y / 1080.0f;
        if (verticalScale < scale) scale = verticalScale;
        if (scale <= 0.0f) return;

        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        NexionMiniMap::Draw(draw, io.DisplaySize, scale);

        // O mapa grande abre em modo limpo: o próprio minimapa já desenhou a
        // cartografia e o botão Fechar; as cápsulas e status não devem sobrepor o mapa.
        if (NexionMiniMap::IsLargeMapOpen()) return;

        const float margin = 38.0f * scale;
        // Dinheiro e todas as capsulas superiores usam a mesma altura e o mesmo raio.
        const float pillHeight = 49.0f * scale;
        const float pillGap = 9.0f * scale;

        CPlayerPed* playerPed = pGame->FindPlayerPed();
        int health = 0;
        int armour = 0;
        int speed = 0;
        int fuel = 0;
        int hunger = (int)pUI->m_hunger;
        int thirst = (int)pUI->m_thirst;
        bool inVehicle = false;

        if (playerPed)
        {
            // Se o servidor, veículo ou respawn equipar outra arma por fora,
            // não deixamos o botão preso no estado "PEGAR" com um item antigo.
            if (pUI->m_weaponHolstered && playerPed->GetCurrentWeapon() != WEAPON_UNARMED)
            {
                pUI->m_weaponHolstered = false;
                pUI->m_savedWeapon = WEAPON_UNARMED;
            }
            health = (int)playerPed->GetHealth();
            armour = (int)playerPed->GetArmour();

            // GetCurrentVehicle pode manter uma referencia durante a animacao de
            // saida. IsInVehicle() e o estado real do ped e remove o painel no
            // mesmo quadro em que o jogador deixa o veiculo.
            if (playerPed->IsInVehicle())
            {
                CVehicle* vehicle = playerPed->GetCurrentVehicle();
                if (vehicle && vehicle->m_pVehicle)
                {
                    CVector moveSpeed = vehicle->m_pVehicle->GetMoveSpeed();
                    speed = (int)(sqrtf((moveSpeed.x * moveSpeed.x) + (moveSpeed.y * moveSpeed.y) +
                                       (moveSpeed.z * moveSpeed.z)) * 180.0f);
                    fuel = (int)pUI->m_fuel;
                    inVehicle = true;
                }
            }
        }

        if (health < 0) health = 0;
        if (health > 100) health = 100;
        if (armour < 0) armour = 0;
        if (armour > 100) armour = 100;
        if (hunger < 0) hunger = 0;
        if (hunger > 100) hunger = 100;
        if (thirst < 0) thirst = 0;
        if (thirst > 100) thirst = 100;
        if (speed < 0) speed = 0;
        if (speed > 999) speed = 999;
        if (fuel < 0) fuel = 0;
        if (fuel > 100) fuel = 100;

        const char* cashText = GetNexionCachedMoneyText(pGame->GetLocalMoney());

        // As moedas VIP serao conectadas ao servidor em uma etapa posterior. O
        // valor inicial seguro e zero, sem reutilizar a contagem de jogadores.
        const char* vipCoinsText = "0";

        // No topo ficam somente: localizacao, horario e moedas VIP.
        const char* locationName = GetNexionCachedLocationName();
        const float locationWidth = 210.0f * scale;
        const float timeWidth = 131.0f * scale;
        const float vipWidth = 117.0f * scale;
        const float totalWidth = locationWidth + timeWidth + vipWidth + pillGap * 2.0f;
        const ImVec2 topStart(io.DisplaySize.x - margin - totalWidth, 22.0f * scale);
        DrawNexionTopPill(draw, topStart, locationWidth, pillHeight, NexionHudIcon::Pin, locationName, scale);
        DrawNexionTopPill(draw, ImVec2(topStart.x + locationWidth + pillGap, topStart.y),
                          timeWidth, pillHeight, NexionHudIcon::Clock, GetNexionTimeText(), scale);
        DrawNexionTopPill(draw, ImVec2(topStart.x + locationWidth + timeWidth + pillGap * 2.0f, topStart.y),
                          vipWidth, pillHeight, NexionHudIcon::Coins, vipCoinsText, scale);

        // Envia localização + horário pro Flutter a cada ~30 frames
        {
            static unsigned char gameDataFrameCounter = 0;
            if (++gameDataFrameCounter >= 30)
            {
                gameDataFrameCounter = 0;
                if (pJavaWrapper)
                    pJavaWrapper->SendGameDataToFlutter(locationName, GetNexionTimeText());
            }
        }

        // O dinheiro fica logo abaixo, sem banco nem informacoes adicionais.
        const float moneyWidth = 210.0f * scale;
        const ImVec2 moneyPos(io.DisplaySize.x - margin - moneyWidth, topStart.y + pillHeight + 12.0f * scale);
        draw->AddRectFilled(moneyPos, ImVec2(moneyPos.x + moneyWidth, moneyPos.y + 49.0f * scale),
                            NexionColor(17, 20, 25, 178), 10.0f * scale);
        DrawNexionIcon(draw, ImVec2(moneyPos.x + 21.0f * scale, moneyPos.y + 24.5f * scale),
                       22.0f * scale, NexionHudIcon::Wallet, NexionColor(244, 246, 248, 255));
        draw->AddText(ImGui::GetFont(), 19.0f * scale,
                      ImVec2(moneyPos.x + 39.0f * scale, moneyPos.y + 13.5f * scale),
                      NexionColor(250, 251, 252, 255), cashText);

        // Sem radar adicional sobre o minimapa: o HUD novo deixa esta regiao limpa.

        // A pe: os quatro status ficam lado a lado. No veiculo, a mesma linha
        // fica logo abaixo do velocimetro, e o painel some no instante da saida.
        if (inVehicle)
        {
            DrawNexionVehicleHud(draw, io.DisplaySize, scale, speed, fuel, health, armour, hunger, thirst,
                                  pUI->bHealth, pUI->bMeliage,
                                  pUI->bEngine != 0, pUI->bLights != 0, pUI->bDoor != 0);
        }
        else
        {
            const float cell = 56.0f * scale;
            DrawNexionPlayerStatusRow(draw, io.DisplaySize, scale, health, armour, hunger, thirst,
                                      io.DisplaySize.y - margin - cell);

            // O botão de ação/arma é desenhado e tratado pelo módulo original
            // NexionControls/SA-MP, com área de toque ampliada.
        }

        // A camada de controles é desenhada por último para ficar acima do HUD
        // informativo e não depender dos widgets touch nativos do GTA. Morto,
        // os controles ficam ocultos — a tela de morte cobre tudo.
        if (!NexionDeath::IsPlayerDead())
            NexionControls::Draw(draw, io.DisplaySize, scale);
    }
}

void UI::DrawConnectionLost()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) return;

    float scale = io.DisplaySize.x / 1920.0f;
    const float verticalScale = io.DisplaySize.y / 1080.0f;
    if (verticalScale < scale) scale = verticalScale;
    if (scale <= 0.0f) return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

    // Overlay cinza com leve gradiente vertical cobrindo a tela toda, TOTALMENTE
    // opaco: nada do GTA (menu de pausa, HUD, fade) aparece por trás dos textos.
    draw->AddRectFilledMultiColor(
        ImVec2(0.0f, 0.0f), io.DisplaySize,
        NexionColor(28, 30, 34, 255),
        NexionColor(18, 20, 24, 255),
        NexionColor(18, 20, 24, 255),
        NexionColor(28, 30, 34, 255));

    // Sem caixa ao redor do texto: ícone, título e corpo ficam soltos sobre o
    // overlay, como na referência. O botão também é só o texto (link).

    // Ícone de aviso: triângulo vermelho com contorno escuro e "!" central.
    const float iconSize = 84.0f * scale;
    const ImVec2 iconCenter(center.x, center.y - 168.0f * scale);

    // Contorno (triângulo maior por trás).
    const ImVec2 triTopO(iconCenter.x, iconCenter.y - iconSize * 0.56f);
    const ImVec2 triLeftO(iconCenter.x - iconSize * 0.62f, iconCenter.y + iconSize * 0.52f);
    const ImVec2 triRightO(iconCenter.x + iconSize * 0.62f, iconCenter.y + iconSize * 0.52f);
    draw->AddTriangleFilled(triTopO, triLeftO, triRightO, NexionColor(60, 18, 18, 255));

    // Triângulo interno (vermelho).
    const ImVec2 triTop(iconCenter.x, iconCenter.y - iconSize * 0.42f);
    const ImVec2 triLeft(iconCenter.x - iconSize * 0.47f, iconCenter.y + iconSize * 0.40f);
    const ImVec2 triRight(iconCenter.x + iconSize * 0.47f, iconCenter.y + iconSize * 0.40f);
    draw->AddTriangleFilled(triTop, triLeft, triRight, NexionColor(226, 58, 58, 255));

    // "!" no centro do triângulo.
    const char* warnText = "!";
    const float warnFont = 44.0f * scale;
    const ImVec2 warnSize = NexionTextSize(warnText, warnFont);
    draw->AddText(ImGui::GetFont(), warnFont,
                  ImVec2(iconCenter.x - warnSize.x * 0.5f, iconCenter.y - warnSize.y * 0.5f - 4.0f * scale),
                  NexionColor(255, 255, 255, 255), warnText);

    // Título.
    DrawNexionCenteredText(draw, ImVec2(center.x, center.y - 56.0f * scale),
                           "CONEXÃO PERDIDA", 38.0f * scale, NexionColor(238, 64, 64, 255));

    // Corpo do texto (3 linhas centralizadas).
    const float bodyY = center.y + 18.0f * scale;
    const float lineGap = 31.0f * scale;
    DrawNexionCenteredText(draw, ImVec2(center.x, bodyY),
                           "Sua conexão com o servidor foi", 21.0f * scale, NexionColor(226, 230, 236, 255));
    DrawNexionCenteredText(draw, ImVec2(center.x, bodyY + lineGap),
                           "encerrada inesperadamente. Reinicie o", 21.0f * scale, NexionColor(226, 230, 236, 255));
    DrawNexionCenteredText(draw, ImVec2(center.x, bodyY + lineGap * 2.0f),
                           "jogo para reconectar.", 21.0f * scale, NexionColor(226, 230, 236, 255));

    // Botão "CLIQUE AQUI PARA REINICIAR" — somente o texto (link vermelho), sem caixa.
    const char* btnText = "CLIQUE AQUI PARA REINICIAR";
    const float btnFont = 24.0f * scale;
    const ImVec2 btnSize = NexionTextSize(btnText, btnFont);
    const ImVec2 btnCenter(center.x, bodyY + lineGap * 3.0f + 32.0f * scale);
    const ImVec2 btnPadding(30.0f * scale, 20.0f * scale);
    m_connLostBtnMin = ImVec2(btnCenter.x - btnSize.x * 0.5f - btnPadding.x,
                               btnCenter.y - btnSize.y * 0.5f - btnPadding.y);
    m_connLostBtnMax = ImVec2(btnCenter.x + btnSize.x * 0.5f + btnPadding.x,
                               btnCenter.y + btnSize.y * 0.5f + btnPadding.y);

    DrawNexionCenteredText(draw, btnCenter, btnText, btnFont, NexionColor(238, 64, 64, 255));
}

void DrawNexionEditorHudPreview(ImDrawList* draw, const ImVec2& display, float scale, int context)
{
    if (!draw || display.x <= 0.0f || display.y <= 0.0f || scale <= 0.0f)
        return;

    int health = 100;
    int armour = 100;
    int speed = 0;
    int fuel = pUI ? static_cast<int>(pUI->m_fuel) : 100;
    int hunger = pUI ? static_cast<int>(pUI->m_hunger) : 100;
    int thirst = pUI ? static_cast<int>(pUI->m_thirst) : 100;
    int vehicleEnergy = 1000;
    int vehicleArmour = 100;
    bool engineOn = true;
    bool lightsOn = false;
    bool doorLocked = false;

    if (pGame)
    {
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (ped)
        {
            health = std::max(0, std::min(100, static_cast<int>(ped->GetHealth())));
            armour = std::max(0, std::min(100, static_cast<int>(ped->GetArmour())));
            CVehicle* vehicle = ped->IsInVehicle() ? ped->GetCurrentVehicle() : nullptr;
            if (vehicle && vehicle->m_pVehicle)
            {
                const CVector moveSpeed = vehicle->m_pVehicle->GetMoveSpeed();
                speed = static_cast<int>(std::sqrt(moveSpeed.x * moveSpeed.x +
                                                    moveSpeed.y * moveSpeed.y +
                                                    moveSpeed.z * moveSpeed.z) * 180.0f);
            }
        }
    }

    if (context == NexionControls::kEditorContextOnFoot)
    {
        DrawNexionPlayerStatusRow(draw, display, scale, health, armour, hunger, thirst, 0.0f);
    }
    else
    {
        DrawNexionVehicleHud(draw, display, scale, speed, fuel, health, armour, hunger, thirst,
                             vehicleEnergy, vehicleArmour, engineOn, lightsOn, doorLocked);
    }
}

void UI::drawList()
{
    if (!visible()) return;

    // A tela de conexão perdida é a camada mais externa: cobre menu, HUD e tudo mais.
    if (m_bConnectionLost)
    {
        NexionControls::Reset();
        DrawConnectionLost();
        return;
    }	// O menu de configurações é uma camada limpa: enquanto estiver aberto,
	// não desenhamos HUD, minimapa, chat, etiquetas, bolhas nem os widgets comuns.
	// Somente o cenário do GTA e o próprio menu permanecem visíveis.
	if (m_settingsMenu && m_settingsMenu->open())
	{
		m_settingsMenu->draw(renderer());
		return;
	}

	// A criação de personagem é a camada mais externa do jogo: enquanto as
	// janelas de seleção/criação estiverem abertas, nada de HUD, minimapa,
	// controles, chat, etiquetas, bolhas, textdraws ou botões 3D na tela.
	// Somente o cenário do GTA e a própria janela da criação permanecem.
	if (CharacterCreator::IsVisible())
	{
		NexionControls::Reset();
		CharacterCreator::Render();
		return;
	}

    /*Label* label;
    label = new Label("1.0.11", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    label->setPosition(ImVec2(0.0, 0.0));
    this->addChild(label);*/

    DrawNexionHud();
    renderDebug();

    // A tela de morte é a camada mais externa do jogo ativo: cobre HUD, minimapa
    // e widgets enquanto o jogador está caído, e nada mais recebe desenho/toque.
    if (NexionDeath::IsPlayerDead())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f)
            NexionDeath::Draw(ImGui::GetBackgroundDrawList(), io.DisplaySize);
        return;
    }	// Mantém a visualização do mapa grande totalmente livre de tags, bolhas,
	// controles e demais widgets do HUD.
	if (NexionMiniMap::IsLargeMapOpen()) return;

	if (pPlayerTags) pPlayerTags->Render(renderer());
    if (pNetGame && pNetGame->GetTextLabelPool()) pNetGame->GetTextLabelPool()->Render(renderer());
    if (pNetGame && pNetGame->GetPlayerBubblePool()) pNetGame->GetPlayerBubblePool()->Render(renderer());

    // ============================================================
    // BOTÃO 3D "Acessar" — renderizado no engine do jogo
    // Precisa ficar DENTRO de drawList() (entre NewFrame/EndFrame)
    // Reseta o flag ANTES do bloco: se o desenho não executar
    // (jogador longe), m_bankBtnVisible fica false.
    // ============================================================
    m_bankBtnVisible = false;
    if (pNetGame && pGame)
    {
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (ped)
        {
            const float BANK_X = 1462.0f;
            const float BANK_Y = -1015.0f;
            const float BANK_Z = 25.7f;

            CVector playerPos = ped->m_pPed->GetPosition();
            float dx = playerPos.x - BANK_X;
            float dy = playerPos.y - BANK_Y;
            float dz = playerPos.z - BANK_Z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);

            FLog("[3DBTN] player=(%.1f,%.1f,%.1f) bank=(%.1f,%.1f,%.1f) dist=%.1f",
                playerPos.x, playerPos.y, playerPos.z, BANK_X, BANK_Y, BANK_Z, dist);

            if (dist < 8.0f)
            {
                CVector worldPos(BANK_X, BANK_Y, BANK_Z + 1.2f);
                CVector screenPos;

                ((void (*)(CVector*, CVector*, float*, float*, bool, bool))
                    (g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))
                    (&worldPos, &screenPos, 0, 0, 0, 0);

                FLog("[3DBTN] screen=(%.1f,%.1f,%.1f)", screenPos.x, screenPos.y, screenPos.z);

                if (screenPos.z > 1.0f)
                {
                    float alpha = 1.0f;
                    if (dist > 2.0f)
                        alpha = (8.0f - dist) / 6.0f;
                    if (alpha < 0.0f) alpha = 0.0f;
                    if (alpha > 1.0f) alpha = 1.0f;

                    EnsureBankIconTexture();

                    ImDrawList* draw = ImGui::GetBackgroundDrawList();
                    ImVec2 center(screenPos.x, screenPos.y);

                    // Botão maior — escala proporcional à resolução
                    const float displayScale = ImGui::GetIO().DisplaySize.x / 1920.0f;
                    float btnW = 200.0f * displayScale;
                    float btnH = 64.0f * displayScale;
                    float rounding = 16.0f * displayScale;
                    ImVec2 btnMin(center.x - btnW/2, center.y - btnH/2);
                    ImVec2 btnMax(center.x + btnW/2, center.y + btnH/2);

                    // Salvar bounds pro touchEvent detectar clique
                    m_bankBtnMin = btnMin;
                    m_bankBtnMax = btnMax;
                    m_bankBtnVisible = true;

                    // --- Fundo principal (dark glass, sem borda) ---
                    ImU32 bgColor = IM_COL32(20, 22, 28, (int)(230 * alpha));
                    draw->AddRectFilled(btnMin, btnMax, bgColor, rounding);

                    // --- Ícone azul (fundo quadrado arredondado) ---
                    float pad = 10.0f * displayScale;
                    float iconBgSize = btnH - pad * 2.0f;
                    ImVec2 iconBgMin(btnMin.x + pad, btnMin.y + pad);
                    ImVec2 iconBgMax(iconBgMin.x + iconBgSize, iconBgMin.y + iconBgSize);
                    float iconRound = 10.0f * displayScale;
                    ImU32 iconBgColor = IM_COL32(56, 142, 255, (int)(230 * alpha));
                    draw->AddRectFilled(iconBgMin, iconBgMax, iconBgColor, iconRound);

                    // --- PNG do ícone (nexionl_interaction_button.png) ---
                    if (g_bankIconRaster)
                    {
                        // Padding maior pra ícone não ser cortado
                        float iconPad = 10.0f * displayScale;
                        ImVec2 iconA(iconBgMin.x + iconPad, iconBgMin.y + iconPad);
                        ImVec2 iconB(iconBgMax.x - iconPad, iconBgMax.y - iconPad);
                        // Sem tint color — usa a cor original branca do PNG
                        draw->AddImage(reinterpret_cast<ImTextureID>(g_bankIconRaster), iconA, iconB, ImVec2(0, 0), ImVec2(1, 1));
                    }

                    // --- Texto "Acessar" ---
                    const char* text = "Acessar";
                    float fontSize = 20.0f * displayScale;
                    ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, 100000.0f, 0.0f, text);
                    float textX = iconBgMax.x + 12.0f * displayScale;
                    float textY = center.y - textSize.y / 2.0f;
                    ImU32 textColor = IM_COL32(255, 255, 255, (int)(255 * alpha));
                    draw->AddText(ImGui::GetFont(), fontSize, ImVec2(textX, textY), textColor, text);

                    // --- Pressionado (highlight) ---
                    if (m_bankBtnPressed)
                    {
                        draw->AddRectFilled(btnMin, btnMax, IM_COL32(56, 142, 255, (int)(50 * alpha)), rounding);
                    }
                }
            }
        }
    }

    // drawList() roda a cada frame. m_bankBtnVisible só é setado
    // true dentro do bloco de desenho 3DBTN. Se o bloco não executou
    // (jogador longe, sem ped, etc.) o flag continua false e os bounds
    // são zerados — touchEvent não vai detectar nada.
    // NÃO resetamos m_bankBtnVisible aqui embaixo porque senão ele
    // estaria sempre false quando o toque chega (toque e render são
    // chamadas diferentes).

    draw(renderer());
}

void UI::touchEvent(const ImVec2& pos, TouchType type)
{
    if(!visible()) return;
    /*
        ? ??????? ??????????
        1 - ??????????
        2 - ??????
        3 - ???
    */

    // A tela de conexão perdida é modal: o toque só serve para acionar o botão
    // de reiniciar, e nada mais (nem o GTA) recebe o evento.
    if (m_bConnectionLost)
    {
        NexionControls::Reset();
        if (type == TouchType::push)
        {
            if (pos.x >= m_connLostBtnMin.x && pos.x <= m_connLostBtnMax.x &&
                pos.y >= m_connLostBtnMin.y && pos.y <= m_connLostBtnMax.y)
            {
                RestartConnectionAfterLoss();
            }
        }
        return;
    }

    // O menu é modal: qualquer toque é dele enquanto estiver aberto.
    if (m_settingsMenu && m_settingsMenu->open())
    {
        m_settingsMenu->touchEvent(pos, type);
        return;
    }

    // Botão 3D "Acessar" — clique
    if (m_bankBtnVisible)
    {
        bool hit = (pos.x >= m_bankBtnMin.x && pos.x <= m_bankBtnMax.x &&
                    pos.y >= m_bankBtnMin.y && pos.y <= m_bankBtnMax.y);

        FLog("[3DBTN-Touch] type=%d pos=(%.0f,%.0f) btn=(%.0f,%.0f)-(%.0f,%.0f) hit=%d",
            (int)type, pos.x, pos.y, m_bankBtnMin.x, m_bankBtnMin.y,
            m_bankBtnMax.x, m_bankBtnMax.y, hit ? 1 : 0);

        if (type == TouchType::push)
        {
            m_bankBtnPressed = hit;
        }
        else if (type == TouchType::pop)
        {
            m_bankBtnPressed = false;
            if (hit)
            {
                FLog("[3DBTN] CLIQUE! SendBankOpenRequest");
                if (pNetGame) pNetGame->SendBankOpenRequest();
                return;
            }
        }
    }

    // Botão CEF/HUD de arma: alterna para mãos vazias sem apagar a arma do
    // inventário e restaura exatamente o mesmo tipo ao tocar novamente.
    if (m_fistBtnVisible)
    {
        const bool hit = pos.x >= m_fistBtnMin.x && pos.x <= m_fistBtnMax.x &&
                         pos.y >= m_fistBtnMin.y && pos.y <= m_fistBtnMax.y;
        if (type == TouchType::push)
        {
            m_fistBtnPressed = hit;
        }
        else if (type == TouchType::pop)
        {
            m_fistBtnPressed = false;
            if (hit && pGame)
            {
                CPlayerPed* ped = pGame->FindPlayerPed();
                if (ped && !ped->IsInVehicle())
                {
                    const uint8_t currentWeapon = ped->GetCurrentWeapon();
                    if (m_weaponHolstered)
                    {
                        if (m_savedWeapon != WEAPON_UNARMED)
                        {
                            if (ped->SelectWeaponSlot(m_savedWeapon))
                            {
                                FLog("[WEAPON-HUD] PEGAR arma=%u", m_savedWeapon);
                                m_savedWeapon = WEAPON_UNARMED;
                                m_weaponHolstered = false;
                            }
                        }
                    }
                    else if (currentWeapon != WEAPON_UNARMED)
                    {
                        // Guarda o tipo, não remove a arma nem a munição. O
                        // slot 0 é a mão vazia; o GTA continua com o item no
                        // inventário e pode equipá-lo novamente depois.
                        if (ped->SelectWeaponSlot(WEAPON_UNARMED))
                        {
                            m_savedWeapon = currentWeapon;
                            m_weaponHolstered = true;
                            FLog("[WEAPON-HUD] GUARDAR arma=%u", currentWeapon);
                        }
                    }
                }
                return;
            }
        }
    }

    if (m_keyboard->visible() && m_keyboard->contains(pos))
    {
        m_keyboard->touchEvent(pos, type);
        return;
    }

    if (m_dialog->visible() && m_dialog->contains(pos))
    {
        m_dialog->touchEvent(pos, type);
        return;
    }

    Widget::touchEvent(pos, type);
}

enum eTouchType
{
    TOUCH_POP = 1,
    TOUCH_PUSH = 2,
    TOUCH_MOVE = 3
};

bool UI::IsBankButtonAt(int x, int y) const
{
    if (!m_bankBtnVisible) return false;
    return ((float)x >= m_bankBtnMin.x && (float)x <= m_bankBtnMax.x &&
            (float)y >= m_bankBtnMin.y && (float)y <= m_bankBtnMax.y);
}

bool UI::OnTouchEvent(int type, bool multi, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();

    /*
    switch (type)
    {
    case 1://TOUCH_PUSH:
        io.MousePos = ImVec2(x, y);
        io.MouseDown[0] = true;
        MyLog2("TOUCH_PUSH");
        break;

    case 2://TOUCH_POP:
        io.MouseDown[0] = false;
        m_bNeedClearMousePos = true;
        MyLog2("TOUCH_POP");
        break;

    case 3://TOUCH_MOVE:
        io.MousePos = ImVec2(x, y);
        MyLog2("TOUCH_MOVE");
        break;
    }*/
    VoiceButton* vbutton = pUI->voicebutton();
    switch (type)
    {
        case TOUCH_PUSH:
            io.MousePos = ImVec2(x, y);
            io.MouseDown[0] = true;
            break;

        case TOUCH_POP:
            io.MouseDown[0] = false;
            m_bNeedClearMousePos = true;
            break;

        case TOUCH_MOVE:
            io.MousePos = ImVec2(x, y);
            //if (vbutton->countdown > 50) vbutton->countdown = 20;
            break;
    }

    return true;
}

#include "../settings.h"
extern CSettings* pSettings;
void UI::renderDebug()
{
    if (pSettings == nullptr || !pSettings->Get().iFPSCounter)
    {
        label->setText("");
        return;
    }

    char fpsText[24]{};
    static float fps = 0.0f;
    static auto lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
    if (CTimer::m_snTimeInMillisecondsNonClipped - lastTick > 500)
    {
        lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
        fps = std::max(0.0f, CTimer::game_FPS);
    }

    std::snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", fps);
    label->setText(fpsText);
    label->setPosition(ImVec2(pUI->ScaleX(40.0f),
                              pUI->ScaleY(1080.0f - UISettings::fontSize() * 5)));

    // Textos de arquitetura, memória e posição foram removidos intencionalmente.
    // Otimizacao: label2-8 removidos — nao eram usados.
}


void UI::PushToBufferedQueueTextDrawPressed(uint16_t textdrawId)
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = m_BufferedCommandTextdraws.WriteLock();

    pCmd->textdrawId = textdrawId;

    m_BufferedCommandTextdraws.WriteUnlock();
}

void UI::ProcessPushedTextdraws()
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = nullptr;
    while (pCmd = m_BufferedCommandTextdraws.ReadLock())
    {
        RakNet::BitStream bs;
        bs.Write(pCmd->textdrawId);
        pNetGame->GetRakClient()->RPC(&RPC_ClickTextDraw, &bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, 0);
        m_BufferedCommandTextdraws.ReadUnlock();
    }
}
