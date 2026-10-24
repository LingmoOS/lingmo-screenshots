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

#include "assistantengine.h"

#include <QFileInfo>
#include <QJsonDocument>

// Events are small; protects against a misbehaving engine
static constexpr int MaxBuffer = 8 * 1024 * 1024;

AssistantEngine::AssistantEngine(QObject *parent)
    : QObject(parent)
{
    // Its logs stay on our stderr; the protocol is on stdout only
    m_process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &AssistantEngine::readOutput);
    connect(&m_process, &QProcess::started, this, [this] {
        for (const QByteArray &line : std::as_const(m_queue))
            m_process.write(line);
        m_queue.clear();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_queue.clear();
            emit failed(tr("The assistant engine couldn't start. Install or reinstall Lingmo Assistant."));
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        m_buffer.clear();
        if (!m_stopping)
            emit failed(tr("The assistant engine stopped. Try again."));
    });
}

AssistantEngine::~AssistantEngine()
{
    m_stopping = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.closeWriteChannel();   // the engine exits on EOF
        if (!m_process.waitForFinished(1500))
            m_process.kill();
        m_process.waitForFinished(500);
    }
}

QString AssistantEngine::enginePath()
{
    const QString override = qEnvironmentVariable("LINGMO_ASSISTANT_ENGINE");
    return override.isEmpty() ? QStringLiteral("/usr/lib/lingmo-assistant/lingmo-assistant-engine") : override;
}

bool AssistantEngine::isInstalled()
{
    const QFileInfo info(enginePath());
    return info.isFile() && info.isExecutable();
}

bool AssistantEngine::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void AssistantEngine::start()
{
    if (isRunning())
        return;
    m_buffer.clear();
    m_process.start(enginePath(), {});
}

void AssistantEngine::send(const QJsonObject &request)
{
    const QByteArray line = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (m_process.state() == QProcess::Running) {
        m_process.write(line);
        return;
    }
    m_queue << line;
    start();
}

void AssistantEngine::readOutput()
{
    m_buffer += m_process.readAllStandardOutput();
    if (m_buffer.size() > MaxBuffer) {
        m_buffer.clear();
        return;
    }
    int newline;
    while ((newline = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        if (line.isEmpty())
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject())
            emit event(doc.object().toVariantMap());
    }
}
