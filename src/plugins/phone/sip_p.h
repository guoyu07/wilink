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

#ifndef __WILINK_PHONE_SIP_P_H__
#define __WILINK_PHONE_SIP_P_H__

#include <QHostAddress>
#include <QMap>
#include <QObject>

#include "sip.h"

class QAudioInput;
class QAudioOutput;
class QUdpSocket;
class QTimer;
class QXmppRtpChannel;

/** The SipCallContext class represents a SIP dialog.
 */
class SipCallContext
{
public:
    SipCallContext();

    quint32 cseq;
    QByteArray id;
    QByteArray tag;

    QMap<QByteArray, QByteArray> challenge;
    QMap<QByteArray, QByteArray> proxyChallenge;
    SipMessage lastRequest;
    QList<SipTransaction*> transactions;
};

class SipCallPrivate : public SipCallContext
{
public:
    SipCallPrivate(SipCall *qq);
    SdpMessage buildSdp(const QList<QXmppJinglePayloadType> &payloadTypes) const;
    void handleReply(const SipMessage &reply);
    void handleRequest(const SipMessage &request);
    bool handleSdp(const SdpMessage &sdp);
    void onStateChanged();
    void sendInvite();
    void setState(QXmppCall::State state);

    QXmppCall::Direction direction;
    QTime startTime;
    QTime stopTime;
    QXmppCall::State state;
    QByteArray activeTime;
    QXmppRtpChannel *audioChannel;
    QAudioInput *audioInput;
    QAudioOutput *audioOutput;
    QXmppIceConnection *iceConnection;
    bool invitePending;
    bool inviteQueued;
    SipMessage inviteRequest;
    QByteArray remoteRecipient;
    QList<QByteArray> remoteRoute;
    QByteArray remoteUri;

    SipClient *client;
    QTimer *timer;

private:
    SipCall *q;
};

class SipClientPrivate : public SipCallContext
{
public:
    SipClientPrivate(SipClient *qq);
    QByteArray authorization(const SipMessage &request, const QMap<QByteArray, QByteArray> &challenge) const;
    SipMessage buildRequest(const QByteArray &method, const QByteArray &uri, SipCallContext *ctx, int seq);
    SipMessage buildResponse(const SipMessage &request);
    bool handleAuthentication(const SipMessage &reply, SipCallContext *ctx);
    void handleReply(const SipMessage &reply);
    void sendRequest(SipMessage &request, SipCallContext *ctx);
    void setState(SipClient::State state);
    SipTransaction *startTransaction(const SipMessage &request, QObject *receiver);

    // timers
    QTimer *connectTimer;
    QTimer *registerTimer;
    QTimer *stunTimer;

    // configuration
    QString displayName;
    QString username;
    QString password;
    QString domain;

    SipClient::State state;
    QHostAddress serverAddress;
    quint16 serverPort;
    QList<SipCall*> calls;

    // sockets
    QHostAddress localAddress;
    QHostAddress contactAddress;
    quint16 contactPort;
    QUdpSocket *socket;

    // STUN
    quint32 stunCookie;
    bool stunDone;
    QByteArray stunId;
    QHostAddress stunReflexiveAddress;
    quint16 stunReflexivePort;
    QHostAddress stunServerAddress;
    quint16 stunServerPort;

private:
    SipClient *q;
};

#endif
