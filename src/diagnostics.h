/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#ifndef __WDESKTOP_DIAGNOSTICS_H__
#define __WDESKTOP_DIAGNOSTICS_H__

#include <QDialog>

class QPushButton;
class QTextBrowser;

class NetworkThread;
class WirelessThread;

class Diagnostics : public QDialog
{
    Q_OBJECT

public:
    Diagnostics(QWidget *parent=0);

protected slots:
    void addItem(const QString &title, const QString &value);
    void addSection(const QString &title);
    void print();
    void refresh();
    void networkFinished();
    void wirelessFinished();

private:
    QPushButton *printButton;
    QPushButton *refreshButton;
    QTextBrowser *text;

    NetworkThread *networkThread;
    WirelessThread *wirelessThread;
};

#endif
