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

#include "annotation.h"

#include <QFontMetricsF>
#include <QGuiApplication>
#include <QJsonArray>
#include <QLineF>
#include <QPainterPathStroker>
#include <QtMath>

namespace {

struct TypeName {
    Annotation::Type type;
    const char *name;
};

const TypeName TYPE_NAMES[] = {
    { Annotation::Pencil, "pencil" },
    { Annotation::Line, "line" },
    { Annotation::Arrow, "arrow" },
    { Annotation::Rectangle, "rectangle" },
    { Annotation::FilledRectangle, "filledRectangle" },
    { Annotation::Ellipse, "ellipse" },
    { Annotation::Marker, "marker" },
    { Annotation::Text, "text" },
    { Annotation::Counter, "counter" },
    { Annotation::Pixelate, "pixelate" },
    { Annotation::Blur, "blur" },
    { Annotation::Invert, "invert" },
};

// Text boxes get a little room around the glyphs, also used by the editor.
const qreal TEXT_MARGIN = 2;

}

QString Annotation::typeName(Type type)
{
    for (const TypeName &entry : TYPE_NAMES) {
        if (entry.type == type)
            return QString::fromLatin1(entry.name);
    }
    return QString();
}

bool Annotation::typeFromName(const QString &name, Type *type)
{
    for (const TypeName &entry : TYPE_NAMES) {
        if (name == QLatin1String(entry.name)) {
            if (type)
                *type = entry.type;
            return true;
        }
    }
    return false;
}

QRectF Annotation::rect() const
{
    if (points.isEmpty())
        return QRectF();

    return QRectF(points.first(), points.last()).normalized();
}

QFont Annotation::font() const
{
    QFont f = QGuiApplication::font();
    f.setPixelSize(qMax(4, qRound(width)));
    f.setBold(bold);
    return f;
}

QRectF Annotation::textRect() const
{
    if (points.isEmpty())
        return QRectF();

    const QFontMetricsF metrics(font());
    // An empty line still takes the height of one line.
    const QString shown = text.isEmpty() ? QStringLiteral(" ") : text;
    QRectF r = metrics.boundingRect(QRectF(0, 0, 100000, 100000), Qt::AlignLeft | Qt::AlignTop, shown);
    r.moveTopLeft(points.first());
    return r.adjusted(-TEXT_MARGIN, -TEXT_MARGIN, TEXT_MARGIN, TEXT_MARGIN);
}

qreal Annotation::counterRadius() const
{
    return 10 + width * 1.5;
}

qreal Annotation::arrowHeadSize() const
{
    return qMax<qreal>(12, width * 3.5);
}

QPainterPath Annotation::shape() const
{
    QPainterPath path;

    if (points.isEmpty())
        return path;

    switch (type) {
    case Pencil:
    case Marker:
        path.moveTo(points.first());
        for (int i = 1; i < points.size(); ++i)
            path.lineTo(points.at(i));
        if (points.size() == 1)
            path.lineTo(points.first() + QPointF(0.01, 0.01));
        break;
    case Line:
    case Arrow:
        path.moveTo(points.first());
        path.lineTo(points.last());
        break;
    case Rectangle:
        path.addRect(rect());
        break;
    case Ellipse:
        path.addEllipse(rect());
        break;
    case FilledRectangle:
    case Pixelate:
    case Blur:
    case Invert:
        path.addRect(rect());
        break;
    case Text:
        path.addRect(textRect());
        break;
    case Counter: {
        const qreal r = counterRadius();
        path.addEllipse(points.first(), r, r);
        break;
    }
    }

    return path;
}

QRectF Annotation::boundingRect() const
{
    if (points.isEmpty())
        return QRectF();

    switch (type) {
    case Text:
        return textRect();
    case Counter: {
        const qreal r = counterRadius();
        return QRectF(points.first() - QPointF(r, r), QSizeF(r * 2, r * 2));
    }
    case FilledRectangle:
    case Pixelate:
    case Blur:
    case Invert:
        return rect();
    default:
        break;
    }

    qreal pad = width / 2 + 1;
    if (type == Marker)
        pad = width * 1.5 + 1;
    if (type == Arrow)
        pad = qMax(pad, arrowHeadSize());

    return shape().boundingRect().adjusted(-pad, -pad, pad, pad);
}

bool Annotation::contains(const QPointF &pos, qreal tolerance) const
{
    switch (type) {
    case FilledRectangle:
    case Pixelate:
    case Blur:
    case Invert:
    case Text:
    case Counter:
        return shape().boundingRect().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(pos)
                && (type != Counter || QLineF(points.first(), pos).length() <= counterRadius() + tolerance);
    default:
        break;
    }

    QPainterPathStroker stroker;
    qreal strokeWidth = width;
    if (type == Marker)
        strokeWidth = width * 3;
    stroker.setWidth(strokeWidth + tolerance * 2);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(shape()).contains(pos);
}

void Annotation::translate(const QPointF &delta)
{
    for (QPointF &p : points)
        p += delta;
}

bool Annotation::isEmpty() const
{
    if (points.isEmpty())
        return true;

    switch (type) {
    case Text:
        return text.trimmed().isEmpty();
    case Counter:
        return false;
    case Pencil:
    case Marker:
        // A single click still leaves a dot.
        return false;
    case Line:
    case Arrow:
        return QLineF(points.first(), points.last()).length() < 2;
    default: {
        const QRectF r = rect();
        return r.width() < 2 || r.height() < 2;
    }
    }
}

QJsonObject Annotation::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("type"), typeName(type));
    object.insert(QStringLiteral("color"), color.name(QColor::HexArgb));
    object.insert(QStringLiteral("width"), width);

    QJsonArray pts;
    for (const QPointF &p : points) {
        pts.append(p.x());
        pts.append(p.y());
    }
    object.insert(QStringLiteral("points"), pts);

    if (type == Text) {
        object.insert(QStringLiteral("text"), text);
        object.insert(QStringLiteral("bold"), bold);
    }
    if (type == Counter)
        object.insert(QStringLiteral("number"), number);

    return object;
}

bool Annotation::fromJson(const QJsonObject &object, Annotation *result)
{
    Annotation a;

    if (!typeFromName(object.value(QStringLiteral("type")).toString(), &a.type))
        return false;

    a.color = QColor(object.value(QStringLiteral("color")).toString());
    if (!a.color.isValid())
        return false;

    a.width = object.value(QStringLiteral("width")).toDouble(4);
    if (a.width <= 0 || a.width > 1000)
        return false;

    const QJsonArray pts = object.value(QStringLiteral("points")).toArray();
    if (pts.isEmpty() || pts.size() % 2 != 0)
        return false;
    for (int i = 0; i + 1 < pts.size(); i += 2)
        a.points.append(QPointF(pts.at(i).toDouble(), pts.at(i + 1).toDouble()));

    a.text = object.value(QStringLiteral("text")).toString();
    a.bold = object.value(QStringLiteral("bold")).toBool();
    a.number = object.value(QStringLiteral("number")).toInt();

    if (result)
        *result = a;
    return true;
}

bool Annotation::operator==(const Annotation &other) const
{
    return type == other.type && color == other.color && qFuzzyCompare(width, other.width)
            && points == other.points && text == other.text && bold == other.bold
            && number == other.number;
}
