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

#ifndef FILENAMEPATTERN_H
#define FILENAMEPATTERN_H

#include <QDateTime>
#include <QString>

// File names for saved captures, from a strftime-like pattern (the same
// tokens Flameshot understands):
//   %Y year  %y 2-digit year  %m month  %d day  %H hour  %M minute
//   %S second  %j day of year  %s unix time  %%  a literal %
//   %w width of the capture  %h height  %t window title (window captures)
// Anything else is kept as is. Characters that are not allowed in file names
// are replaced, and the result never contains a directory separator.
namespace FileNamePattern
{
    struct Context {
        QDateTime time;
        int width = 0;
        int height = 0;
        QString windowTitle;
    };

    QString format(const QString &pattern, const Context &context);

    // pattern formatted, the extension added, in dir, and made unique:
    // "name.png", then "name (2).png"...
    QString uniquePath(const QString &dir, const QString &pattern, const Context &context,
                       const QString &extension);

    // Image format from a file name, "png" when unknown.
    QString formatForPath(const QString &path);
}

#endif // FILENAMEPATTERN_H
