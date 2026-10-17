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

#include "annotationcanvas.h"
#include "annotationrenderer.h"
#include "capturesession.h"

#include <QPainter>

AnnotationCanvas::AnnotationCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_selectionColor(0x30, 0x85, 0xfe)
    , m_paintBase(true)
{
    setAntialiasing(true);
}

void AnnotationCanvas::setSession(CaptureSession *session)
{
    if (m_session == session)
        return;

    if (m_session)
        m_session->annotations()->disconnect(this);

    m_session = session;

    if (m_session) {
        connect(m_session->annotations(), &AnnotationModel::changed, this, [this] { update(); });
        connect(m_session->annotations(), &AnnotationModel::selectionChanged, this, [this] { update(); });
    }

    emit sessionChanged();
    update();
}

void AnnotationCanvas::setSourceRect(const QRectF &rect)
{
    if (m_sourceRect == rect)
        return;
    m_sourceRect = rect;
    emit sourceRectChanged();
    update();
}

void AnnotationCanvas::setSelectionColor(const QColor &color)
{
    if (m_selectionColor == color)
        return;
    m_selectionColor = color;
    emit selectionColorChanged();
    update();
}

void AnnotationCanvas::setPaintBase(bool paint)
{
    if (m_paintBase == paint)
        return;
    m_paintBase = paint;
    emit paintBaseChanged();
    update();
}

void AnnotationCanvas::paint(QPainter *painter)
{
    if (!m_session || m_sourceRect.isEmpty() || width() <= 0 || height() <= 0)
        return;

    const QImage &base = m_session->image();
    AnnotationModel *model = m_session->annotations();

    painter->save();
    painter->scale(width() / m_sourceRect.width(), height() / m_sourceRect.height());
    painter->translate(-m_sourceRect.topLeft());

    if (m_paintBase)
        painter->drawImage(m_sourceRect, base, m_sourceRect);

    AnnotationRenderer::paint(*painter, model->items(), base);
    if (const Annotation *current = model->current())
        AnnotationRenderer::paint(*painter, *current, base);

    // Selected object: a dashed frame (not part of the export).
    const QRectF selected = model->selectedRect();
    if (!selected.isEmpty()) {
        const qreal scale = m_sourceRect.width() / width();
        QPen pen(m_selectionColor, 1.5 * scale, Qt::DashLine);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(selected.adjusted(-3 * scale, -3 * scale, 3 * scale, 3 * scale));
    }

    painter->restore();
}
