#pragma once

class ButtonPanel : public Layout
{
public:
    ButtonPanel();

    void performLayout() override;

    // Controles visuais do HUD: menu, bolinha, celular e chat.
    Widget* m_bSettings;
    Widget* m_bRadio;
    Widget* m_bPhone;
    Widget* m_bChat;
};
