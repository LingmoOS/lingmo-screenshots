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

#ifndef OCR_H
#define OCR_H

#include <QImage>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QStringList>
#include <QVector>

class QProcess;
class QTemporaryDir;

// Text recognition with tesseract, run locally on the cropped capture.
// Nothing leaves the computer.
namespace Ocr
{
    struct Word {
        QRect box;
        QString text;
        int confidence = -1;
        int block = 0;
        int paragraph = 0;
        int line = 0;
    };

    // tesseract's "tsv" output: one word per level-5 row.
    QVector<Word> parseTsv(const QByteArray &tsv);
    // Boxes to hide for Quick redact: the words of each line merged into one
    // box (so the gaps don't leak word lengths), grown by padding.
    QVector<QRect> redactBoxes(const QVector<Word> &words, int padding, int minConfidence = 30);
    // `tesseract --list-langs` output.
    QStringList parseLanguages(const QByteArray &output);
    // The wanted languages ("por+eng") that are installed, joined with '+'.
    QString languageArgument(const QString &wanted, const QStringList &installed);
    // Scales a box from the (upscaled) image tesseract read back to the capture.
    QRect scaleBox(const QRect &box, qreal factor);
}

class OcrEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY stateChanged)
    Q_PROPERTY(bool hasLanguageData READ hasLanguageData NOTIFY stateChanged)
    Q_PROPERTY(QStringList languages READ languages NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool installing READ installing NOTIFY installingChanged)
    Q_PROPERTY(QString wantedLanguages READ wantedLanguages WRITE setWantedLanguages NOTIFY stateChanged)
    Q_PROPERTY(QStringList missingPackages READ missingPackages NOTIFY stateChanged)

public:
    explicit OcrEngine(QObject *parent = nullptr);
    ~OcrEngine() override;

    bool available() const { return !m_tesseract.isEmpty(); }
    bool hasLanguageData() const;
    QStringList languages() const { return m_languages; }
    bool busy() const { return m_process != nullptr; }
    bool installing() const { return m_installer != nullptr; }
    QString wantedLanguages() const { return m_wanted; }
    void setWantedLanguages(const QString &languages);
    // Arch packages needed for the wanted languages.
    QStringList missingPackages() const;

    Q_INVOKABLE void refresh();
    // Installs tesseract and the language data through polkit.
    Q_INVOKABLE void install();

    // Results come back through textReady / boxesReady / failed.
    void recognizeText(const QImage &image);
    void recognizeBoxes(const QImage &image);
    Q_INVOKABLE void cancel();

signals:
    void stateChanged();
    void busyChanged();
    void installingChanged();
    void installFinished(bool ok, const QString &message);
    void textReady(const QString &text);
    void boxesReady(const QVector<QRect> &boxes);
    void failed(const QString &message);

private:
    void run(const QImage &image, bool boxes);

    QString m_tesseract;
    QStringList m_languages;
    QString m_wanted;
    QPointer<QProcess> m_process;
    QPointer<QProcess> m_installer;
    QTemporaryDir *m_tempDir;
    qreal m_scale;
};

#endif // OCR_H
