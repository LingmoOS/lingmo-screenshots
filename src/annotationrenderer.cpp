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

#include "annotationrenderer.h"

#include <QLineF>
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>

QImage AnnotationRenderer::pixelate(const QImage &source, int blockSize)
{
    if (source.isNull())
        return source;

    blockSize = qMax(2, blockSize);
    const int w = qMax(1, source.width() / blockSize);
    const int h = qMax(1, source.height() / blockSize);
    QImage small = source.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return small.scaled(source.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

QImage AnnotationRenderer::blur(const QImage &source, int radius)
{
    if (source.isNull())
        return source;

    radius = qMax(2, radius);
    QImage result = source;
    // Two rounds of down/up scaling: cheap, and smooth enough to hide text.
    for (int round = 0; round < 2; ++round) {
        const int w = qMax(1, result.width() / radius);
        const int h = qMax(1, result.height() / radius);
        result = result.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                       .scaled(source.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return result;
}

QColor AnnotationRenderer::counterTextColor(const QColor &background)
{
    // Relative luminance, sRGB approximation.
    const qreal l = 0.2126 * background.redF() + 0.7152 * background.greenF() + 0.0722 * background.blueF();
    return l > 0.6 ? QColor(20, 20, 20) : QColor(255, 255, 255);
}

namespace {

void paintArrow(QPainter &painter, const Annotation &a)
{
    const QPointF from = a.points.first();
    const QPointF to = a.points.last();
    const QLineF line(from, to);
    if (line.length() < 1)
        return;

    const qreal head = qMin(a.arrowHeadSize(), line.length() * 0.8);
    const qreal angle = qDegreesToRadians(line.angle());
    const QPointF dir(qCos(angle), -qSin(angle));
    const QPointF normal(-dir.y(), dir.x());
    const QPointF base = to - dir * head;
    const qreal halfWidth = head * 0.55;

    // The shaft stops inside the head so its square end doesn't stick out.
    QPen pen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(from, to - dir * (head * 0.6));

    QPolygonF triangle;
    triangle << to << base + normal * halfWidth << base - normal * halfWidth;
    painter.setPen(Qt::NoPen);
    painter.setBrush(a.color);
    painter.drawPolygon(triangle);
}

void paintEffect(QPainter &painter, const Annotation &a, const QImage &base)
{
    const QRect r = a.rect().toAlignedRect().intersected(base.rect());
    if (r.isEmpty())
        return;

    QImage region = base.copy(r);
    region.setDevicePixelRatio(1);

    switch (a.type) {
    case Annotation::Pixelate:
        region = AnnotationRenderer::pixelate(region, qRound(qMax<qreal>(4, a.width * 2.5)));
        break;
    case Annotation::Blur:
        region = AnnotationRenderer::blur(region, qRound(qMax<qreal>(3, a.width * 1.5)));
        break;
    case Annotation::Invert:
        region = region.convertToFormat(QImage::Format_RGB32);
        region.invertPixels();
        break;
    default:
        return;
    }

    painter.drawImage(r.topLeft(), region);
}

}

void AnnotationRenderer::paint(QPainter &painter, const Annotation &a, const QImage &base)
{
    if (a.points.isEmpty())
        return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    switch (a.type) {
    case Annotation::Pencil: {
        painter.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(a.shape());
        break;
    }
    case Annotation::Marker: {
        // Drawn as one path so that overlapping segments don't get darker.
        QColor c = a.color;
        c.setAlpha(110);
        painter.setPen(QPen(c, a.width * 3, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.drawPath(a.shape());
        break;
    }
    case Annotation::Line:
        painter.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(a.points.first(), a.points.last());
        break;
    case Annotation::Arrow:
        paintArrow(painter, a);
        break;
    case Annotation::Rectangle:
        painter.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(a.rect());
        break;
    case Annotation::FilledRectangle:
        painter.setPen(Qt::NoPen);
        painter.setBrush(a.color);
        painter.drawRect(a.rect());
        break;
    case Annotation::Ellipse:
        painter.setPen(QPen(a.color, a.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(a.rect());
        break;
    case Annotation::Text: {
        painter.setFont(a.font());
        painter.setPen(a.color);
        const QRectF r = a.textRect().adjusted(2, 2, -2, -2);
        painter.drawText(r, Qt::AlignLeft | Qt::AlignTop, a.text);
        break;
    }
    case Annotation::Counter: {
        const qreal r = a.counterRadius();
        const QPointF c = a.points.first();
        painter.setPen(Qt::NoPen);
        painter.setBrush(a.color);
        painter.drawEllipse(c, r, r);

        QFont f = a.font();
        f.setBold(true);
        f.setPixelSize(qMax(8, qRound(r * (a.number > 9 ? 0.95 : 1.15))));
        painter.setFont(f);
        painter.setPen(counterTextColor(a.color));
        painter.drawText(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), Qt::AlignCenter, QString::number(a.number));
        break;
    }
    case Annotation::Pixelate:
    case Annotation::Blur:
    case Annotation::Invert:
        paintEffect(painter, a, base);
        break;
    }

    painter.restore();
}

void AnnotationRenderer::paint(QPainter &painter, const QVector<Annotation> &annotations, const QImage &base)
{
    for (const Annotation &a : annotations)
        paint(painter, a, base);
}

QImage AnnotationRenderer::compose(const QImage &base, const QVector<Annotation> &annotations, const QRect &crop)
{
    const QRect r = crop.isNull() ? base.rect() : crop.intersected(base.rect());
    if (r.isEmpty())
        return QImage();

    QImage result = base.copy(r).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    result.setDevicePixelRatio(1);

    if (annotations.isEmpty())
        return result;

    QImage source = base;
    source.setDevicePixelRatio(1);

    QPainter painter(&result);
    painter.translate(-r.topLeft());
    paint(painter, annotations, source);
    painter.end();

    return result;
}
