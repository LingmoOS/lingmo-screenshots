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

// Recent captures: edit again, copy, pin, delete.
ColumnLayout {
    id: page
    spacing: 0

    LingmoUI.PageHeader {
        title: qsTr("History")
        subtitle: history.count > 0 ? qsTr("%n recent capture(s), kept only on this computer", "", history.count) : ""
        large: false
        leftPadding: LingmoUI.Units.largeSpacing * 2
        rightPadding: LingmoUI.Units.largeSpacing * 2
        Layout.fillWidth: true

        LingmoUI.ActionButton {
            visible: history.count > 0
            text: qsTr("Clear history")
            destructive: true
            subtle: true
            onClicked: clearDialog.open()
        }
    }

    GridView {
        id: grid

        readonly property int columns: Math.max(1, Math.floor(width / 260))

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: LingmoUI.Units.largeSpacing * 2 - LingmoUI.Units.smallSpacing
        Layout.rightMargin: LingmoUI.Units.largeSpacing * 2 - LingmoUI.Units.smallSpacing
        clip: true
        model: history
        cellWidth: Math.floor(width / columns)
        cellHeight: Math.round(cellWidth * 0.62) + 64
        boundsBehavior: Flickable.StopAtBounds
        topMargin: LingmoUI.Units.smallSpacing
        bottomMargin: LingmoUI.Units.largeSpacing
        visible: history.count > 0

        ScrollBar.vertical: ScrollBar {}
        LingmoUI.WheelHandler { target: grid }

        delegate: Item {
            id: cell

            required property string entryId
            required property url thumbnail
            required property string createdText
            required property int imageWidth
            required property int imageHeight
            required property string title
            required property string savedPath

            width: grid.cellWidth
            height: grid.cellHeight

            LingmoUI.Card {
                id: card
                anchors.fill: parent
                anchors.margins: LingmoUI.Units.smallSpacing
                clickable: true
                liftOnHover: true
                padding: LingmoUI.Units.smallSpacing

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Edit capture from %1").arg(cell.createdText)

                onClicked: history.edit(cell.entryId)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: LingmoUI.Units.smallSpacing
                    spacing: LingmoUI.Units.smallSpacing

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: LingmoUI.Theme.smallRadius
                        color: LingmoUI.Theme.alternateBackgroundColor
                        clip: true

                        Image {
                            anchors.fill: parent
                            anchors.margins: 1
                            source: cell.thumbnail
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            smooth: true
                            mipmap: true
                        }

                        // Quick actions on hover.
                        Row {
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: LingmoUI.Units.smallSpacing
                            spacing: 2
                            opacity: card.hovered ? 1 : 0
                            visible: opacity > 0

                            Behavior on opacity { NumberAnimation { duration: LingmoUI.Units.shortDuration } }

                            LingmoUI.IconButton {
                                flat: false
                                iconName: "edit-copy"
                                tipText: qsTr("Copy")
                                onClicked: {
                                    history.copy(cell.entryId)
                                    rootWindow.showPassiveNotification(qsTr("Copied to the clipboard"), "short")
                                }
                            }
                            LingmoUI.IconButton {
                                flat: false
                                iconName: "window-pin"
                                tipText: qsTr("Pin on the screen")
                                onClicked: history.pin(cell.entryId)
                            }
                            LingmoUI.IconButton {
                                flat: false
                                visible: cell.savedPath.length > 0
                                iconName: "folder-open"
                                tipText: qsTr("Show the saved file")
                                onClicked: history.showSaved(cell.entryId)
                            }
                            LingmoUI.IconButton {
                                flat: false
                                iconName: "edit-delete"
                                tipText: qsTr("Delete")
                                onClicked: history.remove(cell.entryId)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: LingmoUI.Units.smallSpacing

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                text: cell.title.length > 0 ? cell.title : cell.createdText
                                font.pointSize: LingmoUI.Units.fontBody
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: (cell.title.length > 0 ? cell.createdText + " · " : "")
                                      + "%1 × %2".arg(cell.imageWidth).arg(cell.imageHeight)
                                color: LingmoUI.Theme.secondaryTextColor
                                font.pointSize: LingmoUI.Units.fontCaption
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        LingmoUI.IconButton {
                            iconName: "draw-freehand"
                            tipText: qsTr("Edit")
                            onClicked: history.edit(cell.entryId)
                        }
                    }
                }
            }
        }
    }

    LingmoUI.EmptyState {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: history.count === 0
        iconName: config.historyEnabled ? "history" : "view-private"
        title: config.historyEnabled ? qsTr("No captures yet") : qsTr("History is off")
        text: config.historyEnabled
              ? qsTr("Captures you copy, save or pin show up here, so you can annotate them again.")
              : qsTr("Captures are not kept. Turn the history on to annotate them again later.")
        actionText: config.historyEnabled ? qsTr("New capture") : qsTr("Turn on")
        onActionTriggered: {
            if (config.historyEnabled)
                rootWindow.showPage("capture")
            else
                config.historyEnabled = true
        }
    }

    Dialog {
        id: clearDialog
        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("Clear history?")
        standardButtons: Dialog.NoButton

        ColumnLayout {
            spacing: LingmoUI.Units.largeSpacing

            Label {
                text: qsTr("The recent captures will be deleted. Files you saved are kept.")
                wrapMode: Text.WordWrap
                Layout.preferredWidth: 340
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: LingmoUI.Units.smallSpacing
                LingmoUI.ActionButton {
                    text: qsTr("Cancel")
                    onClicked: clearDialog.close()
                }
                LingmoUI.ActionButton {
                    text: qsTr("Clear")
                    destructive: true
                    onClicked: {
                        history.clear()
                        clearDialog.close()
                    }
                }
            }
        }
    }
}
