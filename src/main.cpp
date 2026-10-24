/*
 * Copyright (C) 2021 LingmoOS Team.
 *
 * Author:     Reion Wong <reionwong@gmail.com>
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

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTranslator>
#include <cstdio>

#include "application.h"
#include "commandline.h"
#include "screenrecorder.h"

namespace {

void loadTranslation(QCoreApplication *app)
{
    QString dir = qEnvironmentVariable("LINGMO_SCREENSHOT_TRANSLATIONS");
    if (dir.isEmpty())
        dir = QStringLiteral("/usr/share/lingmo-screenshot/translations");

    const QString qmFilePath = QStringLiteral("%1/%2.qm").arg(dir, QLocale::system().name());
    if (!QFile::exists(qmFilePath))
        return;

    auto *translator = new QTranslator(app);
    if (translator->load(qmFilePath))
        QCoreApplication::installTranslator(translator);
    else
        delete translator;
}

QStringList arguments(int argc, char *argv[])
{
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);
    return args;
}

}

int main(int argc, char *argv[])
{
    const QStringList args = arguments(argc, argv);
#ifdef HAVE_LINGMO_INTENTS
    // D-Bus activation for the Lingmo Intents (screenshot.capture, screen.record.*,
    // screenshot.ocr): an instance without windows that leaves when idle
    if (args == QStringList{QStringLiteral("--intents")}) {
        QApplication app(argc, argv);
        app.setOrganizationName(QStringLiteral("lingmoos"));
        app.setApplicationName(QStringLiteral("lingmo-screenshot"));
        app.setDesktopFileName(QStringLiteral("lingmo-screenshot"));
        app.setQuitOnLastWindowClosed(false);
        loadTranslation(&app);
        Application application;
        application.setIntentsOnly(true);
        if (!application.registerService())
            return 0;
        return app.exec();
    }
#endif
    CommandLine::Request request;
    QString error;
    const bool parsed = CommandLine::parse(args, &request, &error);

    // Handled without a display connection.
    if (!parsed || request.command == CommandLine::Help || request.command == CommandLine::Version
            || request.command == CommandLine::StopRecording) {
        QCoreApplication app(argc, argv);
        app.setOrganizationName(QStringLiteral("lingmoos"));
        app.setApplicationName(QStringLiteral("lingmo-screenshot"));
        loadTranslation(&app);

        if (!parsed) {
            fprintf(stderr, "%s\n\n%s", qPrintable(error), qPrintable(CommandLine::helpText()));
            return 2;
        }
        if (request.command == CommandLine::Help) {
            printf("%s", qPrintable(CommandLine::helpText()));
            return 0;
        }
        if (request.command == CommandLine::Version) {
            printf("lingmo-screenshot 3.1\n");
            return 0;
        }
        if (!ScreenRecorder::stopRunningRecording()) {
            fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "No screen recording is in progress.")));
            return 1;
        }
        return 0;
    }

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("lingmoos"));
    app.setApplicationName(QStringLiteral("lingmo-screenshot"));
    app.setApplicationDisplayName(QCoreApplication::translate("main", "Screenshot"));
    app.setDesktopFileName(QStringLiteral("lingmo-screenshot"));
    app.setQuitOnLastWindowClosed(false);
    loadTranslation(&app);

    // The instance may run in another directory.
    if (!request.path.isEmpty()) {
        const bool folder = request.path.endsWith(QLatin1Char('/'));
        QString path = request.path;
        if (path.startsWith(QLatin1String("~/")))
            path = QDir::homePath() + path.mid(1);
        request.path = QDir::cleanPath(QDir::current().absoluteFilePath(path)) + (folder ? QStringLiteral("/") : QString());
    }

    if (request.interactive() && Application::forward(request.toArguments()))
        return 0;

    Application application;

    if (request.interactive() && !application.registerService()) {
        // Another instance started at the same moment.
        if (Application::forward(request.toArguments()))
            return 0;
    }

    if (!application.handle(request))
        return 1;

    return app.exec();
}
