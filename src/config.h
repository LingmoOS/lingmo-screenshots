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

#ifndef CONFIG_H
#define CONFIG_H

#include <QColor>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QVariantList>

#include "beautify.h"

// User settings (~/.config/lingmoos/lingmo-screenshot.conf), shared by the
// capture overlay, the launcher window and the command line.
class Config : public QObject
{
    Q_OBJECT

    // Saving
    Q_PROPERTY(QString saveFolder READ saveFolder WRITE setSaveFolder NOTIFY changed)
    Q_PROPERTY(QString defaultSaveFolder READ defaultSaveFolder CONSTANT)
    Q_PROPERTY(QString fileNamePattern READ fileNamePattern WRITE setFileNamePattern NOTIFY changed)
    Q_PROPERTY(QString defaultFileNamePattern READ defaultFileNamePattern CONSTANT)
    Q_PROPERTY(QString imageFormat READ imageFormat WRITE setImageFormat NOTIFY changed)
    Q_PROPERTY(bool copyAfterSave READ copyAfterSave WRITE setCopyAfterSave NOTIFY changed)

    // Capture
    Q_PROPERTY(bool playSound READ playSound WRITE setPlaySound NOTIFY changed)
    Q_PROPERTY(bool showHelp READ showHelp WRITE setShowHelp NOTIFY changed)
    Q_PROPERTY(bool showMagnifier READ showMagnifier WRITE setShowMagnifier NOTIFY changed)
    Q_PROPERTY(bool smartSelection READ smartSelection WRITE setSmartSelection NOTIFY changed)

    // History
    Q_PROPERTY(bool historyEnabled READ historyEnabled WRITE setHistoryEnabled NOTIFY changed)
    Q_PROPERTY(int historySize READ historySize WRITE setHistorySize NOTIFY changed)

    // OCR
    Q_PROPERTY(QString ocrLanguages READ ocrLanguages WRITE setOcrLanguages NOTIFY changed)

    // "Send to…": off unless the user sets a command.
    Q_PROPERTY(QString uploadName READ uploadName WRITE setUploadName NOTIFY changed)
    Q_PROPERTY(QString uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY changed)
    Q_PROPERTY(bool uploadEnabled READ uploadEnabled NOTIFY changed)
    // Lingmo Share is installed: "Send to nearby device"
    Q_PROPERTY(bool nearbyShareAvailable READ nearbyShareAvailable CONSTANT)

    // Beautify
    Q_PROPERTY(int beautifyPadding READ beautifyPadding WRITE setBeautifyPadding NOTIFY changed)
    Q_PROPERTY(int beautifyRadius READ beautifyRadius WRITE setBeautifyRadius NOTIFY changed)
    Q_PROPERTY(bool beautifyShadow READ beautifyShadow WRITE setBeautifyShadow NOTIFY changed)
    Q_PROPERTY(int beautifyBackground READ beautifyBackground WRITE setBeautifyBackground NOTIFY changed)
    Q_PROPERTY(QVariantList beautifyBackgrounds READ beautifyBackgrounds CONSTANT)

    // Drawing tools
    Q_PROPERTY(QColor toolColor READ toolColor WRITE setToolColor NOTIFY changed)
    Q_PROPERTY(int toolWidth READ toolWidth WRITE setToolWidth NOTIFY changed)
    Q_PROPERTY(int textSize READ textSize WRITE setTextSize NOTIFY changed)
    Q_PROPERTY(bool textBold READ textBold WRITE setTextBold NOTIFY changed)
    Q_PROPERTY(QStringList recentColors READ recentColors NOTIFY changed)
    // Drawing colours offered by the colour picker.
    Q_PROPERTY(QStringList palette READ palette CONSTANT)

    // Recording
    Q_PROPERTY(bool recordMicrophone READ recordMicrophone WRITE setRecordMicrophone NOTIFY changed)
    Q_PROPERTY(bool recordSystemAudio READ recordSystemAudio WRITE setRecordSystemAudio NOTIFY changed)
    Q_PROPERTY(bool recordShowClicks READ recordShowClicks WRITE setRecordShowClicks NOTIFY changed)
    Q_PROPERTY(QString recordFormat READ recordFormat WRITE setRecordFormat NOTIFY changed)
    Q_PROPERTY(int recordFrameRate READ recordFrameRate WRITE setRecordFrameRate NOTIFY changed)

public:
    explicit Config(QObject *parent = nullptr);

    QString saveFolder() const;
    void setSaveFolder(const QString &value);
    QString defaultSaveFolder() const;
    QString fileNamePattern() const;
    void setFileNamePattern(const QString &value);
    QString defaultFileNamePattern() const;
    QString imageFormat() const;
    void setImageFormat(const QString &value);
    bool copyAfterSave() const;
    void setCopyAfterSave(bool value);

    bool playSound() const;
    void setPlaySound(bool value);
    bool showHelp() const;
    void setShowHelp(bool value);
    bool showMagnifier() const;
    void setShowMagnifier(bool value);
    bool smartSelection() const;
    void setSmartSelection(bool value);

    bool historyEnabled() const;
    void setHistoryEnabled(bool value);
    int historySize() const;
    void setHistorySize(int value);

    QString ocrLanguages() const;
    void setOcrLanguages(const QString &value);

    QString uploadName() const;
    void setUploadName(const QString &value);
    QString uploadCommand() const;
    void setUploadCommand(const QString &value);
    bool uploadEnabled() const;
    bool nearbyShareAvailable() const;

    int beautifyPadding() const;
    void setBeautifyPadding(int value);
    int beautifyRadius() const;
    void setBeautifyRadius(int value);
    bool beautifyShadow() const;
    void setBeautifyShadow(bool value);
    int beautifyBackground() const;
    void setBeautifyBackground(int value);
    QVariantList beautifyBackgrounds() const;
    Beautify::Options beautifyOptions() const;

    QColor toolColor() const;
    void setToolColor(const QColor &value);
    int toolWidth() const;
    void setToolWidth(int value);
    int textSize() const;
    void setTextSize(int value);
    bool textBold() const;
    void setTextBold(bool value);
    QStringList recentColors() const;
    QStringList palette() const;
    Q_INVOKABLE void addRecentColor(const QColor &color);

    bool recordMicrophone() const;
    void setRecordMicrophone(bool value);
    bool recordSystemAudio() const;
    void setRecordSystemAudio(bool value);
    bool recordShowClicks() const;
    void setRecordShowClicks(bool value);
    QString recordFormat() const;
    void setRecordFormat(const QString &value);
    int recordFrameRate() const;
    void setRecordFrameRate(int value);

    // A file name from the pattern, for the settings page preview.
    Q_INVOKABLE QString previewFileName(const QString &pattern) const;
    Q_INVOKABLE void chooseSaveFolder();
    Q_INVOKABLE void openSaveFolder();

signals:
    void changed();

private:
    QVariant value(const QString &key, const QVariant &def) const;
    void set(const QString &key, const QVariant &value);

    QSettings m_settings;
};

#endif // CONFIG_H
