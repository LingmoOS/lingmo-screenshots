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

#include "commandline.h"

#include <QRegularExpression>

namespace {

struct CommandName {
    const char *name;
    CommandLine::Command command;
};

const CommandName COMMANDS[] = {
    { "gui", CommandLine::Gui },
    { "full", CommandLine::Full },
    { "screen", CommandLine::Screen },
    { "window", CommandLine::Window },
    { "scroll", CommandLine::Scroll },
    { "record", CommandLine::Record },
    { "launcher", CommandLine::Launcher },
    { "history", CommandLine::History },
    { "config", CommandLine::Settings },
    { "settings", CommandLine::Settings },
};

const char *commandName(CommandLine::Command command)
{
    for (const CommandName &c : COMMANDS) {
        if (c.command == command)
            return c.name;
    }
    return "gui";
}

QString tr(const char *text)
{
    return QCoreApplication::translate("CommandLine", text);
}

}

bool CommandLine::Request::interactive() const
{
    switch (command) {
    case Gui:
    case Scroll:
    case Record:
    case Launcher:
    case History:
    case Settings:
        return true;
    case Window:
        return !active;
    default:
        return false;
    }
}

QStringList CommandLine::Request::toArguments() const
{
    QStringList args;
    args << QString::fromLatin1(commandName(command));
    if (delayMs > 0)
        args << QStringLiteral("--delay") << QStringLiteral("%1ms").arg(delayMs);
    if (!path.isEmpty())
        args << QStringLiteral("--path") << path;
    if (clipboard)
        args << QStringLiteral("--clipboard");
    if (raw)
        args << QStringLiteral("--raw");
    if (pin)
        args << QStringLiteral("--pin");
    if (active)
        args << QStringLiteral("--active");
    if (screen >= 0)
        args << QStringLiteral("--number") << QString::number(screen);
    if (!region.isNull())
        args << QStringLiteral("--region")
             << QStringLiteral("%1,%2,%3,%4").arg(region.x()).arg(region.y()).arg(region.width()).arg(region.height());
    return args;
}

int CommandLine::parseDelay(const QString &value)
{
    QString v = value.trimmed().toLower();
    double factor = 1000;
    if (v.endsWith(QLatin1String("ms"))) {
        factor = 1;
        v.chop(2);
    } else if (v.endsWith(QLatin1Char('s'))) {
        v.chop(1);
    }

    bool ok = false;
    const double number = v.replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
    if (!ok || number < 0 || number * factor > 600000)
        return -1;
    return qRound(number * factor);
}

QRect CommandLine::parseRegion(const QString &value)
{
    static const QRegularExpression x11(QStringLiteral("^(\\d+)x(\\d+)([+-]\\d+)([+-]\\d+)$"));
    static const QRegularExpression list(QStringLiteral("^(-?\\d+),(-?\\d+),(\\d+),(\\d+)$"));

    QRegularExpressionMatch m = x11.match(value.trimmed());
    QRect r;
    if (m.hasMatch())
        r = QRect(m.captured(3).toInt(), m.captured(4).toInt(), m.captured(1).toInt(), m.captured(2).toInt());
    else if ((m = list.match(value.trimmed())).hasMatch())
        r = QRect(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt(), m.captured(4).toInt());

    if (r.width() < 1 || r.height() < 1)
        return QRect();
    return r;
}

bool CommandLine::parse(const QStringList &args, Request *request, QString *error)
{
    Request r;
    bool commandSeen = false;

    auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    for (int i = 0; i < args.size(); ++i) {
        const QString arg = args.at(i);

        // --name=value
        QString name = arg;
        QString inlineValue;
        bool hasInline = false;
        if (arg.startsWith(QLatin1String("--")) && arg.contains(QLatin1Char('='))) {
            name = arg.section(QLatin1Char('='), 0, 0);
            inlineValue = arg.section(QLatin1Char('='), 1);
            hasInline = true;
        }

        auto takeValue = [&](QString *out) {
            if (hasInline) {
                *out = inlineValue;
                return true;
            }
            if (i + 1 >= args.size())
                return false;
            *out = args.at(++i);
            return true;
        };

        if (name == QLatin1String("-h") || name == QLatin1String("--help")) {
            r.command = Help;
            commandSeen = true;
        } else if (name == QLatin1String("-v") || name == QLatin1String("--version")) {
            r.command = Version;
            commandSeen = true;
        } else if (name == QLatin1String("--stop-recording")) {
            r.command = StopRecording;
            commandSeen = true;
        } else if (name == QLatin1String("-d") || name == QLatin1String("--delay")) {
            QString v;
            if (!takeValue(&v))
                return fail(tr("--delay needs a number of seconds."));
            r.delayMs = parseDelay(v);
            if (r.delayMs < 0)
                return fail(tr("Invalid delay: %1").arg(v));
        } else if (name == QLatin1String("-p") || name == QLatin1String("--path")) {
            if (!takeValue(&r.path) || r.path.isEmpty())
                return fail(tr("--path needs a file or folder."));
        } else if (name == QLatin1String("-c") || name == QLatin1String("--clipboard")) {
            r.clipboard = true;
        } else if (name == QLatin1String("-r") || name == QLatin1String("--raw")) {
            r.raw = true;
        } else if (name == QLatin1String("--pin")) {
            r.pin = true;
        } else if (name == QLatin1String("-a") || name == QLatin1String("--active")) {
            r.active = true;
        } else if (name == QLatin1String("-n") || name == QLatin1String("--number")) {
            QString v;
            bool ok = false;
            if (takeValue(&v))
                r.screen = v.toInt(&ok);
            if (!ok || r.screen < 0)
                return fail(tr("--number needs a screen number (0, 1…)."));
        } else if (name == QLatin1String("--region")) {
            QString v;
            if (!takeValue(&v) || (r.region = parseRegion(v)).isNull())
                return fail(tr("Invalid region, use WIDTHxHEIGHT+X+Y."));
        } else if (!arg.startsWith(QLatin1Char('-')) && !commandSeen) {
            bool known = false;
            for (const CommandName &c : COMMANDS) {
                if (arg == QLatin1String(c.name)) {
                    r.command = c.command;
                    known = true;
                    break;
                }
            }
            if (!known)
                return fail(tr("Unknown command: %1").arg(arg));
            commandSeen = true;
        } else {
            return fail(tr("Unknown option: %1").arg(arg));
        }
    }

    if (r.raw && r.interactive())
        return fail(tr("--raw only works with full, screen and window --active."));

    if (request)
        *request = r;
    return true;
}

QString CommandLine::helpText()
{
    return tr(
        "Usage: lingmo-screenshot [command] [options]\n"
        "\n"
        "Commands:\n"
        "  gui          Select an area, a window or a screen and annotate it (default)\n"
        "  full         Capture all screens\n"
        "  screen       Capture the screen under the pointer (or --number N)\n"
        "  window       Click a window to capture it (--active: the focused window)\n"
        "  scroll       Select an area and capture it while it scrolls\n"
        "  record       Record the screen\n"
        "  launcher     Open the Screenshot window\n"
        "  history      Show the recent captures\n"
        "  config       Open the settings\n"
        "\n"
        "Options:\n"
        "  -d, --delay SECONDS    Wait before capturing (\"2\", \"1.5\", \"500ms\")\n"
        "  -p, --path PATH        Save to this file or folder\n"
        "  -c, --clipboard        Copy the capture to the clipboard\n"
        "  -r, --raw              Write the PNG to standard output\n"
        "      --pin              Pin the capture on the screen\n"
        "  -a, --active           window: capture the focused window\n"
        "  -n, --number N         screen: capture screen N\n"
        "      --region WxH+X+Y   gui: start with this area selected\n"
        "      --stop-recording   Stop the screen recording in progress\n"
        "  -h, --help             Show this help\n");
}
