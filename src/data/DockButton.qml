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

Item {
    id: button

    property bool active: dialogSwapper.currentSource == 'PreferenceDialog.qml' ? dialogSwapper.currentSource == panelSource : swapper.currentSource == panelSource
    property bool enabled: true
    property bool notified: false
    property string iconSource: ''
    property string iconPress: ''
    property string panelSource: ''
    property string text
    property int shortcut: 0
    signal clicked

    height: 32
    width: 32
    state: mouseArea.pressed ? 'pressed' : (mouseArea.hovered ? 'hovered' : '')

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 40
        color: '#aa597fbe'
    }

    Image {
        id: image

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: button.enabled ? 1 : 0.5
        smooth: true
        source: (button.iconPress != '' && button.active) ? button.iconPress : button.iconSource
        width: parent.width
        height: parent.height
    }

    Rectangle {
        id: labelBackground

        anchors.bottom: label.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: label.top
        opacity: 0
        color: '#597fbe'
    }

    Text {
        id: label

        function shortcutText(shortcut) {
            var text = '';
            if (shortcut & Qt.ControlModifier)
                text += application.osType == 'mac' ? 'Cmd-' : 'Ctrl-';
            var key = shortcut & 0xffffff;
            if (key >= Qt.Key_A && key <= Qt.Key_Z) {
                var alpha = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
                text += alpha[key - Qt.Key_A];
            }
            if (text.length)
                return '<br/>' + text;
            else
                return '';
        }

        anchors.top: image.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: 0
        color: 'white'
        font.pixelSize: appStyle.font.smallSize
        text: '<b>' + button.text + '</b>' + shortcutText(button.shortcut)
    }

    states: [
        State {
            name: 'hovered'
            PropertyChanges { target: label; opacity: 1 }
            PropertyChanges { target: labelBackground; opacity: 1 }
            PropertyChanges { target: image; height: 32; width: 32 }
            PropertyChanges { target: button; height: 32 + label.paintedHeight; width: 32 + label.paintedWidth }
        },
        State {
            name: 'pressed'
            PropertyChanges { target: label; opacity: 0.5 }
            PropertyChanges { target: labelBackground; opacity: 1 }
            PropertyChanges { target: image; height: 32; width: 32 }
            PropertyChanges { target: button; height: 32 + label.paintedHeight; width: 32 + label.paintedWidth }
        }
    ]

    transitions: Transition {
            from: ''
            to: 'hovered'
            reversible: true
            PropertyAnimation { target: label; properties: 'opacity'; duration: 150 }
            PropertyAnimation { target: image; properties: 'height,width'; duration: 150 }
            PropertyAnimation { target: button; properties: 'height,width'; duration: 150 }
        }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        property bool pressed: false
        property bool hovered: false

        onClicked: {
            if (button.enabled) {
                button.clicked();
            }
        }
        onPressed: {
            if (button.enabled) {
                pressed = true;
            }
        }
        onReleased: pressed = false
        onEntered: {
            if (button.enabled) {
                hovered = true;
            }
        }
        onExited: hovered = false
    }

    SequentialAnimation on x {
        loops: Animation.Infinite
        running: notified && button.state == ''

        PropertyAnimation {
            duration: 100
            easing.type: Easing.OutExpo
            to: 32
        }
        PropertyAnimation {
            duration: 1200
            easing.type: Easing.OutBounce
            to: 0
        }
        PropertyAnimation {
            duration: 1000
            to: 0
        }
    }
}
