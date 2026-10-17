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

#include "scrollcapture.h"
#include "application.h"
#include "imageanalysis.h"
#include "x11windows.h"

#include <QGuiApplication>
#include <QPixmap>
#include "itemwindow.h"

#include <QQuickItem>
#include <QScreen>
#include <QSurfaceFormat>
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcScroll, "lingmo.screenshot.scroll", QtInfoMsg)

namespace {

// Wheel clicks per step: small enough to keep a large overlap.
const int CLICKS_PER_STEP = 3;
// Applications need a moment to repaint after scrolling.
const int SETTLE_TIME = 350;
const int MAX_HEIGHT = 30000;
const int MAX_STEPS = 400;

}

ScrollCapture::ScrollCapture(QObject *parent)
    : QObject(parent)
    , m_still(0)
    , m_steps(0)
    , m_running(false)
    , m_timer(new QTimer(this))
    , m_indicator(nullptr)
{
    m_timer->setSingleShot(true);
    m_timer->setInterval(SETTLE_TIME);
    connect(m_timer, &QTimer::timeout, this, &ScrollCapture::step);
}

ScrollCapture::~ScrollCapture()
{
    delete m_indicator;
}

QImage ScrollCapture::grab() const
{
    if (!m_screen)
        return QImage();

    const QRect nativeScreen(m_screen->geometry().topLeft(), m_screen->geometry().size() * m_screen->devicePixelRatio());
    QImage full = m_screen->grabWindow(0).toImage();
    full.setDevicePixelRatio(1);
    return full.copy(m_rect.translated(-nativeScreen.topLeft())).convertToFormat(QImage::Format_RGB32);
}

void ScrollCapture::start(QScreen *screen, const QRect &nativeRect)
{
    if (m_running || !screen)
        return;

    qCInfo(lcScroll) << "Scrolling capture of" << nativeRect;
    m_screen = screen;
    m_rect = nativeRect;
    m_still = 0;
    m_steps = 0;
    m_stitched = grab();
    m_last = m_stitched;
    if (!qEnvironmentVariableIsEmpty("LINGMO_SCREENSHOT_SCROLL_DEBUG"))
        m_last.save(qEnvironmentVariable("LINGMO_SCREENSHOT_SCROLL_DEBUG") + QStringLiteral("/step-000.png"));

    if (m_stitched.isNull()) {
        emit failed(tr("Unable to capture the screen."));
        return;
    }

    // Test the wheel before anything is shown.
    if (!X11Windows::scroll(m_rect.center(), 0)) {
        emit failed(tr("Scrolling capture needs the X11 XTEST extension."));
        return;
    }

    m_running = true;
    emit runningChanged();
    emit progress();
    showIndicator();

    X11Windows::scroll(m_rect.center(), CLICKS_PER_STEP);
    m_timer->start();
}

void ScrollCapture::step()
{
    if (!m_running)
        return;

    const QImage next = grab();
    const Stitcher::Match match = Stitcher::match(m_last, next);
    ++m_steps;
    qCInfo(lcScroll) << "step" << m_steps << "offset" << match.offset << "footer" << match.footer;
    static const QString debugDir = qEnvironmentVariable("LINGMO_SCREENSHOT_SCROLL_DEBUG");
    if (!debugDir.isEmpty())
        next.save(QStringLiteral("%1/step-%2.png").arg(debugDir).arg(m_steps, 3, 10, QLatin1Char('0')));

    if (match.offset == 0) {
        // Nothing moved: the end, or a slow application. Try twice more.
        if (++m_still >= 3) {
            finish();
            return;
        }
    } else if (match.offset < 0) {
        qCWarning(lcScroll) << "No overlap found after step" << m_steps;
        if (m_stitched.height() == m_last.height()) {
            cancel();
            emit failed(tr("The area did not scroll in a way that can be joined. Select only the "
                           "scrolling part of the window, without fixed headers."));
            return;
        }
        // Keep what was joined so far.
        finish();
        return;
    } else {
        m_still = 0;
        m_stitched = Stitcher::append(m_stitched, next, match);
        m_last = next;
        emit progress();
    }

    if (m_stitched.height() >= MAX_HEIGHT || m_steps >= MAX_STEPS) {
        finish();
        return;
    }

    X11Windows::scroll(m_rect.center(), CLICKS_PER_STEP);
    m_timer->start();
}

void ScrollCapture::stop()
{
    if (!m_running)
        return;
    finish();
}

void ScrollCapture::cancel()
{
    m_timer->stop();
    m_stitched = QImage();
    if (m_indicator)
        m_indicator->hide();
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
}

void ScrollCapture::finish()
{
    m_timer->stop();
    if (m_indicator)
        m_indicator->hide();

    const QImage result = m_stitched;
    m_stitched = QImage();
    m_last = QImage();
    m_running = false;
    emit runningChanged();

    if (!result.isNull())
        emit finished(result);
}

void ScrollCapture::showIndicator()
{
    if (!m_indicator) {
        m_indicator = new ItemWindow;
        m_indicator->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool
                              | Qt::WindowDoesNotAcceptFocus);
        m_indicator->setTitle(tr("Scrolling capture"));
        QSurfaceFormat format = m_indicator->format();
        format.setAlphaBufferSize(8);
        m_indicator->setFormat(format);
        m_indicator->setColor(Qt::transparent);
        QObject *object = Application::instance()->createQml(
                QStringLiteral("qrc:/qml/ScrollIndicator.qml"),
                { { QStringLiteral("scroller"), QVariant::fromValue<QObject *>(this) } }, this);
        if (auto *item = qobject_cast<QQuickItem *>(object))
            m_indicator->setItem(item, true);
        else
            delete object;
    }

    QScreen *screen = m_screen ? m_screen.data() : qGuiApp->primaryScreen();
    m_indicator->setScreen(screen);

    // Outside the captured area, so it is never part of the image.
    const QRect available = screen->availableGeometry();
    const qreal dpr = screen->devicePixelRatio();
    const QPoint origin = screen->geometry().topLeft();
    const QRect captured(origin + (m_rect.topLeft() - origin) / dpr, m_rect.size() / dpr);
    const QSize size = m_indicator->size();
    const int margin = 16;

    const QList<QPoint> candidates {
        QPoint(available.center().x() - size.width() / 2, available.bottom() - size.height() - margin),
        QPoint(available.center().x() - size.width() / 2, available.top() + margin),
        QPoint(available.right() - size.width() - margin, available.top() + margin),
        QPoint(available.left() + margin, available.top() + margin),
        QPoint(available.right() - size.width() - margin, available.bottom() - size.height() - margin),
        QPoint(available.left() + margin, available.bottom() - size.height() - margin),
    };

    QPoint pos = candidates.first();
    for (const QPoint &candidate : candidates) {
        if (!captured.intersects(QRect(candidate, size))) {
            pos = candidate;
            break;
        }
    }

    m_indicator->setPosition(pos);
    m_indicator->show();
}
