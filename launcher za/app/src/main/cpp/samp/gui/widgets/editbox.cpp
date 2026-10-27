#include "../gui.h"

extern UI* pUI;

EditBox::EditBox()
{
    m_label = new Label("Tap to enter command...", ImColor(1.0f, 1.0f, 1.0f), false, UISettings::fontSize() / 2);// placeholder
	this->addChild(m_label);
}

void EditBox::performLayout()
{
	m_label->performLayout();

	m_label->setPosition(ImVec2(
		UISettings::padding(),

		(height() - m_label->height()) / 2
	));
}

void EditBox::draw(ImGuiRenderer* renderer)
{
    // gray glass background for input
    renderer->drawRoundedRect(absolutePosition(), absolutePosition() + size(), ImColor(1.0f, 1.0f, 1.0f, 0.18f), 8.0f, true);
    renderer->drawRoundedRect(
        absolutePosition() + ImVec2(UISettings::outlineSize(), UISettings::outlineSize()), 
        (absolutePosition() + size()) - ImVec2(UISettings::outlineSize(), UISettings::outlineSize()), 
        ImColor(0.6f, 0.6f, 0.6f, 0.5f), 8.0f, false, UISettings::outlineSize());

	Widget::draw(renderer);
}

void EditBox::touchPopEvent()
{
	pUI->keyboard()->show(this);
}

void EditBox::keyboardEvent(const std::string& input)
{
	m_input = input;
    m_label->setText(input.empty() ? std::string("Tap to enter command...") : Encoding::cp2utf(input));
}