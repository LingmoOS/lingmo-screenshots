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

// Drawing tools, colour, thickness, undo/redo.
LingmoUI.Card {
    id: card

    required property Item editor

    elevation: 2
    padding: LingmoUI.Units.smallSpacing
    implicitWidth: row.implicitWidth + padding * 2
    implicitHeight: row.implicitHeight + padding * 2

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

        Repeater {
            model: editor.tools

            delegate: LingmoUI.IconButton {
                required property var modelData
                size: 32
                iconSize: 18
                iconName: modelData.icon
                tipText: modelData.tip
                checkable: true
                checked: editor.tool === modelData.name
                onClicked: editor.setTool(modelData.name)
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 22
            Layout.leftMargin: LingmoUI.Units.smallSpacing / 2
            Layout.rightMargin: LingmoUI.Units.smallSpacing / 2
            color: LingmoUI.Theme.dividerColor
        }

        // Current colour and thickness; opens the colour picker.
        AbstractButton {
            id: colorButton
            implicitWidth: 32
            implicitHeight: 32
            hoverEnabled: true
            focusPolicy: Qt.NoFocus

            ToolTip.visible: hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Colour and thickness (right click on the capture)")

            background: Rectangle {
                radius: LingmoUI.Theme.smallRadius
                color: colorButton.pressed ? LingmoUI.Theme.pressedColor
                     : colorButton.hovered ? LingmoUI.Theme.hoverColor : "transparent"
                Behavior on color { ColorAnimation { duration: LingmoUI.Units.veryShortDuration } }
            }

            contentItem: Item {
                Rectangle {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    radius: width / 2
                    color: config.toolColor
                    border.width: 1
                    border.color: LingmoUI.Theme.borderColor
                }
            }

            onClicked: {
                var p = colorButton.mapToItem(editor, 0, colorButton.height + LingmoUI.Units.smallSpacing)
                editor.openColorPopup(p.x, p.y)
            }
        }

        LingmoUI.IconButton {
            size: 32
            iconSize: 18
            visible: editor.tool === "text"
            iconName: "format-text-bold"
            tipText: qsTr("Bold")
            checkable: true
            checked: config.textBold
            onClicked: config.textBold = !config.textBold
        }

        LingmoUI.IconButton {
            size: 32
            iconSize: 18
            iconName: "edit-undo"
            tipText: qsTr("Undo (Ctrl+Z)")
            enabled: editor.annotations.canUndo
            onClicked: editor.annotations.undo()
        }

        LingmoUI.IconButton {
            size: 32
            iconSize: 18
            iconName: "edit-redo"
            tipText: qsTr("Redo (Ctrl+Shift+Z)")
            enabled: editor.annotations.canRedo
            onClicked: editor.annotations.redo()
        }
    }
}
