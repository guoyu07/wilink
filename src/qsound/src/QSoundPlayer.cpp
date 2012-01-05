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

#include <QAbstractNetworkCache>
#include <QAudioOutput>
#include <QIODevice>
#include <QMap>
#include <QPair>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

class QSoundPlayerJobPrivate
{
public:
    QSoundPlayerJobPrivate();

    int id;
    QAudioOutput *audioOutput;
    QNetworkReply *networkReply;
    QSoundPlayer *player;
    QSoundFile *reader;
    bool repeat;
    QUrl url;
};

QSoundPlayerJobPrivate::QSoundPlayerJobPrivate()
    : id(0),
    audioOutput(0),
    networkReply(0),
    player(0),
    reader(0),
    repeat(false)
{
}

QSoundPlayerJob::QSoundPlayerJob(QSoundPlayer *player, int id)
{
    d = new QSoundPlayerJobPrivate;
    d->id = id;
    d->player = player;

    moveToThread(player->thread());
    setParent(player);
}

QSoundPlayerJob::~QSoundPlayerJob() 
{
    if (d->networkReply)
        d->networkReply->deleteLater();
    if (d->audioOutput)
        d->audioOutput->deleteLater();
    if (d->reader)
        d->reader->deleteLater();
    delete d;
}

void QSoundPlayerJob::_q_download()
{
    bool check;
    Q_UNUSED(check);

    qDebug("QSoundPlayer(%i) requesting %s", d->id, qPrintable(d->url.toString()));
    d->networkReply = d->player->networkAccessManager()->get(QNetworkRequest(d->url));
    check = connect(d->networkReply, SIGNAL(finished()),
                    this, SLOT(_q_downloadFinished()));
    Q_ASSERT(check);
}

void QSoundPlayerJob::_q_downloadFinished()
{
    bool check;
    Q_UNUSED(check);
    QNetworkReply *reply = d->networkReply;

    // follow redirect
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        redirectUrl = reply->url().resolved(redirectUrl);

        qDebug("QSoundPlayer(%i) following redirect to %s", d->id, qPrintable(redirectUrl.toString()));
        d->networkReply = d->player->networkAccessManager()->get(QNetworkRequest(redirectUrl));
        check = connect(d->networkReply, SIGNAL(finished()),
                        this, SLOT(_q_downloadFinished()));
        Q_ASSERT(check);
        return;
    }

    // check reply
    if (reply->error() != QNetworkReply::NoError) {
        qWarning("QSoundPlayer(%i) failed to retrieve file: %s", d->id, qPrintable(reply->errorString()));
        emit finished();
        return;
    }

    // read file
    QIODevice *iodevice = d->player->networkAccessManager()->cache()->data(reply->url());
    d->reader = new QSoundFile(iodevice, QSoundFile::Mp3File, this);
    d->reader->setRepeat(d->repeat);

    if (d->reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly))
        _q_start();
}

void QSoundPlayerJob::_q_start()
{
    bool check;
    Q_UNUSED(check);

    if (!d->reader)
        return;

    qDebug("QSoundPlayer(%i) starting audio", d->id);
    d->audioOutput = new QAudioOutput(d->player->outputDevice(), d->reader->format(), this);
    check = connect(d->audioOutput, SIGNAL(stateChanged(QAudio::State)),
                    this, SLOT(_q_stateChanged(QAudio::State)));
    Q_ASSERT(check);

    d->audioOutput->start(d->reader);
}

void QSoundPlayerJob::_q_stateChanged(QAudio::State state)
{
    if (state != QAudio::ActiveState) {
        qDebug("QSoundPlayer(%i) audio stopped", d->id);
        emit finished();
    }
}

class QSoundPlayerPrivate
{
public:
    QSoundPlayerPrivate();
    QString inputName;
    QString outputName;
    QMap<int, QSoundPlayerJob*> jobs;
    QNetworkAccessManager *network;
    int readerId;
};

QSoundPlayerPrivate::QSoundPlayerPrivate()
    : network(0),
    readerId(0)
{
}

QSoundPlayer::QSoundPlayer(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QAudioDeviceInfo>();
    d = new QSoundPlayerPrivate;
}

QSoundPlayer::~QSoundPlayer()
{
    delete d;
}

int QSoundPlayer::play(const QUrl &url, bool repeat)
{
    if (!url.isValid())
        return 0;

    if (url.scheme() == "file") {
        QSoundFile *reader = new QSoundFile(url.toLocalFile());
        reader->setRepeat(repeat);
        return play(reader);
    }
    else if (url.scheme() == "qrc" || url.scheme() == "") {
        const QString path = QLatin1String(":") + (url.path().startsWith("/") ? "" : "/") + url.path();
        QSoundFile *reader = new QSoundFile(path);
        reader->setRepeat(repeat);
        return play(reader);
    }
    else if (d->network) {
        const int id = ++d->readerId;
        QSoundPlayerJob *job = new QSoundPlayerJob(this, id);
        job->d->repeat = repeat;
        job->d->url = url;
        d->jobs[id] = job;
        QMetaObject::invokeMethod(job, "_q_download");
        return id;
    }
    return 0;
}

int QSoundPlayer::play(QSoundFile *reader)
{
    if (!reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        delete reader;
        return 0;
    }

    // register reader
    const int id = ++d->readerId;
    QSoundPlayerJob *job = new QSoundPlayerJob(this, id);
    job->d->reader = reader;
    d->jobs[id] = job;

    // move reader to audio thread
    reader->setParent(0);
    reader->moveToThread(thread());
    reader->setParent(this);

    // schedule play
    QMetaObject::invokeMethod(job, "_q_start");

    return id;
}

QAudioDeviceInfo QSoundPlayer::inputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == d->inputName)
            return info;
    }
    return QAudioDeviceInfo::defaultInputDevice();
}

void QSoundPlayer::setInputDeviceName(const QString &name)
{
    d->inputName = name;
}

QStringList QSoundPlayer::inputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        names << info.deviceName();
    return names;
}

QAudioDeviceInfo QSoundPlayer::outputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.deviceName() == d->outputName)
            return info;
    }
    return QAudioDeviceInfo::defaultOutputDevice();
}

void QSoundPlayer::setOutputDeviceName(const QString &name)
{
    d->outputName = name;
}

QStringList QSoundPlayer::outputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        names << info.deviceName();
    return names;
}

QNetworkAccessManager *QSoundPlayer::networkAccessManager() const
{
    return d->network;
}

void QSoundPlayer::setNetworkAccessManager(QNetworkAccessManager *network)
{
    d->network = network;
}

void QSoundPlayer::stop(int id)
{
    // schedule stop
    QMetaObject::invokeMethod(this, "_q_stop", Q_ARG(int, id));
}

void QSoundPlayer::_q_stop(int id)
{
    QSoundPlayerJob *job = d->jobs.value(id);
    if (!job || !job->d->audioOutput)
        return;

    job->d->audioOutput->stop();
}


