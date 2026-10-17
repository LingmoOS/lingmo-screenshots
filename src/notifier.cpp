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

#include "notifier.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>
#include <QVariantMap>

namespace {

const char *SERVICE = "org.freedesktop.Notifications";
const char *PATH = "/org/freedesktop/Notifications";
const char *INTERFACE = "org.freedesktop.Notifications";

// Actions stop working after this, so the process can exit.
const int ACTION_TIMEOUT = 60 * 1000;

}

Notifier::Notifier(QObject *parent)
    : QObject(parent)
    , m_connected(false)
{
}

void Notifier::notify(const QString &summary, const QString &body, const QStringList &actions,
                      Callback callback, const QString &imagePath)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    if (!m_connected && callback) {
        bus.connect(SERVICE, PATH, INTERFACE, QStringLiteral("ActionInvoked"),
                    this, SLOT(onActionInvoked(uint, QString)));
        bus.connect(SERVICE, PATH, INTERFACE, QStringLiteral("NotificationClosed"),
                    this, SLOT(onClosed(uint, uint)));
        m_connected = true;
    }

    QVariantMap hints;
    hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("lingmo-screenshot"));
    if (!imagePath.isEmpty())
        hints.insert(QStringLiteral("image-path"), imagePath);

    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, INTERFACE, QStringLiteral("Notify"));
    msg << QStringLiteral("lingmo-screenshot") << uint(0) << QStringLiteral("lingmo-screenshot")
        << summary << body << (callback ? actions : QStringList()) << hints << int(callback ? -1 : 6000);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);

    if (!callback) {
        connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
        return;
    }

    // Keep the process alive until we know the id.
    m_callbacks.insert(0, nullptr);
    emit pendingChanged();

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, callback] {
        QDBusPendingReply<uint> reply = *watcher;
        watcher->deleteLater();
        m_callbacks.remove(0);

        if (reply.isError()) {
            emit pendingChanged();
            return;
        }

        const uint id = reply.value();
        m_callbacks.insert(id, callback);
        QTimer::singleShot(ACTION_TIMEOUT, this, [this, id] { forget(id); });
        emit pendingChanged();
    });
}

void Notifier::onActionInvoked(uint id, const QString &action)
{
    const Callback callback = m_callbacks.value(id);
    if (!callback)
        return;

    callback(action);
    // Leave a moment for the launched program to start.
    QTimer::singleShot(500, this, [this, id] { forget(id); });
}

void Notifier::onClosed(uint id, uint reason)
{
    Q_UNUSED(reason)
    if (m_callbacks.contains(id))
        QTimer::singleShot(500, this, [this, id] { forget(id); });
}

void Notifier::forget(uint id)
{
    if (m_callbacks.remove(id))
        emit pendingChanged();
}
