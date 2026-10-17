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

// Keyboard and mouse help, shown when the overlay opens.
LingmoUI.Card {
    id: card

    required property Item editor

    elevation: 3
    width: Math.min(560, editor.width - LingmoUI.Units.largeSpacing * 4)
    implicitHeight: column.implicitHeight + LingmoUI.Units.largeSpacing * 3

    readonly property var shortcuts: [
        [qsTr("Drag"), qsTr("Select an area (click: the highlighted region)")],
        [qsTr("Mouse wheel"), qsTr("Thickness or text size; before selecting: bigger region")],
        [qsTr("Right click"), qsTr("Colour and thickness")],
        [qsTr("Arrows / Shift+Arrows"), qsTr("Move / resize the selection (Ctrl: 10 px)")],
        [qsTr("Enter or Ctrl+C"), qsTr("Copy and close")],
        [qsTr("Ctrl+S / Ctrl+Shift+S"), qsTr("Save / Save as")],
        [qsTr("Ctrl+P"), qsTr("Pin on the screen")],
        [qsTr("Ctrl+T"), qsTr("Copy the text in the selection")],
        [qsTr("Ctrl+Shift+H"), qsTr("Quick redact: hide all text")],
        [qsTr("Ctrl+Z / Ctrl+Shift+Z"), qsTr("Undo / redo")],
        [qsTr("Delete"), qsTr("Delete the selected object")],
        [qsTr("P L A R F O M T N X B I E V"), qsTr("Tools (see their tips)")],
        [qsTr("Esc"), qsTr("Cancel")]
    ]

    // Keeps the pointer from reaching the capture below.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: LingmoUI.Units.largeSpacing * 1.5
        spacing: LingmoUI.Units.largeSpacing

        Label {
            text: qsTr("Take a screenshot")
            font.pointSize: LingmoUI.Units.fontHeading
            font.bold: true
        }

        GridLayout {
            columns: 2
            columnSpacing: LingmoUI.Units.largeSpacing * 1.5
            rowSpacing: LingmoUI.Units.smallSpacing
            Layout.fillWidth: true

            Repeater {
                model: card.shortcuts
                delegate: Label {
                    required property var modelData
                    required property int index
                    text: modelData[0]
                    font.bold: true
                    Layout.row: index
                    Layout.column: 0
                }
            }

            Repeater {
                model: card.shortcuts
                delegate: Label {
                    required property var modelData
                    required property int index
                    text: modelData[1]
                    color: LingmoUI.Theme.secondaryTextColor
                    wrapMode: Text.WordWrap
                    Layout.row: index
                    Layout.column: 1
                    Layout.fillWidth: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: LingmoUI.Units.smallSpacing

            Item { Layout.fillWidth: true }

            LingmoUI.ActionButton {
                text: qsTr("Don't show again")
                onClicked: {
                    config.showHelp = false
                    editor.helpVisible = false
                }
            }
            LingmoUI.ActionButton {
                text: qsTr("Got it")
                primary: true
                onClicked: editor.helpVisible = false
            }
        }
    }
}
