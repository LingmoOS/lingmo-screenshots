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

#ifndef BEAUTIFY_H
#define BEAUTIFY_H

#include <QColor>
#include <QImage>
#include <QPair>
#include <QRect>
#include <QVector>

// "Presentation" look for captures: the image on a gradient with some
// padding, rounded corners and a soft shadow.
namespace Beautify
{
    struct Options {
        int padding = 48;      // image pixels around the capture
        int radius = 12;       // corner radius of the capture
        bool shadow = true;
        int background = 0;    // index in backgrounds()
    };

    struct Geometry {
        QSize canvas;          // size of the result
        QRect image;           // where the capture goes
        QRect shadow;          // shadow box (before blurring), empty without shadow
        int radius = 0;        // effective corner radius
        int shadowBlur = 0;
    };

    // Gradient presets (top-left, bottom-right).
    QVector<QPair<QColor, QColor>> backgrounds();

    Geometry geometry(const QSize &imageSize, const Options &options);
    QImage render(const QImage &image, const Options &options);
}

#endif // BEAUTIFY_H
