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

#ifndef ANNOTATIONRENDERER_H
#define ANNOTATIONRENDERER_H

#include "annotation.h"

#include <QImage>
#include <QPainter>

// Draws annotations. The same code paints the live preview and the exported
// image, so what is shown is exactly what gets saved.
namespace AnnotationRenderer
{
    // The painter works in image pixels of base. Effects (pixelate, blur,
    // invert) read their pixels from base, never from other annotations, so
    // a redaction can't be undone by drawing over it.
    void paint(QPainter &painter, const Annotation &annotation, const QImage &base);
    void paint(QPainter &painter, const QVector<Annotation> &annotations, const QImage &base);

    // base cropped to crop (image pixels) with the annotations on top.
    QImage compose(const QImage &base, const QVector<Annotation> &annotations, const QRect &crop);

    // Exposed for tests.
    QImage pixelate(const QImage &source, int blockSize);
    QImage blur(const QImage &source, int radius);
    QColor counterTextColor(const QColor &background);
}

#endif // ANNOTATIONRENDERER_H
