/*
 * wDesktop
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

#ifndef __WDESKTOP_CHAT_SHARES_H__
#define __WDESKTOP_CHAT_SHARES_H__

#include <QDir>
#include <QIcon>
#include <QTreeWidget>

#include "qxmpp/QXmppShareIq.h"

#include "chat_panel.h"

class ChatClient;
class ChatSharesDatabase;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class QXmppPacket;

class ChatSharesView : public QTreeWidget
{
    Q_OBJECT

public:
    ChatSharesView(QWidget *parent = 0);
    qint64 addItem(const QXmppShareIq::Item &item, QTreeWidgetItem *parent);
    void clear();
    QTreeWidgetItem *findItem(const QXmppShareIq::Item &item, QTreeWidgetItem *parent);

protected:
    void resizeEvent(QResizeEvent *e);

private:
    QIcon collectionIcon;
    QIcon fileIcon;
    QIcon peerIcon;
};

class ChatShares : public ChatPanel
{
    Q_OBJECT

public:
    ChatShares(ChatClient *client, QWidget *parent = 0);
    void setShareServer(const QString &server);

signals:
    void fileExpected(const QString &sid);

private slots:
    void goBack();
    void findRemoteFiles();
    void itemDoubleClicked(QTreeWidgetItem *item);
    void registerWithServer();
    void shareGetIqReceived(const QXmppShareGetIq &getIq);
    void shareSearchIqReceived(const QXmppShareSearchIq &share);
    void searchFinished(const QXmppShareSearchIq &share);

private:
    QString shareServer;
    QDir sharesDir;

    ChatClient *client;
    ChatSharesDatabase *db;
    QLineEdit *lineEdit;
    ChatSharesView *treeWidget;
    QTimer *registerTimer;
};

#endif
