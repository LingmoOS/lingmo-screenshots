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

#ifndef PINWINDOW_H
#define PINWINDOW_H

#include <QImage>
#include "itemwindow.h"

// A capture pinned on the screen: a small always-on-top window that can be
// moved, zoomed with the wheel, made translucent and closed.
class PinWindow : public ItemWindow
{
    Q_OBJECT
    Q_PROPERTY(QString imageUrl READ imageUrl CONSTANT)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal imageOpacity READ imageOpacity WRITE setImageOpacity NOTIFY imageOpacityChanged)

public:
    // image in device pixels; pos: top-left in logical screen coordinates.
    PinWindow(const QImage &image, const QPoint &pos, QScreen *screen);
    ~PinWindow() override;

    QString imageUrl() const;
    qreal zoom() const { return m_zoom; }
    void setZoom(qreal zoom);
    qreal imageOpacity() const { return m_opacity; }
    void setImageOpacity(qreal opacity);

    Q_INVOKABLE void copy();
    Q_INVOKABLE void saveAs();
    Q_INVOKABLE void edit();
    Q_INVOKABLE void closePin();
    Q_INVOKABLE void moveBy(int dx, int dy);

signals:
    void zoomChanged();
    void imageOpacityChanged();
    void closed();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateSize();

    QImage m_image;
    QString m_imageId;
    qreal m_zoom;
    qreal m_opacity;
};

#endif // PINWINDOW_H
