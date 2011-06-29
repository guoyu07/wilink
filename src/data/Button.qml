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

Rectangle {
    id: button

    property bool enabled: true
    property int iconSize: iconSource != '' ? appStyle.icon.smallSize : 0
    property alias iconSource: image.source
    property string text: ''

    signal clicked

    height: button.visible ? (Math.max(iconSize, labelHelper.height) + 8) : 0
    width: button.visible ? labelHelper.width + iconSize + ((iconSource != '' && text != '') ? 3 : 2) * image.anchors.margins : 0
    border.color: '#84bde8'
    gradient: Gradient {
        GradientStop { id: stop1; position: 0.0; color: '#ffffff' }
        GradientStop { id: stop2; position: 1.0; color: '#beceeb' }
    }
    radius: 4
    smooth: true

    Image {
        id: image

        height: iconSize
        width: iconSize
        anchors.left: parent.left
        anchors.margins: 4
        anchors.verticalCenter: parent.verticalCenter
        smooth: true
    }

    Text {
        id: label

        anchors.left: image.right
        anchors.leftMargin: iconSource != '' ? 4 : 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        text: button.text
    }

    Text {
        id: labelHelper

        opacity: 0
        text: button.text
    }

    Rectangle {
        id: overlay

        anchors.fill: button
        color: '#99ffffff'
        visible: !button.enabled
        radius: button.radius
        smooth: true
    }

    MouseArea {
        anchors.fill: parent
        enabled: button.enabled

        onClicked: button.clicked()
        onPressed: button.state = 'pressed'
        onReleased: button.state = ''
    }

    states: State {
        name: 'pressed'
        PropertyChanges { target: stop1; color: '#aacbd9f0' }
        PropertyChanges { target: stop2; color: '#aaffffff' }
    }
}

