#include "../gui.h"

/* ListBox */

ListBox::ListBox()
{
	m_panel = new Panel();
	this->addChild(m_panel);
}

void ListBox::setItemSize(const ImVec2& size)
{
	m_panel->setItemSize(size);
}

void ListBox::addItem(ListBoxItem* item)
{
	m_panel->addItem(item);
}

void ListBox::setColumns(int columns)
{
    m_panel->setColumns(columns);
}

void ListBox::removeItem(int index)
{
	m_panel->removeItem(index);
}

/* ListBox::Panel */

ListBox::Panel::Panel()
{
	m_itemSize = ImVec2(0.0f, 0.0f);
    m_columns = 1;
	m_activeItemIndex = -1;
	m_itemsCount = 0;
}

void ListBox::Panel::performLayout()
{
    // grid layout with m_columns columns
    int cols = ImMax(1, m_columns);
    ImVec2 cellSize = m_itemSize;
    for (auto c : children())
    {
        c->setFixedSize(cellSize);
        c->performLayout();
    }

    ImVec2 item_pos = ImVec2(0.0f, 0.0f);
    int colIndex = 0;
    float rowHeight = cellSize.y;
    float maxRowWidth = 0.0f;

    for (auto c : children())
    {
        c->setPosition(item_pos);
        maxRowWidth = ImMax(maxRowWidth, item_pos.x + cellSize.x);
        colIndex++;
        if (colIndex >= cols) {
            colIndex = 0;
            item_pos.x = 0.0f;
            item_pos.y += rowHeight;
        } else {
            item_pos.x += cellSize.x;
        }
    }

    if (colIndex != 0) {
        item_pos.y += rowHeight; // last incomplete row height
    }

    this->setSize(ImVec2(maxRowWidth, item_pos.y));
}

void ListBox::Panel::draw(ImGuiRenderer* renderer)
{
	float boxPosY1 = parent()->absolutePosition().y - m_itemSize.y;
	float boxPosY2 = parent()->absolutePosition().y + parent()->height();

	for (auto item : children())
	{
		float posY = item->absolutePosition().y;
		if (posY >= boxPosY1 && posY <= boxPosY2)
		{
			item->draw(renderer);
		}
	}
}

void ListBox::Panel::setItemSize(const ImVec2& size)
{
	m_itemSize = size;
}

void ListBox::Panel::addItem(ListBoxItem* item)
{
	if(item->selectable()) 
		item->setIndex(m_itemsCount++);

	this->addChild(item);
	this->performLayout();
}

void ListBox::Panel::removeItem(int index)
{
	this->removeChild(index);
	this->performLayout();
}

/* ListBoxItem */

ListBoxItem::ListBoxItem(bool selectable)
{
	m_index = -1;
	m_selectable = selectable;
}

void ListBoxItem::activateEvent(bool active)
{
	if (!m_selectable) return;

	if (active) {
		dynamic_cast<ListBox::Panel*>(parent())->setActiveItemIndex(m_index);
	}
}

bool ListBoxItem::selected()
{
	if (!m_selectable) return false;
	return dynamic_cast<ListBox::Panel*>(parent())->activeItemIndex() == m_index;
}