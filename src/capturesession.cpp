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

#include "capturesession.h"
#include "annotationrenderer.h"
#include "imageanalysis.h"

#include <QMutexLocker>
#include <QScreen>

QMutex ImageStore::s_mutex;
QHash<QString, QImage> ImageStore::s_images;
int ImageStore::s_serial = 0;

ImageStore::ImageStore()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QString ImageStore::add(const QImage &image)
{
    QMutexLocker locker(&s_mutex);
    const QString id = QString::number(++s_serial);
    s_images.insert(id, image);
    return id;
}

void ImageStore::remove(const QString &id)
{
    QMutexLocker locker(&s_mutex);
    s_images.remove(id);
}

QString ImageStore::url(const QString &id)
{
    return QStringLiteral("image://shots/") + id;
}

QImage ImageStore::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image;
    {
        QMutexLocker locker(&s_mutex);
        image = s_images.value(id.section(QLatin1Char('/'), 0, 0));
    }

    if (size)
        *size = image.size();

    if (!image.isNull() && requestedSize.isValid() && !requestedSize.isEmpty()
            && (requestedSize.width() < image.width() || requestedSize.height() < image.height()))
        return image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return image;
}

CaptureSession::CaptureSession(const QImage &image, QObject *parent)
    : QObject(parent)
    , m_image(image.convertToFormat(QImage::Format_RGB32))
    , m_editor(false)
    , m_annotations(new AnnotationModel(this))
    , m_target(QStringLiteral("area"))
    , m_video(false)
    , m_scroll(false)
    , m_acceptClipboard(false)
    , m_acceptPin(false)
{
    m_image.setDevicePixelRatio(1);
    m_imageId = ImageStore::add(m_image);
}

CaptureSession::~CaptureSession()
{
    ImageStore::remove(m_imageId);
}

QString CaptureSession::imageUrl() const
{
    return ImageStore::url(m_imageId);
}

void CaptureSession::setSource(QScreen *screen, const QRect &nativeGeometry)
{
    m_screen = screen;
    m_nativeGeometry = nativeGeometry;
}

QVariantList CaptureSession::windows() const
{
    QVariantList list;
    for (const WindowInfo &w : m_windows) {
        QVariantMap map;
        map.insert(QStringLiteral("x"), w.rect.x());
        map.insert(QStringLiteral("y"), w.rect.y());
        map.insert(QStringLiteral("width"), w.rect.width());
        map.insert(QStringLiteral("height"), w.rect.height());
        map.insert(QStringLiteral("title"), w.title);
        list.append(map);
    }
    return list;
}

QString CaptureSession::windowTitleAt(const QPoint &pos) const
{
    for (const WindowInfo &w : m_windows) {
        if (w.rect.contains(pos))
            return w.title;
    }
    return QString();
}

void CaptureSession::setAccept(const QString &path, bool clipboard, bool pin)
{
    m_acceptPath = path;
    m_acceptClipboard = clipboard;
    m_acceptPin = pin;
}

QColor CaptureSession::pixelColor(int x, int y) const
{
    if (!m_image.rect().contains(x, y))
        return QColor();
    return m_image.pixelColor(x, y);
}

QVariantList CaptureSession::windowAt(int x, int y) const
{
    // Topmost first: the first hit is the visible window.
    for (const WindowInfo &w : m_windows) {
        if (w.rect.contains(x, y))
            return { QRect(w.rect) };
    }
    return {};
}

QVariantList CaptureSession::regionsAt(int x, int y)
{
    if (m_gray.isNull())
        m_gray = m_image.convertToFormat(QImage::Format_Grayscale8);

    QList<QRect> rects;
    const QPoint pos(x, y);
    const QVector<QRect> regions = RegionDetector::regionsAt(m_gray, pos);
    for (const QRect &r : regions)
        rects.append(r);

    for (const WindowInfo &w : m_windows) {
        if (w.rect.contains(pos)) {
            const QRect window = w.rect.intersected(m_image.rect());
            // UI regions are only interesting inside the window.
            QList<QRect> inside;
            for (const QRect &r : std::as_const(rects)) {
                if (window.contains(r) && r != window)
                    inside.append(r);
            }
            rects = inside;
            rects.append(window);
            break;
        }
    }

    QVariantList result;
    for (const QRect &r : std::as_const(rects)) {
        if (r.width() >= 8 && r.height() >= 8)
            result.append(r);
    }
    return result;
}

QImage CaptureSession::render(const QRect &crop, const Beautify::Options *beautify) const
{
    QImage result = AnnotationRenderer::compose(m_image, m_annotations->items(), crop);
    if (beautify && !result.isNull())
        result = Beautify::render(result, *beautify);
    return result;
}
