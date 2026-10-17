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
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// Border of the selection with eight handles to resize it.
Item {
    id: frame

    required property Item editor

    x: editor.selX
    y: editor.selY
    width: editor.selW
    height: editor.selH

    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        color: "transparent"
        border.width: 1
        border.color: LingmoUI.Theme.highlightColor
    }

    Repeater {
        // h/v: -1 = left/top edge, 0 = middle, 1 = right/bottom edge
        model: [
            { h: -1, v: -1, cursor: Qt.SizeFDiagCursor }, { h: 0, v: -1, cursor: Qt.SizeVerCursor },
            { h: 1, v: -1, cursor: Qt.SizeBDiagCursor }, { h: 1, v: 0, cursor: Qt.SizeHorCursor },
            { h: 1, v: 1, cursor: Qt.SizeFDiagCursor }, { h: 0, v: 1, cursor: Qt.SizeVerCursor },
            { h: -1, v: 1, cursor: Qt.SizeBDiagCursor }, { h: -1, v: 0, cursor: Qt.SizeHorCursor }
        ]

        delegate: Item {
            id: handle

            required property var modelData

            // Hidden on tiny selections, except the corners.
            visible: modelData.h !== 0 && modelData.v !== 0 || (modelData.h === 0 ? frame.width : frame.height) > 40
            width: 22
            height: 22
            x: (modelData.h + 1) / 2 * frame.width - width / 2
            y: (modelData.v + 1) / 2 * frame.height - height / 2

            Rectangle {
                anchors.centerIn: parent
                width: 9
                height: 9
                radius: width / 2
                color: LingmoUI.Theme.secondBackgroundColor
                border.width: 2
                border.color: LingmoUI.Theme.highlightColor
                scale: handleArea.containsMouse || handleArea.pressed ? 1.3 : 1.0

                Behavior on scale { NumberAnimation { duration: LingmoUI.Units.veryShortDuration } }
            }

            MouseArea {
                id: handleArea

                property real startX
                property real startY
                property rect startSel

                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
                cursorShape: handle.modelData.cursor

                onPressed: (mouse) => {
                    var p = mapToItem(frame.parent, mouse.x, mouse.y)
                    startX = p.x
                    startY = p.y
                    startSel = Qt.rect(editor.selX, editor.selY, editor.selW, editor.selH)
                    editor.mode = "resize"
                    editor.forceActiveFocus()
                }

                onPositionChanged: (mouse) => {
                    if (!pressed)
                        return
                    var p = mapToItem(frame.parent, mouse.x, mouse.y)
                    editor.pointerX = p.x
                    editor.pointerY = p.y
                    var dx = p.x - startX, dy = p.y - startY
                    var x1 = startSel.x, y1 = startSel.y
                    var x2 = startSel.x + startSel.width, y2 = startSel.y + startSel.height
                    if (handle.modelData.h < 0) x1 += dx
                    if (handle.modelData.h > 0) x2 += dx
                    if (handle.modelData.v < 0) y1 += dy
                    if (handle.modelData.v > 0) y2 += dy
                    editor.setSelection(x1, y1, x2 - x1, y2 - y1)
                }

                onReleased: editor.mode = ""
            }
        }
    }
}
