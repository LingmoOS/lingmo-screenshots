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

// A capture in a window: reopened from the history, a long capture...
LingmoUI.Window {
    id: rootWindow

    title: editorTitle
    visible: true
    minimumWidth: 820
    minimumHeight: 520
    width: Math.max(minimumWidth, Math.min(Screen.desktopAvailableWidth * 0.9,
                                           capture.imageWidth / Screen.devicePixelRatio + LingmoUI.Units.largeSpacing * 6))
    height: Math.max(minimumHeight, Math.min(Screen.desktopAvailableHeight * 0.9,
                                             capture.imageHeight / Screen.devicePixelRatio + 180))

    background.opacity: LingmoUI.Theme.darkMode ? 0.88 : 0.97
    header.height: LingmoUI.Units.windowHeaderHeight

    LingmoUI.WindowBlur {
        view: rootWindow
        geometry: Qt.rect(rootWindow.x, rootWindow.y, rootWindow.width, rootWindow.height)
        windowRadius: rootWindow.windowRadius
        enabled: true
    }

    headerItem: Item {
        Label {
            anchors.centerIn: parent
            text: rootWindow.title
            font.pointSize: LingmoUI.Units.fontBody
            font.bold: true
            elide: Text.ElideRight
            width: Math.min(implicitWidth, parent.width - LingmoUI.Units.windowControlsInset * 2)
            horizontalAlignment: Text.AlignHCenter
        }
    }

    CaptureEditor {
        id: editor
        anchors.fill: parent
    }
}
