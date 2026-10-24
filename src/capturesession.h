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

#ifndef CAPTURESESSION_H
#define CAPTURESESSION_H

#include <QColor>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QQuickImageProvider>
#include <QRect>
#include <QVariantList>

#include "annotationmodel.h"
#include "beautify.h"

class QScreen;

// Images shown by QML ("image://shots/<id>"): captures and pins. Nothing is
// written to /tmp, where other users could read it.
class ImageStore : public QQuickImageProvider
{
public:
    ImageStore();

    static QString add(const QImage &image);
    static void remove(const QString &id);
    static QString url(const QString &id);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    static QMutex s_mutex;
    static QHash<QString, QImage> s_images;
    static int s_serial;
};

// One capture being edited: the screenshot, its annotations and what is
// known about the screen it came from. All coordinates are image pixels.
class CaptureSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString imageUrl READ imageUrl CONSTANT)
    Q_PROPERTY(int imageWidth READ imageWidth CONSTANT)
    Q_PROPERTY(int imageHeight READ imageHeight CONSTANT)
    Q_PROPERTY(bool editor READ editor CONSTANT)
    Q_PROPERTY(AnnotationModel *annotations READ annotations CONSTANT)
    Q_PROPERTY(QVariantList windows READ windows CONSTANT)
    Q_PROPERTY(QString target READ target CONSTANT)
    Q_PROPERTY(bool video READ video CONSTANT)
    Q_PROPERTY(bool scroll READ scroll CONSTANT)
    Q_PROPERTY(bool ask READ ask CONSTANT)
    Q_PROPERTY(QString askQuestion READ askQuestion CONSTANT)
    Q_PROPERTY(QRect initialRegion READ initialRegion CONSTANT)
    Q_PROPERTY(QString acceptPath READ acceptPath CONSTANT)

public:
    struct WindowInfo {
        QRect rect;
        QString title;
    };

    explicit CaptureSession(const QImage &image, QObject *parent = nullptr);
    ~CaptureSession() override;

    QImage image() const { return m_image; }
    QString imageUrl() const;
    int imageWidth() const { return m_image.width(); }
    int imageHeight() const { return m_image.height(); }

    bool editor() const { return m_editor; }
    void setEditor(bool editor) { m_editor = editor; }

    AnnotationModel *annotations() const { return m_annotations; }

    // Screen the capture was taken from (null for the editor).
    QScreen *screen() const { return m_screen; }
    // Where the image is on the X root window, in native pixels.
    QRect nativeGeometry() const { return m_nativeGeometry; }
    void setSource(QScreen *screen, const QRect &nativeGeometry);

    void setWindows(const QList<WindowInfo> &windows) { m_windows = windows; }
    QVariantList windows() const;
    QString windowTitleAt(const QPoint &pos) const;

    // How the overlay starts.
    QString target() const { return m_target; }
    void setTarget(const QString &target) { m_target = target; }
    bool video() const { return m_video; }
    void setVideo(bool video) { m_video = video; }
    bool scroll() const { return m_scroll; }
    void setScroll(bool scroll) { m_scroll = scroll; }
    // "Ask about the screen": the overlay opens for a question to the AI
    bool ask() const { return m_ask; }
    void setAsk(bool ask) { m_ask = ask; }
    QString askQuestion() const { return m_askQuestion; }
    void setAskQuestion(const QString &question) { m_askQuestion = question; }
    QRect initialRegion() const { return m_initialRegion; }
    void setInitialRegion(const QRect &rect) { m_initialRegion = rect; }

    // Command line --path / --clipboard / --pin: what Enter does.
    QString acceptPath() const { return m_acceptPath; }
    bool acceptClipboard() const { return m_acceptClipboard; }
    bool acceptPin() const { return m_acceptPin; }
    void setAccept(const QString &path, bool clipboard, bool pin);

    // History entry this session belongs to, once it has one.
    QString historyId() const { return m_historyId; }
    void setHistoryId(const QString &id) { m_historyId = id; }
    // Offset of the annotations relative to the history image.
    QPoint historyOrigin() const { return m_historyOrigin; }
    void setHistoryOrigin(const QPoint &origin) { m_historyOrigin = origin; }

    Q_INVOKABLE QColor pixelColor(int x, int y) const;
    // Smart selection: boxes around the point, smallest first (UI regions
    // found in the image, then the window under the point).
    Q_INVOKABLE QVariantList regionsAt(int x, int y);
    Q_INVOKABLE QVariantList windowAt(int x, int y) const;

    // The final image of crop: annotations painted, beautified when
    // options is given.
    QImage render(const QRect &crop, const Beautify::Options *beautify = nullptr) const;

private:
    QImage m_image;
    QImage m_gray;
    QString m_imageId;
    bool m_editor;
    AnnotationModel *m_annotations;
    QPointer<QScreen> m_screen;
    QRect m_nativeGeometry;
    QList<WindowInfo> m_windows;
    QString m_target;
    bool m_video;
    bool m_scroll;
    bool m_ask = false;
    QString m_askQuestion;
    QRect m_initialRegion;
    QString m_acceptPath;
    bool m_acceptClipboard;
    bool m_acceptPin;
    QString m_historyId;
    QPoint m_historyOrigin;
};

#endif // CAPTURESESSION_H
