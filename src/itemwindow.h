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

#ifndef ITEMWINDOW_H
#define ITEMWINDOW_H

#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>

// A window showing one QML item created in the application's shared
// engine (see Application::createQml). QQuickView would need an engine per
// window; one engine keeps the QML types compiled once and the root context
// (app, config, history, ocr, recorder) shared by every window.
class ItemWindow : public QQuickWindow
{
    Q_OBJECT

public:
    explicit ItemWindow(QWindow *parent = nullptr);
    ~ItemWindow() override;

    // fitToItem: the window takes the size of the item, otherwise the item
    // fills the window.
    void setItem(QQuickItem *item, bool fitToItem);
    QQuickItem *item() const { return m_item; }

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void fitWindow();

    QPointer<QQuickItem> m_item;
    bool m_fitToItem;
};

#endif // ITEMWINDOW_H
