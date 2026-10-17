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

#include "annotationmodel.h"

#include <QJsonObject>
#include <QLineF>
#include <QtMath>

namespace {

// Keeps memory bounded on very long editing sessions.
const int MAX_UNDO = 200;

QPointF snapAngle(const QPointF &from, const QPointF &to)
{
    const QLineF line(from, to);
    const qreal step = 45.0;
    QLineF snapped = line;
    snapped.setAngle(qRound(line.angle() / step) * step);
    return snapped.p2();
}

QPointF squareCorner(const QPointF &from, const QPointF &to)
{
    const qreal dx = to.x() - from.x();
    const qreal dy = to.y() - from.y();
    const qreal side = qMax(qAbs(dx), qAbs(dy));
    return QPointF(from.x() + (dx < 0 ? -side : side), from.y() + (dy < 0 ? -side : side));
}

}

AnnotationModel::AnnotationModel(QObject *parent)
    : QObject(parent)
    , m_drawing(false)
    , m_selected(-1)
    , m_moving(false)
    , m_moved(false)
{
}

int AnnotationModel::nextNumber() const
{
    int highest = 0;
    for (const Annotation &a : m_items) {
        if (a.type == Annotation::Counter)
            highest = qMax(highest, a.number);
    }
    return highest + 1;
}

void AnnotationModel::setSelectedIndex(int index)
{
    if (index < -1 || index >= m_items.size())
        index = -1;

    if (index == m_selected)
        return;

    m_selected = index;
    emit selectionChanged();
}

QRectF AnnotationModel::selectedRect() const
{
    if (m_selected < 0 || m_selected >= m_items.size())
        return QRectF();

    return m_items.at(m_selected).boundingRect();
}

bool AnnotationModel::begin(const QString &type, const QPointF &pos, const QColor &color, qreal width)
{
    Annotation::Type t;
    if (!Annotation::typeFromName(type, &t) || t == Annotation::Text || t == Annotation::Counter)
        return false;

    m_current = Annotation();
    m_current.type = t;
    m_current.color = color;
    m_current.width = qMax<qreal>(1, width);
    m_current.points = { pos, pos };
    if (Annotation::isFreehand(t))
        m_current.points = { pos };
    m_anchor = pos;
    m_drawing = true;

    setSelectedIndex(-1);
    emit changed();
    return true;
}

void AnnotationModel::extend(const QPointF &pos, bool constrain)
{
    if (!m_drawing)
        return;

    if (Annotation::isFreehand(m_current.type)) {
        // Skip points that would not change the path.
        if (QLineF(m_current.points.last(), pos).length() < 0.75)
            return;
        m_current.points.append(pos);
    } else {
        QPointF end = pos;
        if (constrain) {
            if (m_current.type == Annotation::Line || m_current.type == Annotation::Arrow)
                end = snapAngle(m_anchor, pos);
            else
                end = squareCorner(m_anchor, pos);
        }
        m_current.points = { m_anchor, end };
    }

    emit changed();
}

bool AnnotationModel::finish()
{
    if (!m_drawing)
        return false;

    m_drawing = false;
    const bool keep = !m_current.isEmpty();

    if (keep) {
        push();
        m_items.append(m_current);
    }

    m_current = Annotation();
    emit changed();
    return keep;
}

void AnnotationModel::cancel()
{
    if (!m_drawing)
        return;

    m_drawing = false;
    m_current = Annotation();
    emit changed();
}

void AnnotationModel::addText(const QPointF &pos, const QString &text, const QColor &color, qreal size, bool bold)
{
    Annotation a;
    a.type = Annotation::Text;
    a.points = { pos };
    a.text = text;
    a.color = color;
    a.width = qMax<qreal>(4, size);
    a.bold = bold;

    if (a.isEmpty())
        return;

    push();
    m_items.append(a);
    emit changed();
}

void AnnotationModel::addCounter(const QPointF &pos, const QColor &color, qreal width)
{
    Annotation a;
    a.type = Annotation::Counter;
    a.points = { pos };
    a.color = color;
    a.width = qMax<qreal>(1, width);
    a.number = nextNumber();

    push();
    m_items.append(a);
    setSelectedIndex(-1);
    emit changed();
}

int AnnotationModel::addEffects(const QString &type, const QVariantList &rects, qreal width)
{
    Annotation::Type t;
    if (!Annotation::typeFromName(type, &t) || !Annotation::isEffect(t))
        return 0;

    QVector<Annotation> added;
    for (const QVariant &value : rects) {
        const QRectF r = value.toRectF();
        Annotation a;
        a.type = t;
        a.width = qMax<qreal>(1, width);
        a.points = { r.topLeft(), r.bottomRight() };
        if (!a.isEmpty())
            added.append(a);
    }

    if (added.isEmpty())
        return 0;

    push();
    m_items += added;
    emit changed();
    return added.size();
}

int AnnotationModel::itemAt(const QPointF &pos, qreal tolerance) const
{
    // Topmost first.
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items.at(i).contains(pos, tolerance))
            return i;
    }
    return -1;
}

void AnnotationModel::beginMove()
{
    if (m_selected < 0)
        return;

    m_moving = true;
    m_moved = false;
    m_beforeMove = m_items;
}

void AnnotationModel::moveSelected(const QPointF &delta)
{
    if (m_selected < 0 || m_selected >= m_items.size() || delta.isNull())
        return;

    if (!m_moving) {
        // A move without beginMove() (keyboard): one undo step per call.
        push();
    } else {
        m_moved = true;
    }

    m_items[m_selected].translate(delta);
    emit changed();
    emit selectionChanged();
}

void AnnotationModel::endMove()
{
    if (!m_moving)
        return;

    m_moving = false;
    if (m_moved) {
        m_undo.append(m_beforeMove);
        if (m_undo.size() > MAX_UNDO)
            m_undo.removeFirst();
        m_redo.clear();
        emit changed();
    }
    m_beforeMove.clear();
}

bool AnnotationModel::removeSelected()
{
    return removeAt(m_selected);
}

bool AnnotationModel::removeAt(int index)
{
    if (index < 0 || index >= m_items.size())
        return false;

    push();
    m_items.removeAt(index);
    m_selected = -1;
    emit selectionChanged();
    emit changed();
    return true;
}

void AnnotationModel::setSelectedColor(const QColor &color)
{
    if (m_selected < 0 || m_selected >= m_items.size() || m_items.at(m_selected).color == color)
        return;
    if (Annotation::isEffect(m_items.at(m_selected).type))
        return;

    push();
    m_items[m_selected].color = color;
    emit changed();
}

void AnnotationModel::setSelectedWidth(qreal width)
{
    if (m_selected < 0 || m_selected >= m_items.size())
        return;

    width = qBound<qreal>(1, width, 400);
    if (qFuzzyCompare(m_items.at(m_selected).width, width))
        return;

    push();
    m_items[m_selected].width = width;
    emit changed();
    emit selectionChanged();
}

QVariantMap AnnotationModel::takeText(int index)
{
    QVariantMap result;
    if (index < 0 || index >= m_items.size() || m_items.at(index).type != Annotation::Text)
        return result;

    const Annotation a = m_items.at(index);
    result.insert(QStringLiteral("x"), a.points.first().x());
    result.insert(QStringLiteral("y"), a.points.first().y());
    result.insert(QStringLiteral("text"), a.text);
    result.insert(QStringLiteral("color"), a.color);
    result.insert(QStringLiteral("size"), a.width);
    result.insert(QStringLiteral("bold"), a.bold);

    removeAt(index);
    return result;
}

QString AnnotationModel::typeAt(int index) const
{
    if (index < 0 || index >= m_items.size())
        return QString();
    return Annotation::typeName(m_items.at(index).type);
}

qreal AnnotationModel::widthAt(int index) const
{
    if (index < 0 || index >= m_items.size())
        return 0;
    return m_items.at(index).width;
}

void AnnotationModel::undo()
{
    if (m_drawing)
        cancel();
    if (m_undo.isEmpty())
        return;

    m_redo.append(m_items);
    restore(m_undo.takeLast());
}

void AnnotationModel::redo()
{
    if (m_redo.isEmpty())
        return;

    m_undo.append(m_items);
    restore(m_redo.takeLast());
}

void AnnotationModel::clear()
{
    if (m_items.isEmpty())
        return;

    push();
    restore({});
}

QJsonArray AnnotationModel::toJson() const
{
    QJsonArray array;
    for (const Annotation &a : m_items)
        array.append(a.toJson());
    return array;
}

void AnnotationModel::fromJson(const QJsonArray &array)
{
    QVector<Annotation> items;
    for (const QJsonValue &value : array) {
        Annotation a;
        if (Annotation::fromJson(value.toObject(), &a))
            items.append(a);
    }
    setItems(items);
}

void AnnotationModel::setItems(const QVector<Annotation> &items)
{
    m_undo.clear();
    m_redo.clear();
    restore(items);
}

void AnnotationModel::push()
{
    m_undo.append(m_items);
    if (m_undo.size() > MAX_UNDO)
        m_undo.removeFirst();
    m_redo.clear();
}

void AnnotationModel::restore(const QVector<Annotation> &items)
{
    m_items = items;
    if (m_selected >= m_items.size() || m_selected >= 0) {
        m_selected = -1;
        emit selectionChanged();
    }
    emit changed();
}
