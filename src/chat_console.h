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

#ifndef __WILINK_CHAT_CONSOLE_H__
#define __WILINK_CHAT_CONSOLE_H__

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

#include "qxmpp/QXmppLogger.h"

#include "chat_panel.h"

class QCheckBox;
class QTextDocument;
class QTextBrowser;

class ChatConsole : public ChatPanel
{
    Q_OBJECT

public:
    ChatConsole(QXmppLogger *logger, QWidget *parent = 0);

private slots:
    void slotShow();
    void slotClose();
    void message(QXmppLogger::MessageType type, const QString &msg);

private:
    QTextBrowser *browser;
    QXmppLogger *currentLogger;
    QCheckBox *showDebug;
    QCheckBox *showPackets;
};

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat tagFormat;
    QTextCharFormat quotationFormat;
};

#endif
