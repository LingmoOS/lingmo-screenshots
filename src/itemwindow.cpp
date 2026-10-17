/*
 * Copyright (C) 2026 LingmoOS Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "itemwindow.h"

#include <QResizeEvent>
#include <QtMath>

ItemWindow::ItemWindow(QWindow *parent)
    : QQuickWindow(parent)
    , m_fitToItem(false)
{
}

ItemWindow::~ItemWindow()
{
    // The item goes before the scene it belongs to.
    delete m_item.data();
}

void ItemWindow::setItem(QQuickItem *item, bool fitToItem)
{
    m_item = item;
    m_fitToItem = fitToItem;
    if (!item)
        return;

    item->setParentItem(contentItem());
    item->setParent(this);

    if (fitToItem) {
        connect(item, &QQuickItem::implicitWidthChanged, this, &ItemWindow::fitWindow);
        connect(item, &QQuickItem::implicitHeightChanged, this, &ItemWindow::fitWindow);
        connect(item, &QQuickItem::widthChanged, this, &ItemWindow::fitWindow);
        connect(item, &QQuickItem::heightChanged, this, &ItemWindow::fitWindow);
        fitWindow();
    } else {
        item->setSize(size());
    }
    item->forceActiveFocus();
}

void ItemWindow::fitWindow()
{
    if (!m_item)
        return;
    const qreal w = m_item->width() > 0 ? m_item->width() : m_item->implicitWidth();
    const qreal h = m_item->height() > 0 ? m_item->height() : m_item->implicitHeight();
    if (w > 0 && h > 0)
        resize(qCeil(w), qCeil(h));
}

void ItemWindow::resizeEvent(QResizeEvent *event)
{
    QQuickWindow::resizeEvent(event);
    if (m_item && !m_fitToItem)
        m_item->setSize(event->size());
}
