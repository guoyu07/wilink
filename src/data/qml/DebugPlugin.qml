/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

import QtQuick 1.1

Plugin {
    name: qsTr('Debugging')
    description: qsTr('This plugin allows you to view debugging information.')
    imageSource: 'image://icon/debug'

    onLoaded: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'wifirst') {
                dock.model.add({
                    'iconSource': 'image://icon/dock-peer',
                    'iconPress': 'image://icon/peer',
                    'panelProperties': {accountJid: account.jid},
                    'panelSource': 'DiscoveryPanel.qml',
                    'priority': -1,
                    'shortcut': Qt.ControlModifier + Qt.Key_B,
                    'text': qsTr('Discovery'),
                    'visible': true});
                }
            }

        dock.model.add({
            'iconSource': 'image://icon/dock-debug',
            'iconPress': 'image://icon/debug',
            'panelSource': 'LogPanel.qml',
            'priority': -1,
            'shortcut': Qt.ControlModifier + Qt.Key_L,
            'text': qsTr('Debugging'),
            'visible': true});
    }

    onUnloaded: {
        dock.model.removePanel('DiscoveryPanel.qml');
        dock.model.removePanel('LogPanel.qml');
    }
}
