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

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QCoreApplication>
#include <QRect>
#include <QString>
#include <QStringList>

// lingmo-screenshot [command] [options]
namespace CommandLine
{
    enum Command {
        Gui,            // interactive capture (default)
        Full,           // all screens
        Screen,         // one screen
        Window,         // pick a window (or the active one with --active)
        Scroll,         // long capture of a scrolled area
        Record,         // interactive screen recording
        Launcher,       // the app window
        History,
        Settings,
        StopRecording,
        Help,
        Version,
    };

    struct Request {
        Command command = Gui;
        int delayMs = 0;
        QString path;           // file or folder to save to
        bool clipboard = false;
        bool raw = false;       // PNG on stdout
        bool pin = false;
        bool active = false;    // window: the focused window, no picking
        int screen = -1;        // screen: index, -1 = under the pointer
        QRect region;           // gui: preselected area, logical pixels of the screen

        // Commands that open a window are handed to the running instance.
        bool interactive() const;
        // The request back as arguments (sent over D-Bus to the instance).
        QStringList toArguments() const;
    };

    // args without the program name. On failure error is set.
    bool parse(const QStringList &args, Request *request, QString *error);

    // "1.5" seconds, "300ms", "2s". -1 when invalid.
    int parseDelay(const QString &value);
    // "WxH+X+Y" (X11 geometry) or "X,Y,W,H". Null when invalid.
    QRect parseRegion(const QString &value);

    QString helpText();
}

#endif // COMMANDLINE_H
