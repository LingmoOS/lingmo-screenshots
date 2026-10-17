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

#include "historymodel.h"

#include <QBuffer>
#include <QClipboard>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

namespace {

const QString IMAGE_FILE = QStringLiteral("image.png");
const QString PREVIEW_FILE = QStringLiteral("preview.png");
const QString THUMB_FILE = QStringLiteral("thumb.png");
const QString ANNOTATIONS_FILE = QStringLiteral("annotations.json");
const QString META_FILE = QStringLiteral("meta.json");
const int THUMB_WIDTH = 480;

const QFile::Permissions PRIVATE_FILE = QFile::ReadOwner | QFile::WriteOwner;
const QFile::Permissions PRIVATE_DIR = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;

bool writePrivate(const QString &path, const QByteArray &data)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.setPermissions(PRIVATE_FILE);
    file.write(data);
    return file.commit();
}

QByteArray png(const QImage &image)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

bool validId(const QString &id)
{
    if (id.isEmpty() || id.size() > 64)
        return false;
    for (const QChar c : id) {
        if (!c.isLetterOrNumber() && c != QLatin1Char('-') && c != QLatin1Char('_'))
            return false;
    }
    return true;
}

}

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    reload();
}

QString HistoryModel::directory()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("history"));
}

QString HistoryModel::entryPath(const QString &id, const QString &file) const
{
    return QDir(directory()).filePath(id + QLatin1Char('/') + file);
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();

    const Entry &e = m_entries.at(index.row());
    switch (role) {
    case IdRole:
        return e.id;
    case ThumbnailRole: {
        QUrl url = QUrl::fromLocalFile(entryPath(e.id, THUMB_FILE));
        // A new version after editing must not come from the image cache.
        url.setQuery(QStringLiteral("v=%1").arg(e.version));
        return url;
    }
    case CreatedRole:
        return e.created;
    case CreatedTextRole: {
        const QDate today = QDate::currentDate();
        const QLocale locale;
        if (e.created.date() == today)
            return tr("Today, %1").arg(locale.toString(e.created.time(), QLocale::ShortFormat));
        if (e.created.date() == today.addDays(-1))
            return tr("Yesterday, %1").arg(locale.toString(e.created.time(), QLocale::ShortFormat));
        return locale.toString(e.created, QLocale::ShortFormat);
    }
    case WidthRole:
        return e.width;
    case HeightRole:
        return e.height;
    case TitleRole:
        return e.title;
    case SavedPathRole:
        return e.savedPath;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        { IdRole, "entryId" },
        { ThumbnailRole, "thumbnail" },
        { CreatedRole, "created" },
        { CreatedTextRole, "createdText" },
        { WidthRole, "imageWidth" },
        { HeightRole, "imageHeight" },
        { TitleRole, "title" },
        { SavedPathRole, "savedPath" },
    };
}

bool HistoryModel::readEntry(const QString &id, Entry *entry) const
{
    QFile file(entryPath(id, META_FILE));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonObject meta = QJsonDocument::fromJson(file.readAll()).object();
    entry->id = id;
    entry->created = QDateTime::fromString(meta.value(QStringLiteral("created")).toString(), Qt::ISODate);
    entry->width = meta.value(QStringLiteral("width")).toInt();
    entry->height = meta.value(QStringLiteral("height")).toInt();
    entry->title = meta.value(QStringLiteral("title")).toString();
    entry->savedPath = meta.value(QStringLiteral("savedPath")).toString();
    entry->version = QFileInfo(entryPath(id, THUMB_FILE)).lastModified().toMSecsSinceEpoch();

    return entry->created.isValid() && QFile::exists(entryPath(id, IMAGE_FILE));
}

bool HistoryModel::writeMeta(const Entry &entry) const
{
    QJsonObject meta;
    meta.insert(QStringLiteral("created"), entry.created.toString(Qt::ISODate));
    meta.insert(QStringLiteral("width"), entry.width);
    meta.insert(QStringLiteral("height"), entry.height);
    meta.insert(QStringLiteral("title"), entry.title);
    meta.insert(QStringLiteral("savedPath"), entry.savedPath);
    return writePrivate(entryPath(entry.id, META_FILE), QJsonDocument(meta).toJson());
}

int HistoryModel::indexOf(const QString &id) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).id == id)
            return i;
    }
    return -1;
}

void HistoryModel::reload()
{
    beginResetModel();
    m_entries.clear();

    const QDir dir(directory());
    const QStringList ids = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &id : ids) {
        Entry e;
        if (validId(id) && readEntry(id, &e))
            m_entries.append(e);
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const Entry &a, const Entry &b) {
        return a.created > b.created;
    });

    endResetModel();
    emit countChanged();
}

QString HistoryModel::add(const QImage &base, const QJsonArray &annotations, const QImage &preview,
                          const QString &title, int maxEntries)
{
    if (base.isNull())
        return QString();

    QDir root(directory());
    if (!root.mkpath(QStringLiteral(".")))
        return QString();
    QFile::setPermissions(root.path(), PRIVATE_DIR);

    Entry e;
    e.created = QDateTime::currentDateTime();
    e.id = e.created.toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"))
            + QStringLiteral("-%1").arg(QRandomGenerator::global()->bounded(0x10000), 4, 16, QLatin1Char('0'));
    e.width = base.width();
    e.height = base.height();
    e.title = title;

    if (!root.mkdir(e.id))
        return QString();
    QFile::setPermissions(root.filePath(e.id), PRIVATE_DIR);

    const QImage shown = preview.isNull() ? base : preview;
    const bool ok = writePrivate(entryPath(e.id, IMAGE_FILE), png(base))
            && writePrivate(entryPath(e.id, ANNOTATIONS_FILE), QJsonDocument(annotations).toJson(QJsonDocument::Compact))
            && writePrivate(entryPath(e.id, PREVIEW_FILE), png(shown))
            && writePrivate(entryPath(e.id, THUMB_FILE),
                            png(shown.width() > THUMB_WIDTH ? shown.scaledToWidth(THUMB_WIDTH, Qt::SmoothTransformation) : shown))
            && writeMeta(e);

    if (!ok) {
        QDir(root.filePath(e.id)).removeRecursively();
        return QString();
    }

    e.version = QDateTime::currentMSecsSinceEpoch();
    beginInsertRows(QModelIndex(), 0, 0);
    m_entries.prepend(e);
    endInsertRows();
    emit countChanged();

    trim(maxEntries);
    return e.id;
}

bool HistoryModel::update(const QString &id, const QJsonArray &annotations, const QImage &preview)
{
    const int row = indexOf(id);
    if (row < 0 || preview.isNull())
        return false;

    const bool ok = writePrivate(entryPath(id, ANNOTATIONS_FILE), QJsonDocument(annotations).toJson(QJsonDocument::Compact))
            && writePrivate(entryPath(id, PREVIEW_FILE), png(preview))
            && writePrivate(entryPath(id, THUMB_FILE),
                            png(preview.width() > THUMB_WIDTH ? preview.scaledToWidth(THUMB_WIDTH, Qt::SmoothTransformation) : preview));

    m_entries[row].version = QDateTime::currentMSecsSinceEpoch();
    emit dataChanged(index(row), index(row));
    return ok;
}

void HistoryModel::setSavedPath(const QString &id, const QString &path)
{
    const int row = indexOf(id);
    if (row < 0)
        return;

    m_entries[row].savedPath = path;
    writeMeta(m_entries.at(row));
    emit dataChanged(index(row), index(row), { SavedPathRole });
}

bool HistoryModel::load(const QString &id, QImage *base, QJsonArray *annotations) const
{
    if (!validId(id) || indexOf(id) < 0)
        return false;

    if (base && !base->load(entryPath(id, IMAGE_FILE), "PNG"))
        return false;

    if (annotations) {
        QFile file(entryPath(id, ANNOTATIONS_FILE));
        if (file.open(QIODevice::ReadOnly))
            *annotations = QJsonDocument::fromJson(file.readAll()).array();
    }
    return true;
}

QImage HistoryModel::preview(const QString &id) const
{
    if (!validId(id) || indexOf(id) < 0)
        return QImage();
    QImage image(entryPath(id, PREVIEW_FILE), "PNG");
    return image;
}

void HistoryModel::trim(int maxEntries)
{
    while (m_entries.size() > qMax(1, maxEntries))
        remove(m_entries.last().id);
}

bool HistoryModel::remove(const QString &id)
{
    const int row = indexOf(id);
    if (row < 0)
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();
    QDir(QDir(directory()).filePath(id)).removeRecursively();
    emit countChanged();
    return true;
}

void HistoryModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    QDir(directory()).removeRecursively();
    emit countChanged();
}

void HistoryModel::copy(const QString &id)
{
    const QImage image = preview(id);
    if (image.isNull())
        return;
    QGuiApplication::clipboard()->setImage(image);
    emit copied();
}

void HistoryModel::edit(const QString &id)
{
    if (indexOf(id) >= 0)
        emit editRequested(id);
}

void HistoryModel::pin(const QString &id)
{
    if (indexOf(id) >= 0)
        emit pinRequested(id);
}

void HistoryModel::showSaved(const QString &id)
{
    const int row = indexOf(id);
    if (row < 0 || m_entries.at(row).savedPath.isEmpty())
        return;

    const QString path = m_entries.at(row).savedPath;
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.FileManager1"),
                                                      QStringLiteral("/org/freedesktop/FileManager1"),
                                                      QStringLiteral("org.freedesktop.FileManager1"),
                                                      QStringLiteral("ShowItems"));
    msg << QStringList(QUrl::fromLocalFile(path).toString()) << QString();
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 3000);
    if (reply.type() != QDBusMessage::ReplyMessage)
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}
