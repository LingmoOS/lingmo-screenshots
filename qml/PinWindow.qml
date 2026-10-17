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
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// A pinned capture: drag to move, wheel to zoom, Ctrl+wheel for opacity,
// double click or Esc to close, right click for more.
Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: mouse.containsMouse ? LingmoUI.Theme.highlightColor : LingmoUI.Theme.borderColor
        Behavior on border.color { ColorAnimation { duration: LingmoUI.Units.veryShortDuration } }
    }

    Image {
        anchors.fill: parent
        anchors.margins: 1
        source: pin.imageUrl
        smooth: pin.zoom !== 1
        mipmap: pin.zoom < 1
        opacity: pin.imageOpacity
    }

    // Zoom level, briefly.
    Rectangle {
        visible: zoomTimer.running
        anchors.centerIn: parent
        width: zoomLabel.implicitWidth + LingmoUI.Units.largeSpacing
        height: zoomLabel.implicitHeight + LingmoUI.Units.smallSpacing
        radius: LingmoUI.Theme.smallRadius
        color: LingmoUI.Theme.secondBackgroundColor
        border.width: 1
        border.color: LingmoUI.Theme.borderColor

        Label {
            id: zoomLabel
            anchors.centerIn: parent
            text: "%1%".arg(Math.round((zoomTimer.showOpacity ? pin.imageOpacity : pin.zoom) * 100))
        }
    }

    Timer {
        id: zoomTimer
        property bool showOpacity: false
        interval: 800
    }

    MouseArea {
        id: mouse

        property point pressPos

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

        onPressed: (event) => {
            pressPos = Qt.point(event.x, event.y)
            if (event.button === Qt.RightButton)
                menu.popup()
        }
        onPositionChanged: (event) => {
            if (pressed && (event.buttons & Qt.LeftButton))
                pin.moveBy(event.x - pressPos.x, event.y - pressPos.y)
        }
        onDoubleClicked: pin.closePin()
        onWheel: (wheel) => {
            var up = wheel.angleDelta.y > 0
            if (wheel.modifiers & Qt.ControlModifier) {
                pin.imageOpacity = pin.imageOpacity + (up ? 0.1 : -0.1)
                zoomTimer.showOpacity = true
            } else {
                pin.zoom = pin.zoom * (up ? 1.1 : 1 / 1.1)
                zoomTimer.showOpacity = false
            }
            zoomTimer.restart()
        }
    }

    Menu {
        id: menu
        popupType: Popup.Window

        MenuItem { text: qsTr("Copy"); icon.name: "edit-copy"; onTriggered: pin.copy() }
        MenuItem { text: qsTr("Save as…"); icon.name: "document-save-as"; onTriggered: pin.saveAs() }
        MenuItem { text: qsTr("Edit"); icon.name: "draw-freehand"; onTriggered: pin.edit() }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Original size")
            icon.name: "zoom-original"
            onTriggered: {
                pin.zoom = 1
                pin.imageOpacity = 1
            }
        }
        MenuSeparator {}
        MenuItem { text: qsTr("Close"); icon.name: "window-close"; onTriggered: pin.closePin() }
    }
}
