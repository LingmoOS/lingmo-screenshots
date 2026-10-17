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

#include "desktopapps.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

#include <algorithm>

namespace {

struct Entry {
    QHash<QString, QString> values;

    QString value(const QString &key) const { return values.value(key); }

    QString localized(const QString &key) const
    {
        const QString locale = QLocale::system().name();          // pt_BR
        const QString lang = locale.section(QLatin1Char('_'), 0, 0); // pt
        for (const QString &k : { key + QLatin1Char('[') + locale + QLatin1Char(']'),
                                  key + QLatin1Char('[') + lang + QLatin1Char(']'), key }) {
            if (values.contains(k))
                return values.value(k);
        }
        return QString();
    }
};

bool readEntry(const QString &path, Entry *entry)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    bool inGroup = false;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1Char('['))) {
            if (inGroup)
                break;
            inGroup = line == QLatin1String("[Desktop Entry]");
            continue;
        }
        if (!inGroup)
            continue;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        entry->values.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
    }

    return !entry->values.isEmpty();
}

QString defaultApp(const QString &mimeType)
{
    QProcess p;
    p.start(QStringLiteral("xdg-mime"), { QStringLiteral("query"), QStringLiteral("default"), mimeType });
    if (!p.waitForFinished(2000) || p.exitCode() != 0)
        return QString();
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

}

QList<DesktopApps::App> DesktopApps::appsFor(const QString &mimeType)
{
    QList<App> apps;
    QSet<QString> seen;

    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString &dir : dirs) {
        QDirIterator it(dir, { QStringLiteral("*.desktop") }, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            // Desktop file ids use '-' for subdirectories.
            const QString id = QDir(dir).relativeFilePath(path).replace(QLatin1Char('/'), QLatin1Char('-'));
            // The first directory wins (user overrides system).
            if (seen.contains(id))
                continue;
            seen.insert(id);

            Entry e;
            if (!readEntry(path, &e))
                continue;
            if (e.value(QStringLiteral("Type")) != QLatin1String("Application")
                    || e.value(QStringLiteral("NoDisplay")) == QLatin1String("true")
                    || e.value(QStringLiteral("Hidden")) == QLatin1String("true"))
                continue;

            const QStringList mimes = e.value(QStringLiteral("MimeType")).split(QLatin1Char(';'), Qt::SkipEmptyParts);
            if (!mimes.contains(mimeType))
                continue;

            const QString tryExec = e.value(QStringLiteral("TryExec"));
            if (!tryExec.isEmpty() && QStandardPaths::findExecutable(tryExec).isEmpty() && !QFileInfo(tryExec).isExecutable())
                continue;

            App app;
            app.id = id;
            app.name = e.localized(QStringLiteral("Name"));
            app.icon = e.value(QStringLiteral("Icon"));
            app.exec = e.value(QStringLiteral("Exec"));
            // Ourselves: opening a capture in the capture tool makes no sense.
            if (app.name.isEmpty() || app.exec.isEmpty() || app.exec.startsWith(QLatin1String("lingmo-screenshot")))
                continue;
            apps.append(app);
        }
    }

    std::sort(apps.begin(), apps.end(), [](const App &a, const App &b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });

    const QString def = defaultApp(mimeType);
    for (int i = 0; i < apps.size(); ++i) {
        if (apps.at(i).id == def) {
            apps.move(i, 0);
            break;
        }
    }

    return apps;
}

bool DesktopApps::expandExec(const QString &exec, const QString &file, QString *program, QStringList *args)
{
    const QStringList parts = QProcess::splitCommand(exec);
    if (parts.isEmpty())
        return false;

    const QString url = QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded);
    QStringList out;
    bool used = false;

    for (int i = 1; i < parts.size(); ++i) {
        const QString part = parts.at(i);
        // Codes that stand for nothing here.
        if (part == QLatin1String("%i") || part == QLatin1String("%c") || part == QLatin1String("%k")
                || part == QLatin1String("%d") || part == QLatin1String("%D") || part == QLatin1String("%n")
                || part == QLatin1String("%N") || part == QLatin1String("%v") || part == QLatin1String("%m"))
            continue;

        QString value;
        for (int j = 0; j < part.size(); ++j) {
            const QChar c = part.at(j);
            if (c != QLatin1Char('%') || j + 1 >= part.size()) {
                value += c;
                continue;
            }
            const QChar code = part.at(++j);
            switch (code.unicode()) {
            case 'f': case 'F': value += file; used = true; break;
            case 'u': case 'U': value += url; used = true; break;
            case '%': value += QLatin1Char('%'); break;
            default: break; // unknown codes are dropped
            }
        }
        out.append(value);
    }

    // Applications without a field code get the file at the end.
    if (!used)
        out.append(file);

    if (program)
        *program = parts.first();
    if (args)
        *args = out;
    return true;
}

bool DesktopApps::launch(const App &app, const QString &file)
{
    QString program;
    QStringList args;
    if (!expandExec(app.exec, file, &program, &args))
        return false;

    return QProcess::startDetached(program, args, QDir::homePath());
}
