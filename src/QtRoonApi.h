/******************************************************************************
 *
 * Copyright (C) 2019 Christian Riedl <ric@rts.co.at>
 *
 * This file is part of the YIO-Remote software project.
 *
 * YIO-Remote software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YIO-Remote software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YIO-Remote software. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QLoggingCategory>

class RoonCore
{
public:
    QString             core_id;
    QString             display_name;
    QString             display_version;

    void                fromVariant(const QVariantMap& map);
};
struct IRoonPaired {
    virtual             ~IRoonPaired();
    virtual void        OnPaired(const RoonCore& core) = 0;
    virtual void        OnUnpaired(const RoonCore& core) = 0;
};
class RoonRegister
{
public:
    RoonRegister() { paired = nullptr; }
	
    QString             extension_id;
    QString             display_name;
    QString             display_version;
    QString             publisher;
    QString             email;
    QStringList         required_services;
    QStringList         optional_services;
    QStringList         provided_services;
    QString             token;
    QString             website;
    IRoonPaired*        paired;
    void                toVariant(QVariantMap &map) const;
    void                toJson(QString& json) const;
};
struct ReceivedContent
{
public:
    int                 _contentLength;
    int			_requestId;
    QString		_contentType;
    QString		_messageType;
    QString		_service;
    QString		_command;
    QString		_body;
};

struct IRoonCallback {
    virtual ~IRoonCallback();
    virtual void OnReceived(const ReceivedContent& content) = 0;
};

class QtRoonApi : public QObject, IRoonCallback
{
	Q_OBJECT
public:
    explicit		QtRoonApi		(const QString& url, const QString& directory, RoonRegister& reg, QLoggingCategory& log, QObject* parent = nullptr);
    void		setup			(const QString& url, const QVariantMap& config);
    void		open			();
    void		close			();
    void		send			(const QString& path, int requestId, const QString* body = nullptr);
    int			send			(const QString& path, IRoonCallback* callback, const QString* body = nullptr);
    void		reply			(const QString& command, int requestId, bool cont = false, QString* body = nullptr);
    void		replyAll		(QMap<int,int> subscriptions, const QString& command, QString* body = nullptr);
    void		addService		(const QString& serviceName, IRoonCallback* service);
    int                 sendSubscription	(const QString& path, IRoonCallback* callback, int subscriptionKey = -1);

    virtual void	OnReceived		(const ReceivedContent& content) override;
    QLoggingCategory&   Log                     () { return _log; }


    static QString      ServiceRegistry;
    static QString      ServiceTransport;
    static QString      ServiceStatus;
    static QString      ServicePairing;
    static QString      ServicePing;
    static QString      ServiceImage;
    static QString      ServiceBrowse;
    static QString      ServiceSettings;
    static QString      ControlVolume;
    static QString      ControlSource;
    static QString      MessageRequest;
    static QString      MessageComplete;
    static QString      MessageContinue;
    static QString      Success;
    static QString      Registered;
    static QString      Subscribed;
    static QString      Unsubscribed;
    static QString      Changed;

signals:
    void		closed                  ();
    void		error                   (QString err);
    void		paired                  ();
    void		unpaired                ();
    void        stateChanged            (const QVariantMap& stateMap);

public slots:
    void		onConnected             ();
    void		onDisconnected          ();
    void		onBinaryMessageReceived (const QByteArray& message);
    void		onStateChanged          (QAbstractSocket::SocketState state);
    void                onError                 (QAbstractSocket::SocketError error);

private:
    struct RoonState
    {
        QMap<QString, QVariant>	tokens;
        QString			paired_core_id;
    };

    void                onPairing               (const ReceivedContent& content);
    void		setRegistration         ();
    void		getRegistrationInfo     ();
    bool		setCallback             (int requestId, IRoonCallback* callback);
    bool		parseReveived           (const QByteArray& data, ReceivedContent&  content);
    const QVariantMap   getState    ();

    RoonRegister&                   _register;
    RoonCore                        _roonCore;
    QUrl                            _url;
    QWebSocket                      _webSocket;
    QString                         _directory;
    int                             _requestId;
    QMap<int, IRoonCallback*>       _requests;
    QMap<QString, IRoonCallback*>   _services;
    QMap<int, int>                  _subscriptions;
    bool                            _paired;
    RoonState                       _roonState;
    QLoggingCategory&               _log;
};
