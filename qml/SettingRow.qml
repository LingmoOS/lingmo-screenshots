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

// One line of a settings card: title, optional explanation, control on the right.
RowLayout {
    id: row

    property string title
    property string subtitle
    default property alias controls: trailing.data

    Layout.fillWidth: true
    Layout.minimumHeight: LingmoUI.Units.controlHeight
    spacing: LingmoUI.Units.largeSpacing

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            text: row.title
            font.pointSize: LingmoUI.Units.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            visible: row.subtitle.length > 0
            text: row.subtitle
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    RowLayout {
        id: trailing
        spacing: LingmoUI.Units.smallSpacing
    }
}
