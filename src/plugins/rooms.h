/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#ifndef __WILINK_ROOMS_H__
#define __WILINK_ROOMS_H__

#include <QObject>

class Chat;
class QMenu;
class QModelIndex;
class QXmppMessage;
class QXmppMucOwnerIq;

class ChatRoomWatcher : public QObject
{
    Q_OBJECT

public:
    ChatRoomWatcher(Chat *chatWindow);

private slots:
    void inviteContact();
    void messageReceived(const QXmppMessage &msg);
    void mucOwnerIqReceived(const QXmppMucOwnerIq &iq);
    void mucServerFound(const QString &roomServer);
    void roomOptions();
    void roomMembers();
    void rosterMenu(QMenu *menu, const QModelIndex &index);

private:
    Chat *chat;
    QString chatRoomServer;
};

#endif
