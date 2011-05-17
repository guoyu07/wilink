/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 1.0

Column {
    id: blocks

    signal itemClicked(string id)

    anchors.fill: parent

    RosterView {
        id: rooms

        model: roomModel
        title: qsTr('My rooms')
        height: parent.height / 3
        width: parent.width

        Connections {
            onItemClicked: { blocks.itemClicked(model.id); }
        }
    }

    RosterView {
        id: contacts

        model: contactModel
        title: qsTr('My contacts')
        height: 2 * parent.height / 3
        width: parent.width

        Menu {
            id: menu
            opacity: 0
        }

        Connections {
            onItemClicked: { blocks.itemClicked(model.id); }
            onItemContextMenu: {
                menu.model.clear()
                if (model.url != undefined && model.url != '')
                    menu.model.append({'title': qsTr('Show profile'), 'url':model.url})
                menu.x = 16;
                menu.y = point.y - 16;
                menu.state = 'visible';
            }
        }

        Connections {
            target: menu
            onItemClicked: Qt.openUrlExternally(menu.model.get(index).url)
        }
    }

}
