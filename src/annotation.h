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

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

// One drawing object on top of a capture. All coordinates are in image
// pixels of the capture the annotation belongs to.
struct Annotation
{
    enum Type {
        Pencil,
        Line,
        Arrow,
        Rectangle,
        FilledRectangle,
        Ellipse,
        Marker,
        Text,
        Counter,
        Pixelate,
        Blur,
        Invert,
    };

    Type type = Pencil;
    QColor color = QColor(229, 57, 53);
    // Stroke width. Text: font pixel size. Counter: drives the circle size.
    // Pixelate/Blur: strength.
    qreal width = 4;
    // Pencil/Marker: the path. Text/Counter: the anchor. Others: start, end.
    QVector<QPointF> points;
    QString text;
    bool bold = false;
    int number = 0;

    static QString typeName(Type type);
    static bool typeFromName(const QString &name, Type *type);

    static bool isFreehand(Type type) { return type == Pencil || type == Marker; }
    static bool isEffect(Type type) { return type == Pixelate || type == Blur || type == Invert; }
    // Types that are dragged out as a box.
    static bool isBox(Type type)
    {
        return type == Rectangle || type == FilledRectangle || type == Ellipse || isEffect(type);
    }

    // Normalized box between the first and the last point.
    QRectF rect() const;
    // Area covered on the image, including the stroke.
    QRectF boundingRect() const;
    // The outline that is painted for strokes, used for hit testing.
    QPainterPath shape() const;
    bool contains(const QPointF &pos, qreal tolerance) const;
    void translate(const QPointF &delta);
    // Zero sized objects (a click without a drag) are dropped.
    bool isEmpty() const;

    QFont font() const;
    QRectF textRect() const;
    qreal counterRadius() const;
    qreal arrowHeadSize() const;

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &object, Annotation *result);

    bool operator==(const Annotation &other) const;
};

#endif // ANNOTATION_H
