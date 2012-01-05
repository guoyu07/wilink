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

#ifndef __WILINK_SOUND_PLAYER_H__
#define __WILINK_SOUND_PLAYER_H__

#include <QAudioDeviceInfo>
#include <QStringList>

class QNetworkAccessManager;
class QSoundFile;
class QSoundPlayer;
class QSoundPlayerPrivate;
class QSoundPlayerJobPrivate;

class QSoundPlayerJob : public QObject
{
    Q_OBJECT

public:
    int id() const;

signals:
    void finished();

private slots:
    void _q_download();
    void _q_downloadFinished();
    void _q_start();
    void _q_stateChanged(QAudio::State state);

private:
    QSoundPlayerJob(QSoundPlayer *player, int id);
    ~QSoundPlayerJob();

    QSoundPlayerJobPrivate *d;
    friend class QSoundPlayer;
};

class QSoundPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList inputDeviceNames READ inputDeviceNames CONSTANT)
    Q_PROPERTY(QStringList outputDeviceNames READ outputDeviceNames CONSTANT)

public:
    QSoundPlayer(QObject *parent = 0);
    ~QSoundPlayer();

    QAudioDeviceInfo inputDevice() const;
    QStringList inputDeviceNames() const;

    QAudioDeviceInfo outputDevice() const;
    QStringList outputDeviceNames() const;

    QNetworkAccessManager *networkAccessManager() const;
    void setNetworkAccessManager(QNetworkAccessManager *manager);

    int play(QSoundFile *reader);

signals:
    void finished(int id);

public slots:
    int play(const QUrl &url, bool repeat = false);
    void setInputDeviceName(const QString &name);
    void setOutputDeviceName(const QString &name);
    void stop(int id);

private slots:
    void _q_stop(int id);

private:
    QSoundPlayerPrivate *d;
};

#endif
