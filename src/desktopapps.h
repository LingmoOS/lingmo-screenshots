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

#ifndef DESKTOPAPPS_H
#define DESKTOPAPPS_H

#include <QList>
#include <QString>
#include <QStringList>

// "Open with": the installed applications that open images.
namespace DesktopApps
{
    struct App {
        QString id;      // desktop file name, "org.gimp.GIMP.desktop"
        QString name;
        QString icon;
        QString exec;
    };

    // Visible applications that declare the mime type, the default one first.
    QList<App> appsFor(const QString &mimeType);

    // Exec line of a desktop file with the field codes replaced for one file.
    bool expandExec(const QString &exec, const QString &file, QString *program, QStringList *args);

    bool launch(const App &app, const QString &file);
}

#endif // DESKTOPAPPS_H
