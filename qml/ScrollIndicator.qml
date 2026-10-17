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

// Shown during a scrolling capture, outside the captured area.
Rectangle {
    id: control

    width: layout.implicitWidth + LingmoUI.Units.largeSpacing * 2
    height: 44
    radius: LingmoUI.Theme.hugeRadius
    color: LingmoUI.Theme.backgroundColor
    border.width: 1
    border.color: LingmoUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.15)

    RowLayout {
        id: layout
        anchors.centerIn: parent
        spacing: LingmoUI.Units.largeSpacing

        LingmoUI.Spinner {
            running: scroller.running
            size: LingmoUI.Units.iconSizes.small + 2
        }

        Label {
            text: qsTr("Scrolling capture: %1 px").arg(scroller.capturedHeight)
            font.pointSize: LingmoUI.Units.fontBody
        }

        LingmoUI.ActionButton {
            text: qsTr("Cancel")
            onClicked: scroller.cancel()
        }

        LingmoUI.ActionButton {
            text: qsTr("Done")
            primary: true
            onClicked: scroller.stop()
        }
    }
}
