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
import wiLink 2.0

NotificationDialog {
    id: dialog

    property QtObject call
    property Item panel

    iconSource: vcard.avatar
    text: qsTr('%1 wants to talk to you.\n\nDo you accept?').replace('%1', vcard.name)
    title: qsTr('Call from %1').replace('%1', vcard.name)

    VCard {
        id: vcard

        jid: Qt.isQtObject(call) ? call.jid : ''
    }

    onAccepted: {
        panel.showConversation(call.jid);
        dialog.call.accept();
        dialog.close();
    }

    onRejected: {
        dialog.call.hangup();
    }

    Component.onCompleted: {
        // play a sound
        dialog.soundJob = appSoundPlayer.play(":/sounds/call-incoming.ogg", true);
    }

    Connections {
        target: call
        onFinished: dialog.close()
    }
}
