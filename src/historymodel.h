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

#ifndef HISTORYMODEL_H
#define HISTORYMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QImage>
#include <QJsonArray>

// Recent captures, kept privately in ~/.local/share/lingmoos/lingmo-screenshot/history:
// the capture without annotations, the annotations (so they can be edited
// again) and a preview. Folders are only readable by the user.
class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        ThumbnailRole,
        CreatedRole,
        CreatedTextRole,
        WidthRole,
        HeightRole,
        TitleRole,
        SavedPathRole,
    };

    explicit HistoryModel(QObject *parent = nullptr);

    static QString directory();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_entries.size(); }

    // Returns the new id, or an empty string on failure.
    QString add(const QImage &base, const QJsonArray &annotations, const QImage &preview,
                const QString &title, int maxEntries);
    bool update(const QString &id, const QJsonArray &annotations, const QImage &preview);
    void setSavedPath(const QString &id, const QString &path);

    bool load(const QString &id, QImage *base, QJsonArray *annotations) const;
    QImage preview(const QString &id) const;
    void trim(int maxEntries);

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool remove(const QString &id);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void copy(const QString &id);
    Q_INVOKABLE void edit(const QString &id);
    Q_INVOKABLE void pin(const QString &id);
    Q_INVOKABLE void showSaved(const QString &id);

signals:
    void countChanged();
    void editRequested(const QString &id);
    void pinRequested(const QString &id);
    void copied();

private:
    struct Entry {
        QString id;
        QDateTime created;
        int width = 0;
        int height = 0;
        QString title;
        QString savedPath;
        qint64 version = 0;
    };

    bool readEntry(const QString &id, Entry *entry) const;
    bool writeMeta(const Entry &entry) const;
    int indexOf(const QString &id) const;
    QString entryPath(const QString &id, const QString &file) const;

    QList<Entry> m_entries;
};

#endif // HISTORYMODEL_H
