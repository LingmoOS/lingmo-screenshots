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

#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <functional>

// Desktop notifications (org.freedesktop.Notifications) with actions. While
// a notification with actions is open, pending() is true so that the
// process stays alive to handle a click.
class Notifier : public QObject
{
    Q_OBJECT

public:
    using Callback = std::function<void(const QString &action)>;

    explicit Notifier(QObject *parent = nullptr);

    // actions: key, label, key, label...
    void notify(const QString &summary, const QString &body, const QStringList &actions = {},
                Callback callback = nullptr, const QString &imagePath = QString());

    bool pending() const { return !m_callbacks.isEmpty(); }

signals:
    void pendingChanged();

private slots:
    void onActionInvoked(uint id, const QString &action);
    void onClosed(uint id, uint reason);

private:
    void forget(uint id);

    QHash<uint, Callback> m_callbacks;
    bool m_connected;
};

#endif // NOTIFIER_H
