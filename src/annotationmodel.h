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

#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

#include "annotation.h"

#include <QJsonArray>
#include <QObject>
#include <QVariantList>

// The drawing objects of a capture, with undo/redo. Every change that the
// user can undo goes through push(), which stores a snapshot of the list:
// annotations are small, so snapshots keep undo simple and always correct.
class AnnotationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(bool drawing READ drawing NOTIFY changed)
    Q_PROPERTY(int nextNumber READ nextNumber NOTIFY changed)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectionChanged)
    Q_PROPERTY(QRectF selectedRect READ selectedRect NOTIFY selectionChanged)

public:
    explicit AnnotationModel(QObject *parent = nullptr);

    int count() const { return m_items.size(); }
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    bool drawing() const { return m_drawing; }
    int nextNumber() const;

    int selectedIndex() const { return m_selected; }
    void setSelectedIndex(int index);
    QRectF selectedRect() const;

    const QVector<Annotation> &items() const { return m_items; }
    // The object being drawn right now, or nullptr.
    const Annotation *current() const { return m_drawing ? &m_current : nullptr; }

    // Freehand and box tools: press, drag, release.
    Q_INVOKABLE bool begin(const QString &type, const QPointF &pos, const QColor &color, qreal width);
    // constrain: lines snap to 45 degrees, boxes become squares.
    Q_INVOKABLE void extend(const QPointF &pos, bool constrain = false);
    Q_INVOKABLE bool finish();
    Q_INVOKABLE void cancel();

    Q_INVOKABLE void addText(const QPointF &pos, const QString &text, const QColor &color, qreal size, bool bold);
    Q_INVOKABLE void addCounter(const QPointF &pos, const QColor &color, qreal width);
    // Adds one effect box per rect (Quick redact) as a single undo step.
    Q_INVOKABLE int addEffects(const QString &type, const QVariantList &rects, qreal width);

    Q_INVOKABLE int itemAt(const QPointF &pos, qreal tolerance = 4) const;
    Q_INVOKABLE void beginMove();
    Q_INVOKABLE void moveSelected(const QPointF &delta);
    Q_INVOKABLE void endMove();
    Q_INVOKABLE bool removeSelected();
    Q_INVOKABLE bool removeAt(int index);
    // Restyles the selected object (color picker / wheel while an object is selected).
    Q_INVOKABLE void setSelectedColor(const QColor &color);
    Q_INVOKABLE void setSelectedWidth(qreal width);
    // Editing a text object: returns its properties and removes it (as an
    // undoable step, re-added by addText()).
    Q_INVOKABLE QVariantMap takeText(int index);
    Q_INVOKABLE QString typeAt(int index) const;
    Q_INVOKABLE qreal widthAt(int index) const;

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clear();

    QJsonArray toJson() const;
    // Replaces the content (no undo step). Invalid entries are skipped.
    void fromJson(const QJsonArray &array);
    void setItems(const QVector<Annotation> &items);

signals:
    void changed();
    void selectionChanged();

private:
    void push();
    void restore(const QVector<Annotation> &items);

    QVector<Annotation> m_items;
    QVector<QVector<Annotation>> m_undo;
    QVector<QVector<Annotation>> m_redo;

    Annotation m_current;
    bool m_drawing;
    QPointF m_anchor;

    int m_selected;
    bool m_moving;
    bool m_moved;
    QVector<Annotation> m_beforeMove;
};

#endif // ANNOTATIONMODEL_H
