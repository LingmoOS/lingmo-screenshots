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

#include "imageanalysis.h"

#include <QPainter>
#include <cstdlib>

namespace {

// Brightness step that counts as an edge.
const int EDGE_THRESHOLD = 10;
// Rows/columns around the pointer that a line has to cross.
const int BAND = 5;
// Share of an edge that must be present along a whole side.
const double SIDE_COVERAGE = 0.6;
const int MIN_REGION = 12;

class Edges
{
public:
    explicit Edges(const QImage &gray) : m_img(gray), m_w(gray.width()), m_h(gray.height()) {}

    int width() const { return m_w; }
    int height() const { return m_h; }

    // Edge between columns x-1 and x, on row y.
    bool vertical(int x, int y) const
    {
        const uchar *row = m_img.constScanLine(y);
        return std::abs(int(row[x]) - int(row[x - 1])) > EDGE_THRESHOLD;
    }

    // Edge between rows y-1 and y, on column x.
    bool horizontal(int x, int y) const
    {
        return std::abs(int(m_img.constScanLine(y)[x]) - int(m_img.constScanLine(y - 1)[x])) > EDGE_THRESHOLD;
    }

    int verticalCount(int x, int y0, int y1) const
    {
        int n = 0;
        for (int y = qMax(0, y0); y <= qMin(m_h - 1, y1); ++y)
            n += vertical(x, y);
        return n;
    }

    int horizontalCount(int y, int x0, int x1) const
    {
        int n = 0;
        for (int x = qMax(0, x0); x <= qMin(m_w - 1, x1); ++x)
            n += horizontal(x, y);
        return n;
    }

private:
    const QImage &m_img;
    int m_w;
    int m_h;
};

// Nearest line crossing the band around pos, scanning from `from` in
// direction step (-1 left/up, +1 right/down). -1 when there is none.
int nextVertical(const Edges &e, int from, int step, int y0, int y1, int need)
{
    for (int x = from; x >= 1 && x < e.width(); x += step) {
        if (e.verticalCount(x, y0, y1) >= need)
            return x;
    }
    return -1;
}

int nextHorizontal(const Edges &e, int from, int step, int x0, int x1, int need)
{
    for (int y = from; y >= 1 && y < e.height(); y += step) {
        if (e.horizontalCount(y, x0, x1) >= need)
            return y;
    }
    return -1;
}

// The innermost box around pos whose sides lie outside `inside` (or
// anywhere around pos when inside is null). Edge positions follow Edges.
// Letters next to the pointer also look like short lines: a side that is
// not a line along the whole box is replaced by the next candidate.
QRect scanBox(const Edges &e, const QPoint &pos, const QRect &inside)
{
    const int w = e.width();
    const int h = e.height();

    const int y0 = pos.y() - BAND, y1 = pos.y() + BAND;
    const int x0 = pos.x() - BAND, x1 = pos.x() + BAND;
    const int needV = int((qMin(h - 1, y1) - qMax(0, y0) + 1) * 0.9);
    const int needH = int((qMin(w - 1, x1) - qMax(0, x0) + 1) * 0.9);

    int left = nextVertical(e, inside.isNull() ? pos.x() : inside.left() - 1, -1, y0, y1, needV);
    int right = nextVertical(e, inside.isNull() ? pos.x() + 1 : inside.right() + 2, 1, y0, y1, needV);
    int top = nextHorizontal(e, inside.isNull() ? pos.y() : inside.top() - 1, -1, x0, x1, needH);
    int bottom = nextHorizontal(e, inside.isNull() ? pos.y() + 1 : inside.bottom() + 2, 1, x0, x1, needH);

    for (int attempt = 0; attempt < 32; ++attempt) {
        if (left < 0 || right < 0 || top < 0 || bottom < 0)
            return QRect();

        // The box is [left, right) x [top, bottom).
        const int bw = right - left;
        const int bh = bottom - top;
        if (bw < MIN_REGION || bh < MIN_REGION)
            return QRect();

        const bool okLeft = e.verticalCount(left, top, bottom - 1) >= bh * SIDE_COVERAGE;
        const bool okRight = e.verticalCount(right, top, bottom - 1) >= bh * SIDE_COVERAGE;
        const bool okTop = e.horizontalCount(top, left, right - 1) >= bw * SIDE_COVERAGE;
        const bool okBottom = e.horizontalCount(bottom, left, right - 1) >= bw * SIDE_COVERAGE;

        if (okLeft && okRight && okTop && okBottom) {
            QRect box(QPoint(left, top), QPoint(right - 1, bottom - 1));

            // Include a one pixel border line: a second edge right outside.
            if (left >= 2 && e.verticalCount(left - 1, top, bottom - 1) >= bh * SIDE_COVERAGE)
                box.setLeft(left - 1);
            if (right + 1 < w && e.verticalCount(right + 1, top, bottom - 1) >= bh * SIDE_COVERAGE)
                box.setRight(right);
            if (top >= 2 && e.horizontalCount(top - 1, left, right - 1) >= bw * SIDE_COVERAGE)
                box.setTop(top - 1);
            if (bottom + 1 < h && e.horizontalCount(bottom + 1, left, right - 1) >= bw * SIDE_COVERAGE)
                box.setBottom(bottom);

            return box;
        }

        if (!okLeft)
            left = nextVertical(e, left - 1, -1, y0, y1, needV);
        if (!okRight)
            right = nextVertical(e, right + 1, 1, y0, y1, needV);
        if (!okTop)
            top = nextHorizontal(e, top - 1, -1, x0, x1, needH);
        if (!okBottom)
            bottom = nextHorizontal(e, bottom + 1, 1, x0, x1, needH);
    }

    return QRect();
}

bool almostSame(const QRect &a, const QRect &b)
{
    const int t = 6;
    return std::abs(a.left() - b.left()) <= t && std::abs(a.top() - b.top()) <= t
            && std::abs(a.right() - b.right()) <= t && std::abs(a.bottom() - b.bottom()) <= t;
}

}

QVector<QRect> RegionDetector::regionsAt(const QImage &image, const QPoint &pos, int maxLevels)
{
    QVector<QRect> result;

    if (image.isNull() || !image.rect().adjusted(1, 1, -1, -1).contains(pos))
        return result;

    const QImage gray = image.format() == QImage::Format_Grayscale8
            ? image : image.convertToFormat(QImage::Format_Grayscale8);
    const Edges edges(gray);

    QRect inside;
    for (int attempt = 0; attempt < maxLevels * 3 && result.size() < maxLevels; ++attempt) {
        const QRect box = scanBox(edges, pos, inside);
        if (box.isNull() || box == gray.rect())
            break;

        if (result.isEmpty() || !almostSame(result.last(), box))
            result.append(box);
        inside = box;
    }

    return result;
}

namespace {

// Rows in grayscale without the borders, where scroll bars move. Text
// drawn at fractional scroll positions is antialiased a little differently
// each time, so rows are compared with a tolerance, not byte for byte.
struct Rows {
    QImage gray;
    int x0 = 0;
    int x1 = 0;
    QVector<bool> uniform;

    explicit Rows(const QImage &image)
        : gray(image.convertToFormat(QImage::Format_Grayscale8))
    {
        const int w = gray.width();
        x0 = w > 120 ? 4 : 0;
        x1 = w > 120 ? w - 24 : w;
        uniform.resize(gray.height());
        for (int y = 0; y < gray.height(); ++y) {
            const uchar *line = gray.constScanLine(y);
            int lo = 255, hi = 0;
            for (int x = x0; x < x1; ++x) {
                lo = qMin(lo, int(line[x]));
                hi = qMax(hi, int(line[x]));
            }
            uniform[y] = hi - lo < 8;
        }
    }

    // Mean absolute difference of two rows (every second column).
    double diff(int y, const Rows &other, int otherY) const
    {
        const uchar *a = gray.constScanLine(y);
        const uchar *b = other.gray.constScanLine(otherY);
        int sum = 0, n = 0;
        for (int x = x0; x < x1; x += 2, ++n)
            sum += std::abs(int(a[x]) - int(b[x]));
        return n > 0 ? double(sum) / n : 0;
    }
};

// Rows count as the same below this mean difference.
const double SAME_ROW = 2.0;
// A match is accepted below this mean difference over the overlap.
const double MATCH_LIMIT = 6.0;

}

Stitcher::Match Stitcher::match(const QImage &prev, const QImage &next, int minOverlap)
{
    Match result;

    if (prev.size() != next.size() || prev.isNull())
        return result;

    const int h = prev.height();
    const Rows p(prev);
    const Rows n(next);

    // Rows that did not move: fixed header and footer.
    int header = 0;
    while (header < h && n.diff(header, p, header) < SAME_ROW)
        ++header;
    if (header == h) {
        result.offset = 0;
        return result;
    }
    int footer = 0;
    while (footer < h - header && n.diff(h - 1 - footer, p, h - 1 - footer) < SAME_ROW)
        ++footer;

    double best = 1e9;
    int bestOffset = -1;

    for (int dy = 1; dy <= h - header - footer - minOverlap; ++dy) {
        double total = 0;
        int informative = 0;
        for (int r = header; r + dy < h - footer; ++r) {
            if (n.uniform[r] && p.uniform[r + dy])
                continue;
            ++informative;
            total += n.diff(r, p, r + dy);
            // Already worse than the best one: stop early.
            if (informative > 16 && total / informative > best * 3 + MATCH_LIMIT)
                break;
        }

        if (informative < qMax(4, minOverlap / 2))
            continue;

        const double mean = total / informative;
        if (mean < best) {
            best = mean;
            bestOffset = dy;
        }
    }

    if (bestOffset < 0 || best > MATCH_LIMIT)
        return result;

    result.offset = bestOffset;
    result.footer = footer;
    return result;
}

QImage Stitcher::append(const QImage &stitched, const QImage &next, const Match &match)
{
    if (match.offset <= 0 || next.isNull())
        return stitched;
    if (stitched.isNull())
        return next;

    const int h = next.height();
    const int footer = qBound(0, match.footer, h);
    const int added = qMin(match.offset, h - footer);
    const int body = stitched.height() - footer;

    QImage result(stitched.width(), body + added + footer, QImage::Format_RGB32);
    QPainter p(&result);
    p.drawImage(QPoint(0, 0), stitched, QRect(0, 0, stitched.width(), body));
    p.drawImage(QPoint(0, body), next, QRect(0, h - footer - added, next.width(), added + footer));
    p.end();

    return result;
}
