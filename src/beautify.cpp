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

#include "beautify.h"
#include "annotationrenderer.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

QVector<QPair<QColor, QColor>> Beautify::backgrounds()
{
    // Artwork presets, not UI colours: they are part of the exported image.
    return {
        { QColor(0x8E, 0xC5, 0xFC), QColor(0xE0, 0xC3, 0xFC) },
        { QColor(0xFB, 0xAB, 0x7E), QColor(0xF7, 0xCE, 0x68) },
        { QColor(0x85, 0xE3, 0xBD), QColor(0x3E, 0x9B, 0xD8) },
        { QColor(0x41, 0x58, 0xD0), QColor(0xC8, 0x50, 0xC0) },
        { QColor(0x0F, 0x20, 0x27), QColor(0x2C, 0x53, 0x64) },
        { QColor(0xF5, 0xF7, 0xFA), QColor(0xC3, 0xCF, 0xE2) },
    };
}

Beautify::Geometry Beautify::geometry(const QSize &imageSize, const Options &options)
{
    Geometry g;

    if (imageSize.isEmpty())
        return g;

    const int padding = qBound(0, options.padding, 1000);
    g.canvas = imageSize + QSize(padding * 2, padding * 2);
    g.image = QRect(QPoint(padding, padding), imageSize);
    g.radius = qBound(0, options.radius, qMin(imageSize.width(), imageSize.height()) / 2);

    // The shadow has to fit in the padding, otherwise it gets cut.
    if (options.shadow && padding >= 8) {
        g.shadowBlur = qMin(padding * 3 / 5, 40);
        const int offset = qMax(2, g.shadowBlur / 4);
        g.shadow = g.image.translated(0, offset);
    }

    return g;
}

QImage Beautify::render(const QImage &image, const Options &options)
{
    const Geometry g = geometry(image.size(), options);
    if (g.canvas.isEmpty())
        return image;

    QImage result(g.canvas, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Background
    if (g.image.topLeft() != QPoint(0, 0)) {
        const auto presets = backgrounds();
        const auto colors = presets.at(qBound(0, options.background, int(presets.size()) - 1));
        QLinearGradient gradient(QPointF(0, 0), QPointF(g.canvas.width(), g.canvas.height()));
        gradient.setColorAt(0, colors.first);
        gradient.setColorAt(1, colors.second);
        p.fillRect(result.rect(), gradient);
    }

    // Shadow: a dark rounded box, blurred.
    if (!g.shadow.isEmpty()) {
        QImage shadow(g.canvas, QImage::Format_ARGB32_Premultiplied);
        shadow.fill(Qt::transparent);
        {
            QPainter sp(&shadow);
            sp.setRenderHint(QPainter::Antialiasing, true);
            sp.setPen(Qt::NoPen);
            sp.setBrush(QColor(0, 0, 0, 110));
            sp.drawRoundedRect(g.shadow, g.radius, g.radius);
        }
        shadow = AnnotationRenderer::blur(shadow, qMax(2, g.shadowBlur / 3));
        p.drawImage(0, 0, shadow);
    }

    // The capture with rounded corners.
    QPainterPath clip;
    clip.addRoundedRect(QRectF(g.image), g.radius, g.radius);
    p.setClipPath(clip);
    QImage source = image;
    source.setDevicePixelRatio(1);
    p.drawImage(g.image.topLeft(), source);
    p.end();

    return result;
}
