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

#include "ocr.h"

#include <QFile>
#include <QMap>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtMath>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcOcr, "lingmo.screenshot.ocr", QtInfoMsg)

namespace {

const char *STORE_HELPER = "/usr/lib/lingmo-store/lingmo-store-helper";
// Larger images are not upscaled: tesseract is slow enough on them already.
const qint64 UPSCALE_LIMIT = 4000000;

}

QVector<Ocr::Word> Ocr::parseTsv(const QByteArray &tsv)
{
    QVector<Word> words;
    const QList<QByteArray> lines = tsv.split('\n');

    for (const QByteArray &rawLine : lines) {
        QByteArray line = rawLine;
        if (line.endsWith('\r'))
            line.chop(1);

        const QList<QByteArray> cols = line.split('\t');
        // level page block par line word left top width height conf text
        if (cols.size() < 12)
            continue;

        bool ok = false;
        const int level = cols.at(0).toInt(&ok);
        if (!ok || level != 5)
            continue;

        Word w;
        w.block = cols.at(2).toInt();
        w.paragraph = cols.at(3).toInt();
        w.line = cols.at(4).toInt();
        const int left = cols.at(6).toInt();
        const int top = cols.at(7).toInt();
        const int width = cols.at(8).toInt();
        const int height = cols.at(9).toInt();
        w.confidence = qRound(cols.at(10).toDouble());
        // The text itself may contain tabs only in theory; keep the rest.
        w.text = QString::fromUtf8(cols.mid(11).join('\t')).trimmed();

        if (w.text.isEmpty() || width <= 0 || height <= 0)
            continue;

        w.box = QRect(left, top, width, height);
        words.append(w);
    }

    return words;
}

QVector<QRect> Ocr::redactBoxes(const QVector<Word> &words, int padding, int minConfidence)
{
    // Key: block/paragraph/line, in reading order.
    QMap<qint64, QRect> lines;

    for (const Word &w : words) {
        if (w.confidence < minConfidence)
            continue;
        // Single characters with low confidence are usually icons or noise.
        if (w.text.size() == 1 && !w.text.at(0).isLetterOrNumber())
            continue;

        const qint64 key = (qint64(w.block) << 32) | (qint64(w.paragraph) << 16) | qint64(w.line);
        lines[key] = lines.value(key).united(w.box);
    }

    QVector<QRect> boxes;
    for (const QRect &r : std::as_const(lines))
        boxes.append(r.adjusted(-padding, -padding, padding, padding));

    return boxes;
}

QStringList Ocr::parseLanguages(const QByteArray &output)
{
    QStringList langs;
    const QList<QByteArray> lines = output.split('\n');

    for (const QByteArray &raw : lines) {
        const QString line = QString::fromUtf8(raw).trimmed();
        // "List of available languages in "/usr/share/tessdata/" (3):"
        if (line.isEmpty() || line.contains(QLatin1Char(' ')) || line.endsWith(QLatin1Char(':')))
            continue;
        if (line == QLatin1String("osd") || line == QLatin1String("equ"))
            continue;
        langs.append(line);
    }

    return langs;
}

QString Ocr::languageArgument(const QString &wanted, const QStringList &installed)
{
    QStringList result;
    const QStringList parts = wanted.split(QLatin1Char('+'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString lang = part.trimmed();
        if (installed.contains(lang) && !result.contains(lang))
            result.append(lang);
    }

    // Nothing that was asked for: use what there is.
    if (result.isEmpty() && !installed.isEmpty())
        result.append(installed.contains(QLatin1String("eng")) ? QStringLiteral("eng") : installed.first());

    return result.join(QLatin1Char('+'));
}

QRect Ocr::scaleBox(const QRect &box, qreal factor)
{
    if (factor <= 0)
        return box;

    const int left = qFloor(box.left() / factor);
    const int top = qFloor(box.top() / factor);
    const int right = qCeil((box.left() + box.width()) / factor);
    const int bottom = qCeil((box.top() + box.height()) / factor);
    return QRect(left, top, right - left, bottom - top);
}

OcrEngine::OcrEngine(QObject *parent)
    : QObject(parent)
    , m_wanted(QStringLiteral("por+eng"))
    , m_tempDir(nullptr)
    , m_scale(1)
{
    refresh();
}

OcrEngine::~OcrEngine()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    delete m_tempDir;
}

bool OcrEngine::hasLanguageData() const
{
    return !m_languages.isEmpty();
}

void OcrEngine::setWantedLanguages(const QString &languages)
{
    const QString value = languages.trimmed().isEmpty() ? QStringLiteral("por+eng") : languages.trimmed();
    if (value == m_wanted)
        return;
    m_wanted = value;
    emit stateChanged();
}

QStringList OcrEngine::missingPackages() const
{
    QStringList packages;
    if (!available())
        packages << QStringLiteral("tesseract");

    const QStringList parts = m_wanted.split(QLatin1Char('+'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString lang = part.trimmed();
        // Only plain language codes become package names.
        bool valid = !lang.isEmpty();
        for (const QChar c : lang)
            valid &= c.isLetterOrNumber() || c == QLatin1Char('_');
        if (valid && !m_languages.contains(lang))
            packages << QStringLiteral("tesseract-data-") + lang.toLower();
    }

    return packages;
}

void OcrEngine::refresh()
{
    m_tesseract = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
    m_languages.clear();

    if (!m_tesseract.isEmpty()) {
        QProcess p;
        p.start(m_tesseract, { QStringLiteral("--list-langs") });
        if (p.waitForFinished(5000) && p.exitStatus() == QProcess::NormalExit)
            m_languages = Ocr::parseLanguages(p.readAllStandardOutput() + p.readAllStandardError());
    }

    emit stateChanged();
}

void OcrEngine::install()
{
    if (m_installer)
        return;

    const QStringList packages = missingPackages();
    if (packages.isEmpty()) {
        emit installFinished(true, QString());
        return;
    }

    m_installer = new QProcess(this);
    m_installer->setProcessChannelMode(QProcess::MergedChannels);

    QStringList args;
    if (QFile::exists(QString::fromLatin1(STORE_HELPER)))
        args << QString::fromLatin1(STORE_HELPER) << QStringLiteral("install") << packages;
    else
        args << QStringLiteral("pacman") << QStringLiteral("-S") << QStringLiteral("--needed")
             << QStringLiteral("--noconfirm") << packages;

    qCInfo(lcOcr) << "Installing" << packages;

    connect(m_installer, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        const QByteArray output = m_installer->readAll();
        m_installer->deleteLater();
        m_installer = nullptr;
        emit installingChanged();
        refresh();

        const bool ok = status == QProcess::NormalExit && exitCode == 0 && available() && hasLanguageData();
        if (!ok)
            qCWarning(lcOcr).noquote() << "Install failed:" << exitCode << output.right(2000);

        // pkexec: 126 = dialog dismissed, 127 = not authorized.
        QString message;
        if (exitCode == 126 || exitCode == 127)
            message = tr("The installation was cancelled.");
        else if (!ok)
            message = tr("The text recognition packages could not be installed.");
        emit installFinished(ok, message);
    });
    connect(m_installer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_installer)
            return;
        m_installer->deleteLater();
        m_installer = nullptr;
        emit installingChanged();
        emit installFinished(false, tr("Could not start pkexec. Make sure polkit is installed."));
    });

    m_installer->start(QStringLiteral("pkexec"), args);
    emit installingChanged();
}

void OcrEngine::recognizeText(const QImage &image)
{
    run(image, false);
}

void OcrEngine::recognizeBoxes(const QImage &image)
{
    run(image, true);
}

void OcrEngine::cancel()
{
    if (!m_process)
        return;

    m_process->disconnect(this);
    m_process->kill();
    m_process->deleteLater();
    m_process = nullptr;
    emit busyChanged();
}

void OcrEngine::run(const QImage &image, bool boxes)
{
    if (m_process) {
        emit failed(tr("Text recognition is already running."));
        return;
    }

    if (!available() || !hasLanguageData()) {
        emit failed(tr("Text recognition needs tesseract and its language data."));
        return;
    }

    if (image.isNull()) {
        emit failed(tr("There is nothing to recognize."));
        return;
    }

    // Private folder: the capture may contain anything.
    delete m_tempDir;
    QString base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (base.isEmpty())
        base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_tempDir = new QTemporaryDir(base + QStringLiteral("/lingmo-screenshot-ocr-XXXXXX"));
    if (!m_tempDir->isValid()) {
        emit failed(tr("Unable to create a temporary file for text recognition."));
        return;
    }

    // Screen text is small for tesseract (made for ~300 dpi scans): doubling
    // the size and dropping the colours helps a lot.
    QImage input = image.convertToFormat(QImage::Format_Grayscale8);
    m_scale = 1;
    if (qint64(image.width()) * image.height() <= UPSCALE_LIMIT) {
        m_scale = 2;
        input = input.scaled(image.size() * 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    const QString file = m_tempDir->filePath(QStringLiteral("input.png"));
    if (!input.save(file, "PNG")) {
        emit failed(tr("Unable to create a temporary file for text recognition."));
        return;
    }

    QStringList args;
    args << file << QStringLiteral("stdout")
         << QStringLiteral("-l") << Ocr::languageArgument(m_wanted, m_languages)
         << QStringLiteral("--psm") << QStringLiteral("3");
    if (boxes)
        args << QStringLiteral("tsv");

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Several threads make small images slower, not faster.
    env.insert(QStringLiteral("OMP_THREAD_LIMIT"), QStringLiteral("1"));
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::finished, this, [this, boxes](int exitCode, QProcess::ExitStatus status) {
        QProcess *p = m_process;
        m_process = nullptr;
        const QByteArray out = p->readAllStandardOutput();
        const QByteArray err = p->readAllStandardError();
        p->deleteLater();
        delete m_tempDir;
        m_tempDir = nullptr;
        emit busyChanged();

        if (status != QProcess::NormalExit || exitCode != 0) {
            qCWarning(lcOcr).noquote() << "tesseract failed:" << err.right(1000);
            emit failed(tr("Text recognition failed."));
            return;
        }

        if (boxes) {
            QVector<QRect> result;
            const QVector<QRect> raw = Ocr::redactBoxes(Ocr::parseTsv(out), qRound(3 * m_scale));
            for (const QRect &r : raw)
                result.append(Ocr::scaleBox(r, m_scale));
            emit boxesReady(result);
        } else {
            QString text = QString::fromUtf8(out);
            // tesseract ends pages with a form feed.
            text.remove(QChar(0x0C));
            emit textReady(text.trimmed());
        }
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_process)
            return;
        m_process->deleteLater();
        m_process = nullptr;
        emit busyChanged();
        emit failed(tr("Unable to start tesseract."));
    });

    qCInfo(lcOcr).noquote() << "Running tesseract" << args.mid(1).join(QLatin1Char(' '));
    m_process->start(m_tesseract, args);
    emit busyChanged();
}
