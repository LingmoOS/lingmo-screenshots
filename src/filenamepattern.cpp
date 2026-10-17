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

#include "filenamepattern.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString pad(int value, int width)
{
    return QStringLiteral("%1").arg(value, width, 10, QLatin1Char('0'));
}

QString sanitize(QString name)
{
    // '/' would create directories, NUL ends the string, the others upset
    // other systems when the file gets shared.
    static const QString forbidden = QStringLiteral("/\\:*?\"<>|");
    for (QChar &c : name) {
        if (forbidden.contains(c) || c.unicode() < 0x20)
            c = QLatin1Char('_');
    }

    name = name.trimmed();
    // Hidden files and "." / ".." are not what anybody wants.
    while (name.startsWith(QLatin1Char('.')))
        name.remove(0, 1);

    if (name.size() > 200)
        name.truncate(200);

    return name;
}

}

QString FileNamePattern::format(const QString &pattern, const Context &context)
{
    const QDateTime t = context.time.isValid() ? context.time : QDateTime::currentDateTime();
    const QDate d = t.date();
    const QTime tm = t.time();

    QString out;
    out.reserve(pattern.size() + 16);

    for (int i = 0; i < pattern.size(); ++i) {
        const QChar c = pattern.at(i);
        if (c != QLatin1Char('%') || i + 1 >= pattern.size()) {
            out += c;
            continue;
        }

        const QChar token = pattern.at(++i);
        switch (token.unicode()) {
        case 'Y': out += pad(d.year(), 4); break;
        case 'y': out += pad(d.year() % 100, 2); break;
        case 'm': out += pad(d.month(), 2); break;
        case 'd': out += pad(d.day(), 2); break;
        case 'H': out += pad(tm.hour(), 2); break;
        case 'M': out += pad(tm.minute(), 2); break;
        case 'S': out += pad(tm.second(), 2); break;
        case 'j': out += pad(d.dayOfYear(), 3); break;
        case 's': out += QString::number(t.toSecsSinceEpoch()); break;
        case 'w': out += QString::number(context.width); break;
        case 'h': out += QString::number(context.height); break;
        case 't': out += context.windowTitle; break;
        case '%': out += QLatin1Char('%'); break;
        default:
            out += c;
            out += token;
            break;
        }
    }

    out = sanitize(out);
    if (out.isEmpty())
        out = QStringLiteral("screenshot");
    return out;
}

QString FileNamePattern::uniquePath(const QString &dir, const QString &pattern, const Context &context,
                                    const QString &extension)
{
    const QString base = format(pattern, context);
    const QString ext = extension.isEmpty() ? QStringLiteral("png") : extension;
    const QDir directory(dir);

    QString path = directory.filePath(base + QLatin1Char('.') + ext);
    for (int i = 2; QFileInfo::exists(path) && i < 10000; ++i)
        path = directory.filePath(QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(ext));

    return path;
}

QString FileNamePattern::formatForPath(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("jpg") || suffix == QLatin1String("jpeg"))
        return QStringLiteral("jpg");
    if (suffix == QLatin1String("webp"))
        return QStringLiteral("webp");
    if (suffix == QLatin1String("bmp"))
        return QStringLiteral("bmp");
    return QStringLiteral("png");
}
