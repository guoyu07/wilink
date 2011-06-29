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

#ifndef __WILINK_WINDOW_H__
#define __WILINK_WINDOW_H__

#include <QMainWindow>

class QFileDialog;
class QUrl;

/** Chat represents the user interface's main window.
 */
class Window : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(bool isActiveWindow READ isActiveWindow NOTIFY isActiveWindowChanged)

public:
    Window(const QUrl &url, const QString &jid, QWidget *parent = 0);

signals:
    void isActiveWindowChanged();
    void showAbout();
    void showHelp();
    void showPreferences();

public slots:
    void alert();
    QFileDialog *fileDialog();

protected:
    void changeEvent(QEvent *event);
};

#endif
