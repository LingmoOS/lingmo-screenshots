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

// Loupe next to the pointer: the pixels around it, enlarged, with the
// position and colour of the pixel under it.
Item {
    id: control

    required property Item editor

    readonly property int zoom: 8
    readonly property int cells: 15
    readonly property int lens: cells * zoom
    readonly property int px: Math.floor(editor.pointerX * editor.s)
    readonly property int py: Math.floor(editor.pointerY * editor.s)
    readonly property color pixel: capture.pixelColor(px, py)
    readonly property real offset: 24

    width: lens + 2
    height: lens + 2 + info.height + LingmoUI.Units.smallSpacing

    // Beside the pointer, flipped near the edges.
    x: editor.pointerX + offset + width > parent.width ? editor.pointerX - offset - width : editor.pointerX + offset
    y: editor.pointerY + offset + height > parent.height ? editor.pointerY - offset - height : editor.pointerY + offset

    function hex(c) {
        function two(v) { var h = Math.round(v * 255).toString(16).toUpperCase(); return h.length < 2 ? "0" + h : h }
        return "#" + two(c.r) + two(c.g) + two(c.b)
    }

    LingmoUI.Elevation {
        anchors.fill: lensFrame
        level: 2
        radius: lensFrame.radius
    }

    Rectangle {
        id: lensFrame
        width: control.lens + 2
        height: control.lens + 2
        radius: LingmoUI.Theme.smallRadius
        color: LingmoUI.Theme.backgroundColor
        border.width: 1
        border.color: LingmoUI.Theme.borderColor
        clip: true

        Image {
            source: capture.imageUrl
            cache: true
            smooth: false
            mipmap: false
            width: capture.imageWidth * control.zoom
            height: capture.imageHeight * control.zoom
            x: 1 + control.lens / 2 - (control.px + 0.5) * control.zoom
            y: 1 + control.lens / 2 - (control.py + 0.5) * control.zoom
        }

        // The pixel under the pointer.
        Rectangle {
            anchors.centerIn: parent
            width: control.zoom + 2
            height: control.zoom + 2
            color: "transparent"
            border.width: 1
            border.color: LingmoUI.Theme.highlightColor
        }
    }

    Rectangle {
        id: info
        anchors.top: lensFrame.bottom
        anchors.topMargin: LingmoUI.Units.smallSpacing
        anchors.horizontalCenter: lensFrame.horizontalCenter
        width: infoRow.implicitWidth + LingmoUI.Units.largeSpacing
        height: infoRow.implicitHeight + LingmoUI.Units.smallSpacing
        radius: LingmoUI.Theme.tinyRadius
        color: LingmoUI.Theme.secondBackgroundColor
        border.width: 1
        border.color: LingmoUI.Theme.borderColor

        RowLayout {
            id: infoRow
            anchors.centerIn: parent
            spacing: LingmoUI.Units.smallSpacing

            Rectangle {
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                radius: 2
                color: control.pixel
                border.width: 1
                border.color: LingmoUI.Theme.borderColor
            }

            Label {
                text: "%1, %2 · %3".arg(control.px).arg(control.py).arg(control.hex(control.pixel))
                font.pointSize: LingmoUI.Units.fontCaption
                font.family: LingmoUI.Theme.fontFamily
            }
        }
    }
}
