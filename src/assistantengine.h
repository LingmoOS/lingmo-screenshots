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

#ifndef ASSISTANTENGINE_H
#define ASSISTANTENGINE_H

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QVariantMap>

// The Lingmo Assistant engine (lingmo-assistant-engine, from the
// lingmo-assistant package), run with QProcess (argv, no shell). It speaks
// NDJSON on stdin/stdout: see docs/protocol.md in lingmo-assistant. Used by
// "Ask about the screen" (request ask_image), like Spotlight uses it for its
// AI answers, so ai.conf, the permissions and the history are the same as in
// the Assistant.
class AssistantEngine : public QObject
{
    Q_OBJECT

public:
    explicit AssistantEngine(QObject *parent = nullptr);
    ~AssistantEngine() override;

    // /usr/lib/lingmo-assistant/lingmo-assistant-engine, or
    // $LINGMO_ASSISTANT_ENGINE for development
    static QString enginePath();
    static bool isInstalled();

    bool isRunning() const;
    void send(const QJsonObject &request);

signals:
    void event(const QVariantMap &event);
    // The engine exited or couldn't start; message is user-facing
    void failed(const QString &message);

private:
    void start();
    void readOutput();

    QProcess m_process;
    QByteArray m_buffer;
    QList<QByteArray> m_queue;   // requests written before the process started
    bool m_stopping = false;
};

#endif // ASSISTANTENGINE_H
