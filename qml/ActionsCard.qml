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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// What to do with the capture (photo), the recording options (video) or
// the scrolling capture.
LingmoUI.Card {
    id: card

    required property Item editor

    elevation: 2
    padding: LingmoUI.Units.smallSpacing
    implicitWidth: row.implicitWidth + padding * 2
    implicitHeight: Math.max(row.implicitHeight + padding * 2, 44)

    // Keeps the pointer from reaching the capture below.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
    }

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 2

        // ------------------------------------------------ photo
        RowLayout {
            visible: !editor.videoMode && !editor.scrollMode
            spacing: 2

            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "lingmo-ocr"
                tipText: qsTr("Copy text (Ctrl+T)")
                onClicked: editor.act("ocr")
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "view-private"
                tipText: qsTr("Quick redact: hide all text (Ctrl+Shift+H)")
                onClicked: editor.act("redact")
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "color-gradient"
                tipText: qsTr("Presentation: background, rounded corners and shadow")
                checkable: true
                checked: view.beautify
                onClicked: view.beautify = !view.beautify
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 22
                Layout.leftMargin: LingmoUI.Units.smallSpacing / 2
                Layout.rightMargin: LingmoUI.Units.smallSpacing / 2
                color: LingmoUI.Theme.dividerColor
            }

            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "window-pin"
                tipText: qsTr("Pin on the screen (Ctrl+P)")
                onClicked: editor.act("pin")
            }
            LingmoUI.IconButton {
                id: openWithButton
                size: 32; iconSize: 18
                iconName: "document-open"
                tipText: qsTr("Open with…")
                onClicked: {
                    appsMenu.apps = view.openWithApps()
                    appsMenu.popup(openWithButton, 0, openWithButton.height + LingmoUI.Units.smallSpacing)
                }

                Menu {
                    id: appsMenu
                    property var apps: []

                    Repeater {
                        model: appsMenu.apps
                        delegate: MenuItem {
                            required property var modelData
                            text: modelData.name
                            icon.name: modelData.icon
                            onTriggered: editor.act("openWith", modelData.id)
                        }
                    }

                    MenuItem {
                        visible: appsMenu.apps.length === 0
                        height: visible ? implicitHeight : 0
                        enabled: false
                        text: qsTr("No application opens images")
                    }
                }
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                visible: config.uploadEnabled
                iconName: "document-send"
                tipText: config.uploadName.length > 0 ? qsTr("Send to %1").arg(config.uploadName) : qsTr("Send to…")
                onClicked: editor.act("upload")
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                visible: config.nearbyShareAvailable
                iconName: "smartphone"
                tipText: qsTr("Send to nearby device")
                onClicked: editor.act("sendNearby")
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "document-save-as"
                tipText: qsTr("Save as… (Ctrl+Shift+S)")
                onClicked: editor.act("saveAs")
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "document-save"
                tipText: qsTr("Save to %1 (Ctrl+S)").arg(config.saveFolder)
                onClicked: editor.act("save")
            }
        }

        // ------------------------------------------------ video
        RowLayout {
            visible: editor.videoMode
            spacing: 2

            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "audio-input-microphone"
                tipText: qsTr("Microphone")
                checkable: true
                checked: config.recordMicrophone && config.recordFormat !== "gif"
                enabled: config.recordFormat !== "gif"
                onClicked: config.recordMicrophone = !config.recordMicrophone
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "audio-volume-high"
                tipText: qsTr("System audio")
                checkable: true
                checked: config.recordSystemAudio && config.recordFormat !== "gif"
                enabled: config.recordFormat !== "gif"
                onClicked: config.recordSystemAudio = !config.recordSystemAudio
            }
            LingmoUI.IconButton {
                size: 32; iconSize: 18
                iconName: "highlight-pointer-spot"
                tipText: qsTr("Show clicks")
                checkable: true
                checked: config.recordShowClicks
                onClicked: config.recordShowClicks = !config.recordShowClicks
            }

            LingmoUI.SegmentedControl {
                Layout.leftMargin: LingmoUI.Units.smallSpacing
                model: ["MP4", "GIF"]
                currentIndex: config.recordFormat === "gif" ? 1 : 0
                onActivated: (index) => config.recordFormat = index === 1 ? "gif" : "mp4"
            }
        }

        // ------------------------------------------------ scrolling capture
        Label {
            visible: editor.scrollMode
            Layout.leftMargin: LingmoUI.Units.smallSpacing
            Layout.maximumWidth: 320
            text: qsTr("Select the part that scrolls. The page is scrolled for you; click Done to stop early.")
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 22
            Layout.leftMargin: LingmoUI.Units.smallSpacing / 2
            Layout.rightMargin: LingmoUI.Units.smallSpacing / 2
            color: LingmoUI.Theme.dividerColor
        }

        LingmoUI.IconButton {
            size: 32; iconSize: 18
            iconName: "window-close"
            tipText: qsTr("Cancel (Esc)")
            onClicked: view.cancel()
        }

        LingmoUI.ActionButton {
            Layout.leftMargin: 2
            primary: true
            text: editor.videoMode ? qsTr("Record")
                 : editor.scrollMode ? qsTr("Start")
                 : (capture.acceptPath.length > 0 ? qsTr("Save") : qsTr("Copy"))
            iconName: editor.videoMode ? "media-record" : editor.scrollMode ? "go-down" : "edit-copy"
            tipText: qsTr("Enter")
            onClicked: editor.act(editor.videoMode ? "record" : editor.scrollMode ? "scroll" : "accept")
        }
    }
}
