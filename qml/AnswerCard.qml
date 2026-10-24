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
import QtQuick.Window
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// "Pin": an answer about the screen kept on top of the other windows. Drag to
// move it; Esc or × closes it. Nothing is stored.
Window {
    id: card

    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    title: qsTr("Answer about the screen")
    width: 400
    height: Math.min(520, column.implicitHeight + LingmoUI.Units.largeSpacing * 2)

    LingmoUI.GlassSurface {
        id: glass
        anchors.fill: parent
        radius: LingmoUI.Theme.hugeRadius

        DragHandler {
            target: null
            onActiveChanged: if (active) card.startSystemMove()
        }

        Shortcut {
            sequences: [StandardKey.Cancel]
            onActivated: card.close()
        }

        ColumnLayout {
            id: column
            anchors.fill: parent
            anchors.margins: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: LingmoUI.Units.smallSpacing

                LingmoUI.IconItem {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    source: "lingmo-assistant"
                }
                Label {
                    color: LingmoUI.Theme.textColor
                    Layout.fillWidth: true
                    text: cardQuestion
                    font.bold: true
                    elide: Text.ElideRight
                }
                LingmoUI.IconButton {
                    iconColor: LingmoUI.Theme.textColor
                    size: 28; iconSize: 16
                    iconName: "edit-copy"
                    tipText: qsTr("Copy answer")
                    onClicked: {
                        answer.selectAll()
                        answer.copy()
                        answer.deselect()
                    }
                }
                LingmoUI.IconButton {
                    iconColor: LingmoUI.Theme.textColor
                    size: 28; iconSize: 16
                    iconName: "window-close"
                    tipText: qsTr("Close")
                    onClicked: card.close()
                }
            }

            ScrollView {
                id: scroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: answer.implicitHeight
                clip: true
                contentWidth: availableWidth

                TextEdit {
                    id: answer
                    width: scroll.availableWidth
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.MarkdownText
                    text: cardAnswer
                    color: LingmoUI.Theme.textColor
                    font.pointSize: LingmoUI.Units.fontBody
                    selectionColor: LingmoUI.Theme.highlightColor
                    selectedTextColor: LingmoUI.Theme.highlightedTextColor
                    onLinkActivated: (link) => {
                        if (/^https?:\/\//.test(link))
                            Qt.openUrlExternally(link)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: cardModel.length > 0 ? qsTr("Analyzed on your computer · %1").arg(cardModel)
                                           : qsTr("Analyzed on your computer")
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
                elide: Text.ElideRight
            }
        }
    }
}
