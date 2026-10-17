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

// The text recognized in the selection, editable before copying.
LingmoUI.Card {
    id: card

    property alias text: area.text
    signal copyRequested(string text)

    elevation: 3
    width: Math.min(560, parent.width - LingmoUI.Units.largeSpacing * 4)
    height: Math.min(460, parent.height - LingmoUI.Units.largeSpacing * 4)

    onVisibleChanged: if (visible) area.forceActiveFocus()

    // Keeps the pointer from reaching the capture below.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: LingmoUI.Units.largeSpacing * 1.5
        spacing: LingmoUI.Units.largeSpacing

        Label {
            text: qsTr("Recognized text")
            font.pointSize: LingmoUI.Units.fontHeading
            font.bold: true
        }

        Label {
            text: qsTr("Recognized on this computer with tesseract. Check it before using it.")
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                id: area
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                font.pointSize: LingmoUI.Units.fontBody

                Keys.onEscapePressed: card.visible = false
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: LingmoUI.Units.smallSpacing

            LingmoUI.ActionButton {
                text: qsTr("Back")
                onClicked: card.visible = false
            }
            LingmoUI.ActionButton {
                text: qsTr("Copy text")
                iconName: "edit-copy"
                primary: true
                onClicked: card.copyRequested(area.text)
            }
        }
    }
}
