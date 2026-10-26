#include "../../main.h"
#include "../gui.h"
#include "../../java/jniutil.h"
#include "../../game/game.h"
#include "../../net/netgame.h"
#include "../../net/localplayer.h"
#include "../../vendor/imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <functional>

extern UI* pUI;
extern CJavaWrapper* pJavaWrapper;

bool bNeedEnterVehicle = false;
// Mantido para compatibilidade com o desenho de CButton legado.
bool OpenButton = false;

namespace
{
    enum class NexionControlIcon
    {
        Settings,
        Radio,
        Phone,
        Chat,
        Text
    };

    class HudControlButton : public Widget
    {
    public:
        HudControlButton(NexionControlIcon icon, const std::string& caption = "")
            : m_icon(icon), m_caption(caption)
        {
        }

        void setCallback(const std::function<void()>& callback)
        {
            m_callback = callback;
        }

        virtual void performLayout() override
        {
            // O painel pode ocupar uma area maior, mas os controles permanecem
            // compactos como os botoes de atalho abaixo do minimapa.
            const float edge = UISettings::fontSize() * 2.0f;
            this->setFixedSize(ImVec2(edge, edge));
        }

        virtual void draw(ImGuiRenderer* renderer) override
        {
            const ImVec2 pos = absolutePosition();
            const ImVec2 end = pos + size();
            const ImColor background = focused()
                    ? UISettings::buttonFocusedColor()
                    : UISettings::buttonColor();
            const ImColor iconColor = ImColor(1.0f, 1.0f, 1.0f, 0.96f);
            const ImVec2 center = pos + size() * 0.50f;
            const float unit = ImMin(width(), height());

            renderer->drawRoundedRect(pos, end, background, unit * 0.20f, true);

            switch (m_icon)
            {
                case NexionControlIcon::Settings:
                {
                    const float half = unit * 0.22f;
                    const float gap = unit * 0.16f;
                    renderer->drawLine(ImVec2(center.x - half, center.y - gap), ImVec2(center.x + half, center.y - gap), iconColor, 2.0f);
                    renderer->drawLine(ImVec2(center.x - half, center.y),       ImVec2(center.x + half, center.y),       iconColor, 2.0f);
                    renderer->drawLine(ImVec2(center.x - half, center.y + gap), ImVec2(center.x + half, center.y + gap), iconColor, 2.0f);
                    break;
                }

                case NexionControlIcon::Radio:
                {
                    const float outer = unit * 0.24f;
                    const float inner = unit * 0.09f;
                    renderer->drawRoundedRect(ImVec2(center.x - outer, center.y - outer),
                                              ImVec2(center.x + outer, center.y + outer),
                                              iconColor, outer, false, 2.0f);
                    renderer->drawRoundedRect(ImVec2(center.x - inner, center.y - inner),
                                              ImVec2(center.x + inner, center.y + inner),
                                              iconColor, inner, true);
                    break;
                }

                case NexionControlIcon::Phone:
                {
                    const float bodyW = unit * 0.24f;
                    const float bodyH = unit * 0.40f;
                    const ImVec2 bodyMin(center.x - bodyW, center.y - bodyH);
                    const ImVec2 bodyMax(center.x + bodyW, center.y + bodyH);
                    renderer->drawRoundedRect(bodyMin, bodyMax, iconColor, unit * 0.07f, false, 2.0f);
                    const float dot = unit * 0.035f;
                    const ImVec2 dotCenter(center.x, bodyMax.y - unit * 0.10f);
                    renderer->drawRoundedRect(ImVec2(dotCenter.x - dot, dotCenter.y - dot),
                                              ImVec2(dotCenter.x + dot, dotCenter.y + dot),
                                              iconColor, dot, true);
                    renderer->drawLine(ImVec2(bodyMin.x + unit * 0.10f, bodyMin.y + unit * 0.10f),
                                       ImVec2(bodyMax.x - unit * 0.10f, bodyMin.y + unit * 0.10f), iconColor, 1.4f);
                    break;
                }

                case NexionControlIcon::Chat:
                {
                    const float halfW = unit * 0.25f;
                    const float halfH = unit * 0.19f;
                    const ImVec2 bubbleMin(center.x - halfW, center.y - halfH);
                    const ImVec2 bubbleMax(center.x + halfW, center.y + halfH);
                    renderer->drawRoundedRect(bubbleMin, bubbleMax, iconColor, unit * 0.08f, false, 2.0f);
                    renderer->drawTriangle(ImVec2(center.x - halfW * 0.15f, bubbleMax.y),
                                           ImVec2(center.x + halfW * 0.20f, bubbleMax.y),
                                           ImVec2(center.x - halfW * 0.06f, bubbleMax.y + unit * 0.13f),
                                           iconColor, true);
                    break;
                }

                case NexionControlIcon::Text:
                {
                    const ImVec2 textSize = renderer->calculateTextSize(m_caption, UISettings::fontSize() * 0.62f);
                    renderer->drawText(center - textSize * 0.50f, iconColor, m_caption, false, UISettings::fontSize() * 0.62f);
                    break;
                }
            }
        }

        virtual void touchPopEvent() override
        {
            if (m_callback) m_callback();
        }

    private:
        NexionControlIcon m_icon;
        std::string m_caption;
        std::function<void()> m_callback;
    };
}

void ButtonPanel::performLayout()
{
    // Os quatro controles ficam juntos abaixo do minimapa, na ordem:
    // menu (=), bolinha, celular e chat.
    ImGuiIO& io = ImGui::GetIO();
    const float scale = std::min(io.DisplaySize.x / 1920.0f, io.DisplaySize.y / 1080.0f);
    if (scale <= 0.0f) return;

    // Mesmos valores usados em nexion_minimap.cpp: centro (margin+radius) e raio 146*scale.
    const float radarRadius = 146.0f * scale;
    const float radarMargin = 26.0f * scale;
    const ImVec2 radarCenter(radarMargin + radarRadius, radarMargin + radarRadius);

    // Botoes proporcionais ao radar (e nao ao fontSize, que escala demais em
    // telas verticais e fazia os botoes cobrirem o mapa).
    const float buttonEdge = 72.0f * scale;

    const ImVec2 panelOrigin = absolutePosition();

    const float gap = 10.0f * scale;
    const float totalWidth = buttonEdge * 4.0f + gap * 3.0f;
    const float startX = radarCenter.x - totalWidth * 0.5f;
    const float rowY = radarCenter.y + radarRadius + 48.0f * scale;
    for (size_t i = 0; i < children().size() && i < 4; ++i)
    {
        Widget* child = children()[i];
        child->performLayout();
        child->setFixedSize(ImVec2(buttonEdge, buttonEdge));

        child->setPosition(ImVec2(
            startX - panelOrigin.x + static_cast<float>(i) * (buttonEdge + gap),
            rowY - panelOrigin.y - child->height() * 0.5f));
    }
}

ButtonPanel::ButtonPanel()
        : Layout(Orientation::HORIZONTAL)
{
    setPadding(0.0f);
    setSpacing(UISettings::fontSize() * 0.18f);

    auto* settingsButton = new HudControlButton(NexionControlIcon::Settings);
    auto* radioButton = new HudControlButton(NexionControlIcon::Radio);
    auto* phoneButton = new HudControlButton(NexionControlIcon::Phone);
    auto* chatButton = new HudControlButton(NexionControlIcon::Chat);

    // Apenas o primeiro atalho abre o menu nativo de configurações.
    settingsButton->setCallback([]()
    {
        if (pUI && pUI->settingsmenu())
        {
            pUI->settingsmenu()->show();
        }
    });
    radioButton->setCallback([]() {});
    phoneButton->setCallback([]()
    {
        if (pJavaWrapper) pJavaWrapper->TogglePhone();
    });
    chatButton->setCallback([]()
    {
        if (pUI && pUI->chat())
        {
            pUI->chat()->openInput();
        }
    });

    m_bSettings = settingsButton;
    m_bRadio = radioButton;
    m_bPhone = phoneButton;
    m_bChat = chatButton;

    this->addChild(m_bSettings);
    this->addChild(m_bRadio);
    this->addChild(m_bPhone);
    this->addChild(m_bChat);
}
