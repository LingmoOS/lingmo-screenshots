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

#ifndef SCREENASKCORE_H
#define SCREENASKCORE_H

#include <QByteArray>
#include <QImage>
#include <QLocale>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QUrl>
#include <QVector>

// "Ask about the screen": the parts without any window or process, shared by
// the app and the unit tests.
namespace ScreenAskCore
{
    // The crop as sent to the model: at most maxSide pixels on its longest
    // side, PNG (JPEG when the PNG would be larger than maxBytes). Never
    // written to disk. format gets "png" or "jpeg".
    QByteArray encodeImage(const QImage &image, int maxSide = 1280, int maxBytes = 3 * 1024 * 1024,
                           QByteArray *format = nullptr);

    // A circle, a scribble or a straight drag drawn around something: the
    // box around the points, grown by padding and kept inside bounds. Null
    // when the points don't make an area.
    QRectF strokeBounds(const QVector<QPointF> &points, qreal padding, const QRectF &bounds);

    // The quick chips. kind: explain, translate, summarize, what, copy.
    // The question sent to the model, in the UI language.
    QString chipQuestion(const QString &kind, const QLocale &locale = QLocale());
    // "Portuguese (Brazil)" in the locale's own language ("português (Brasil)")
    QString languageName(const QLocale &locale = QLocale());

    // "Search the web": the recognized text, cut at a sensible length, on
    // DuckDuckGo. Empty URL when there is no text.
    QUrl webSearchUrl(const QString &text, int maxLength = 200);
}

#endif // SCREENASKCORE_H
