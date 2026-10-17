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

// Colour, recent colours, a colour from the capture, thickness and text.
// Drawn inside the overlay: a separate dialog window would open under it.
Popup {
    id: popup

    required property Item editor

    parent: editor
    padding: LingmoUI.Units.largeSpacing
    width: 280
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    function openAt(px, py) {
        x = Math.max(LingmoUI.Units.smallSpacing, Math.min(px, editor.width - width - LingmoUI.Units.smallSpacing))
        y = Math.max(LingmoUI.Units.smallSpacing, Math.min(py, editor.height - implicitHeight - LingmoUI.Units.smallSpacing))
        hexField.text = config.toolColor.toString().toUpperCase()
        open()
    }

    background: Item {
        LingmoUI.Elevation {
            anchors.fill: parent
            level: 2
            radius: bg.radius
        }
        Rectangle {
            id: bg
            anchors.fill: parent
            radius: LingmoUI.Theme.mediumRadius
            color: LingmoUI.Theme.secondBackgroundColor
            border.width: 1
            border.color: LingmoUI.Theme.borderColor
        }
    }

    component Swatch: AbstractButton {
        id: swatch
        property color swatchColor
        implicitWidth: 26
        implicitHeight: 26
        hoverEnabled: true
        focusPolicy: Qt.NoFocus
        background: Rectangle {
            radius: width / 2
            color: swatch.swatchColor
            border.width: Qt.colorEqual(config.toolColor, swatch.swatchColor) ? 3 : 1
            border.color: Qt.colorEqual(config.toolColor, swatch.swatchColor) ? LingmoUI.Theme.highlightColor
                                                                             : LingmoUI.Theme.borderColor
            scale: swatch.hovered ? 1.12 : 1.0
            Behavior on scale { NumberAnimation { duration: LingmoUI.Units.veryShortDuration } }
        }
        onClicked: editor.setColor(swatchColor)
    }

    contentItem: ColumnLayout {
        spacing: LingmoUI.Units.smallSpacing

        GridLayout {
            columns: 7
            rowSpacing: LingmoUI.Units.smallSpacing
            columnSpacing: LingmoUI.Units.smallSpacing
            Layout.alignment: Qt.AlignHCenter

            Repeater {
                model: config.palette
                delegate: Swatch {
                    required property string modelData
                    swatchColor: modelData
                }
            }
        }

        Label {
            visible: config.recentColors.length > 0
            text: qsTr("Recent")
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            Layout.topMargin: LingmoUI.Units.smallSpacing
        }

        RowLayout {
            visible: config.recentColors.length > 0
            spacing: LingmoUI.Units.smallSpacing

            Repeater {
                model: config.recentColors
                delegate: Swatch {
                    required property string modelData
                    swatchColor: modelData
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: LingmoUI.Units.smallSpacing
            spacing: LingmoUI.Units.smallSpacing

            TextField {
                id: hexField
                Layout.fillWidth: true
                placeholderText: "#RRGGBB"
                selectByMouse: true
                onAccepted: {
                    var c = Qt.color(text)
                    if (c.valid)
                        editor.setColor(c)
                }
            }

            LingmoUI.IconButton {
                iconName: "color-picker"
                tipText: qsTr("Pick a colour from the capture")
                checkable: true
                checked: editor.eyedropper
                onClicked: {
                    editor.eyedropper = true
                    popup.close()
                }
            }
        }

        Label {
            text: editor.tool === "text" ? qsTr("Text size: %1").arg(config.textSize)
                                         : qsTr("Thickness: %1").arg(config.toolWidth)
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            Layout.topMargin: LingmoUI.Units.smallSpacing
        }

        Slider {
            Layout.fillWidth: true
            from: editor.tool === "text" ? 8 : 1
            to: editor.tool === "text" ? 120 : 40
            stepSize: 1
            value: editor.tool === "text" ? config.textSize : config.toolWidth
            onMoved: {
                if (editor.tool === "text")
                    config.textSize = value
                else
                    config.toolWidth = value
            }
        }

        RowLayout {
            visible: editor.tool === "text"
            Layout.fillWidth: true
            Label {
                text: qsTr("Bold")
                Layout.fillWidth: true
            }
            Switch {
                checked: config.textBold
                onToggled: config.textBold = checked
            }
        }
    }
}
