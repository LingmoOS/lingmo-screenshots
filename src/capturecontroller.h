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

#ifndef CAPTURECONTROLLER_H
#define CAPTURECONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QRect>
#include <QVariantList>

class CaptureSession;
class ItemWindow;
class ScreenAsk;
class QScreen;
class QWindow;

// One capture on screen: the full screen overlay (select, annotate, act) or
// the editor window (a capture reopened from the history, a long capture).
// Exposed to QML as "view". Crop rectangles are image pixels.
class CaptureController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool beautify READ beautify WRITE setBeautify NOTIFY beautifyChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString busyText READ busyText NOTIFY busyChanged)
    Q_PROPERTY(bool overlay READ overlay CONSTANT)

public:
    explicit CaptureController(CaptureSession *session, QObject *parent = nullptr);
    ~CaptureController() override;

    CaptureSession *session() const { return m_session; }

    // Full screen overlay on screen (logical geometry geometry).
    void showOverlay(QScreen *screen);
    // A normal window.
    void showEditor(const QString &title);
    bool overlay() const { return m_overlay; }
    QWindow *window() const;

    bool beautify() const { return m_beautify; }
    void setBeautify(bool beautify);
    bool busy() const { return !m_busyText.isEmpty(); }
    QString busyText() const { return m_busyText; }

    // Enter / ✓: what the command line asked for, otherwise copy.
    Q_INVOKABLE void accept(const QRect &crop);
    Q_INVOKABLE void copy(const QRect &crop);
    Q_INVOKABLE void save(const QRect &crop);
    Q_INVOKABLE void saveAs(const QRect &crop);
    Q_INVOKABLE void pin(const QRect &crop);
    Q_INVOKABLE QVariantList openWithApps() const;
    Q_INVOKABLE void openWith(const QRect &crop, const QString &appId);
    Q_INVOKABLE void upload(const QRect &crop);
    Q_INVOKABLE void sendNearby(const QRect &crop);
    // OCR: textRecognized() or message().
    Q_INVOKABLE void recognizeText(const QRect &crop);
    Q_INVOKABLE void copyText(const QString &text);
    // Quick redact: pixelates the text found in crop.
    Q_INVOKABLE void redact(const QRect &crop);
    Q_INVOKABLE void record(const QRect &crop);
    Q_INVOKABLE void scrollCapture(const QRect &crop);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openSettings(const QString &page = QString());
    // A short sound, the "shutter".
    Q_INVOKABLE void playSound();

signals:
    void beautifyChanged();
    void busyChanged();
    // Feedback shown inside the overlay.
    void message(const QString &text, bool error);
    void textRecognized(const QString &text);
    // The capture is done (or cancelled): the controller goes away.
    void finished();

private:
    QImage finalImage(const QRect &crop) const;
    void remember(const QRect &crop, const QImage &image);
    void close();
    void setBusy(const QString &text);
    void hideForDialog();
    void showAgain();

    CaptureSession *m_session;
    bool m_overlay;
    bool m_beautify;
    bool m_closing;
    QString m_busyText;
    ScreenAsk *m_ask;
    QPointer<ItemWindow> m_view;
    QPointer<QWindow> m_editorWindow;
};

#endif // CAPTURECONTROLLER_H
