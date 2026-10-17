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

#ifndef IMAGEANALYSIS_H
#define IMAGEANALYSIS_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QVector>

// Smart selection: rectangles of the user interface (panels, cards, text
// fields, images...) found from the edges in the capture.
namespace RegionDetector
{
    // Nested boxes around pos, innermost first. Each one is bounded by long
    // straight edges on all four sides. At most maxLevels boxes. image should
    // be Format_Grayscale8 (other formats are converted, which is slower).
    QVector<QRect> regionsAt(const QImage &image, const QPoint &pos, int maxLevels = 3);
}

// Long captures: joins screenshots of a scrolled area.
namespace Stitcher
{
    struct Match {
        // How many rows the content moved up between prev and next (same
        // size): row r of next shows what row r + offset of prev showed.
        // 0 when nothing moved, -1 when no reliable match was found.
        int offset = -1;
        // Rows at the bottom that stay in place (a footer, a status bar).
        int footer = 0;
    };

    Match match(const QImage &prev, const QImage &next, int minOverlap = 24);

    // Adds the content that next brings below stitched. The last
    // match.footer rows of stitched are the fixed footer: the new rows go
    // above it.
    QImage append(const QImage &stitched, const QImage &next, const Match &match);
}

#endif // IMAGEANALYSIS_H
