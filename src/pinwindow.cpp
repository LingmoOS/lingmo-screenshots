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

#include "pinwindow.h"
#include "application.h"
#include "capturesession.h"

#include <QJsonArray>
#include <QKeyEvent>
#include <QQuickItem>
#include <QScreen>

namespace {

// Room around the image for the hover frame.
const int BORDER = 1;

}

PinWindow::PinWindow(const QImage &image, const QPoint &pos, QScreen *screen)
    : m_image(image)
    , m_zoom(1.0)
    , m_opacity(1.0)
{
    m_image.setDevicePixelRatio(1);
    m_imageId = ImageStore::add(m_image);

    setTitle(tr("Pinned screenshot"));
    setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setColor(Qt::transparent);
    if (screen)
        setScreen(screen);

    QObject *object = Application::instance()->createQml(QStringLiteral("qrc:/qml/PinWindow.qml"),
                                                         { { QStringLiteral("pin"), QVariant::fromValue<QObject *>(this) } },
                                                         this);
    if (auto *item = qobject_cast<QQuickItem *>(object))
        setItem(item, false);
    else
        delete object;

    updateSize();
    setPosition(pos - QPoint(BORDER, BORDER));
}

PinWindow::~PinWindow()
{
    ImageStore::remove(m_imageId);
}

QString PinWindow::imageUrl() const
{
    return ImageStore::url(m_imageId);
}

void PinWindow::setZoom(qreal zoom)
{
    zoom = qBound(0.1, zoom, 8.0);
    if (qFuzzyCompare(zoom, m_zoom))
        return;
    m_zoom = zoom;
    updateSize();
    emit zoomChanged();
}

void PinWindow::setImageOpacity(qreal opacity)
{
    opacity = qBound(0.2, opacity, 1.0);
    if (qFuzzyCompare(opacity, m_opacity))
        return;
    m_opacity = opacity;
    emit imageOpacityChanged();
}

void PinWindow::updateSize()
{
    const qreal dpr = screen() ? screen()->devicePixelRatio() : 1.0;
    const QSize size = (QSizeF(m_image.size()) / dpr * m_zoom).toSize().expandedTo(QSize(8, 8));
    resize(size + QSize(BORDER * 2, BORDER * 2));
}

void PinWindow::copy()
{
    Application::instance()->copyImage(m_image);
}

void PinWindow::saveAs()
{
    FileNamePattern::Context context;
    context.time = QDateTime::currentDateTime();
    context.width = m_image.width();
    context.height = m_image.height();
    const QString saved = Application::instance()->saveImageAs(m_image, context);
    if (!saved.isEmpty())
        Application::instance()->notifySaved(saved, false);
}

void PinWindow::edit()
{
    Application::instance()->openEditor(m_image, QJsonArray(), QString(), tr("Pinned screenshot"));
    closePin();
}

void PinWindow::closePin()
{
    hide();
    emit closed();
    deleteLater();
}

void PinWindow::moveBy(int dx, int dy)
{
    setPosition(position() + QPoint(dx, dy));
}

void PinWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        closePin();
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        copy();
        return;
    }
    if (event->matches(QKeySequence::Save)) {
        saveAs();
        return;
    }
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        setZoom(m_zoom * 1.15);
        return;
    }
    if (event->key() == Qt::Key_Minus) {
        setZoom(m_zoom / 1.15);
        return;
    }
    if (event->key() == Qt::Key_0) {
        setZoom(1.0);
        return;
    }
    ItemWindow::keyPressEvent(event);
}
