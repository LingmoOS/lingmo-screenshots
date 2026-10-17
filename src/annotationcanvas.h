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

#ifndef ANNOTATIONCANVAS_H
#define ANNOTATIONCANVAS_H

#include <QPointer>
#include <QQuickPaintedItem>

#include "capturesession.h"

// Shows sourceRect of a capture (image pixels) with its annotations, painted
// with the same renderer as the exported image.
class AnnotationCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(CaptureSession *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY sourceRectChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    // Paint the screenshot itself too (otherwise only the annotations).
    Q_PROPERTY(bool paintBase READ paintBase WRITE setPaintBase NOTIFY paintBaseChanged)

public:
    explicit AnnotationCanvas(QQuickItem *parent = nullptr);

    CaptureSession *session() const { return m_session; }
    void setSession(CaptureSession *session);

    QRectF sourceRect() const { return m_sourceRect; }
    void setSourceRect(const QRectF &rect);

    QColor selectionColor() const { return m_selectionColor; }
    void setSelectionColor(const QColor &color);

    bool paintBase() const { return m_paintBase; }
    void setPaintBase(bool paint);

    void paint(QPainter *painter) override;

signals:
    void sessionChanged();
    void sourceRectChanged();
    void selectionColorChanged();
    void paintBaseChanged();

private:
    QPointer<CaptureSession> m_session;
    QRectF m_sourceRect;
    QColor m_selectionColor;
    bool m_paintBase;
};

#endif // ANNOTATIONCANVAS_H
