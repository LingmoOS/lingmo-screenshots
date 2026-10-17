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

// The Screenshot window: new capture, recent captures and settings.
LingmoUI.Window {
    id: rootWindow

    width: 940
    height: 660
    minimumWidth: 780
    minimumHeight: 560
    title: qsTr("Screenshot")
    visible: true

    background.opacity: LingmoUI.Theme.darkMode ? 0.88 : 0.97
    header.height: LingmoUI.Units.windowHeaderHeight
    contentTopMargin: 0

    readonly property var pages: ["capture", "history", "settings"]

    function showPage(page) {
        var index = pages.indexOf(page)
        if (index >= 0)
            sideBar.currentIndex = index
    }

    Component.onCompleted: showPage(initialPage)

    Connections {
        target: app
        function onLauncherPageRequested(page) { rootWindow.showPage(page) }
    }

    LingmoUI.WindowBlur {
        view: rootWindow
        geometry: Qt.rect(rootWindow.x, rootWindow.y, rootWindow.width, rootWindow.height)
        windowRadius: rootWindow.windowRadius
        enabled: true
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        LingmoUI.SettingsSideBar {
            id: sideBar
            Layout.fillHeight: true
            model: ListModel {
                ListElement { title: qsTr("New capture"); iconSource: "qrc:/images/sidebar/capture.svg"; iconColor: "#0A84FF"; section: "" }
                ListElement { title: qsTr("History"); iconSource: "qrc:/images/sidebar/history.svg"; iconColor: "#FF9F0A"; section: "" }
                ListElement { title: qsTr("Settings"); iconSource: "qrc:/images/sidebar/settings.svg"; iconColor: "#8E8E93"; section: "" }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: sideBar.currentIndex

            CapturePage {}
            HistoryPage {}
            SettingsPage {}
        }
    }
}
