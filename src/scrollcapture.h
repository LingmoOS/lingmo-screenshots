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

#ifndef SCROLLCAPTURE_H
#define SCROLLCAPTURE_H

#include <QImage>
#include <QObject>
#include <QPointer>
#include <QRect>

class ItemWindow;
class QScreen;
class QTimer;

// Long capture on X11: turns the mouse wheel over an area (XTEST), grabs it
// after every step and joins the pieces where they overlap. Ends when the
// content stops moving, at the size limit or when the user clicks "Done".
class ScrollCapture : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int capturedHeight READ capturedHeight NOTIFY progress)

public:
    explicit ScrollCapture(QObject *parent = nullptr);
    ~ScrollCapture() override;

    // nativeRect: root window coordinates, native pixels.
    void start(QScreen *screen, const QRect &nativeRect);
    bool running() const { return m_running; }
    int capturedHeight() const { return m_stitched.height(); }

    Q_INVOKABLE void stop();
    Q_INVOKABLE void cancel();

signals:
    void runningChanged();
    void progress();
    void finished(const QImage &image);
    void failed(const QString &message);

private:
    void step();
    QImage grab() const;
    void finish();
    void showIndicator();

    QPointer<QScreen> m_screen;
    QRect m_rect;
    QImage m_stitched;
    QImage m_last;
    int m_still;
    int m_steps;
    bool m_running;
    QTimer *m_timer;
    ItemWindow *m_indicator;
};

#endif // SCROLLCAPTURE_H
