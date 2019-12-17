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

#include "QtRoonApi.h"
#include <QJsonDocument>
#include <QFile>

IRoonPaired::~IRoonPaired()
{}
IRoonCallback::~IRoonCallback()
{}

void RoonRegister::toVariant(QVariantMap & map) const {
    map["extension_id"] = extension_id;
    map["display_name"] = display_name;
    map["display_version"] = display_version;
    map["publisher"] = publisher;
    map["email"] = email;
    map["required_services"] = required_services;
    map["optional_services"] = optional_services;
    map["provided_services"] = provided_services;
    map["token"] = token;
    map["website"] = website;
}
void RoonRegister::toJson(QString& json) const
{
    QVariantMap map;
    toVariant(map);
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    json = doc.toJson(QJsonDocument::JsonFormat::Compact);
}

void RoonCore::fromVariant(const QVariantMap& map) {
    for (auto e : map.keys())
    {
        QVariant var = map.value(e);
        if (e == "core_id")
            core_id = var.toString();
        else if (e == "display_name")
            display_name = var.toString();
        else if (e == "display_version")
            display_version = var.toString();
    }
}

QT_USE_NAMESPACE

QString QtRoonApi::ServiceRegistry  = "com.roonlabs.registry:1";
QString QtRoonApi::ServiceTransport = "com.roonlabs.transport:2";		// support for zones_seek_changed
QString QtRoonApi::ServiceStatus    = "com.roonlabs.status:1";
QString QtRoonApi::ServicePairing   = "com.roonlabs.pairing:1";
QString QtRoonApi::ServicePing      = "com.roonlabs.ping:1";
QString QtRoonApi::ServiceImage     = "com.roonlabs.image:1";
QString QtRoonApi::ServiceBrowse    = "com.roonlabs.browse:1";
QString QtRoonApi::ServiceSettings  = "com.roonlabs.settings:1";
QString QtRoonApi::ControlVolume    = "com.roonlabs.volumecontrol:1";
QString QtRoonApi::ControlSource    = "com.roonlabs.sourcecontrol:1";

QString QtRoonApi::MessageRequest   = "REQUEST";
QString QtRoonApi::MessageComplete  = "COMPLETE";
QString QtRoonApi::MessageContinue  = "CONTINUE";
QString QtRoonApi::Success          = "Success";
QString QtRoonApi::Registered       = "Registered";
QString QtRoonApi::Subscribed       = "Subscribed";
QString QtRoonApi::Unsubscribed     = "Unsubscribed";
QString QtRoonApi::Changed          = "Changed";

QtRoonApi::QtRoonApi(const QString& url, const QString& directory, RoonRegister& reg, QLoggingCategory& log, QObject* parent) :
    QObject(parent),
    _register(reg),
    _url(url),
    _directory(directory),
    _requestId(0),
    _paired(false),
    _log(log)
{
    connect(&_webSocket, &QWebSocket::connected, this, &QtRoonApi::onConnected);
    connect(&_webSocket, &QWebSocket::disconnected, this, &QtRoonApi::onDisconnected);
    connect(&_webSocket, &QWebSocket::stateChanged, this, &QtRoonApi::onStateChanged);
    connect(&_webSocket, &QWebSocket::binaryMessageReceived, this, &QtRoonApi::onBinaryMessageReceived);
    //_webSocket.ignoreSslErrors();
    //connect(&_webSocket, &QWebSocket::error, this, &QtRoonApi::onError);  not working
    QObject::connect(&_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    //loadState();
}
void QtRoonApi::setup(const QString& url, const QString& directory)
{
    _url = url;
    _directory = directory;

    loadState();
}
void QtRoonApi::open() {
    _webSocket.open(_url);
}
void QtRoonApi::close() {
    _webSocket.close();
}
void QtRoonApi::addService(const QString& serviceName, IRoonCallback* service)
{
    _services.insert(serviceName, service);
    _register.provided_services.append(serviceName);
}
int QtRoonApi::sendSubscription(const QString& path, IRoonCallback* callback, int subscriptionKey)
{
    QVariantMap map;
    if (subscriptionKey < 0)
        subscriptionKey = _requests.count();
    map["subscription_key"] = subscriptionKey;
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    QString json = doc.toJson(QJsonDocument::JsonFormat::Compact);

    send(path, callback, &json);
    return subscriptionKey;
}
int QtRoonApi::send(const QString& path, IRoonCallback* callback, const QString* body) {
    int requestId = _requestId;
    setCallback(_requestId, callback);
    send(path, _requestId++, body);
    return requestId;
}

void QtRoonApi::send(const QString& command, int requestId, const QString* body) {
    QString data;
    QString contentType = "application/json";
    if (body == nullptr)
        data = QString("MOO/1 REQUEST %1\nRequest-Id: %2\n\n").arg (command, QString::number(requestId));
    else
        data = QString("MOO/1 REQUEST %1\nRequest-Id: %2\nContent-Length: %3\nContent-Type: %4\n\n%5").arg (command, QString::number(requestId), QString::number(body->length()), contentType, QString(*body));
    _webSocket.sendBinaryMessage(data.toUtf8());
    if (_log.isDebugEnabled())
        qCDebug(_log) << "send : " << command << " " << requestId << " " << (body != nullptr ? *body : "");
}
void QtRoonApi::reply(const QString& command, int requestId, bool cont, QString* body) {
    QString data;
    QString contentType = "application/json";
    QString mode = cont ? "CONTINUE" : "COMPLETE";
    if (body == nullptr)
        data = QString("MOO/1 %1 %2\nRequest-Id: %3\n\n").arg(mode, command, QString::number(requestId));
    else
        data = QString("MOO/1 %1 %2\nRequest-Id: %3\nContent-Length: %4\nContent-Type: %5\n\n%6").arg(mode, command, QString::number(requestId), QString::number(body->length()), contentType, QString(*body));
    _webSocket.sendBinaryMessage(data.toUtf8());
    if (_log.isDebugEnabled())
        qCDebug(_log) << "reply : " << command << " " << requestId << " " << (body != nullptr ? *body : "");
}
void QtRoonApi::replyAll(QMap<int, int> subscriptions, const QString& command, QString* body) {
    for (auto e : subscriptions.keys())
    {
        int requestId = subscriptions.value(e);
        reply(command, requestId, true, body);
    }
}

void QtRoonApi::onConnected() {
    qCDebug(_log) << "OnConnected";
    getRegistrationInfo();
}
void QtRoonApi::onDisconnected() {
    qCDebug(_log) << "OnDisconnected";
    // reconnect @@@
    emit closed();
}
void QtRoonApi::onBinaryMessageReceived(const QByteArray& message) {
    ReceivedContent content;
    if (parseReveived(message, content)) {
        IRoonCallback* cb;
        if (content._messageType == MessageRequest) {
            if (content._service == ServicePing) {
                this->reply(Success, content._requestId, false, nullptr);
            }
            else if (content._service == ServicePairing) {
                onPairing (content);
            }
            else {
                cb = _services.value(content._service, nullptr);
                if (cb != nullptr)
                    cb->OnReceived(content);
            }
        }
        else if (content._messageType == MessageComplete) {
            cb = _requests.value(content._requestId, nullptr);
            if (cb != nullptr)
                cb->OnReceived(content);
            _requests.remove(content._requestId);
        }
        else if (content._messageType == MessageContinue) {
            cb = _requests.value(content._requestId, nullptr);
            if (cb != nullptr)
                cb->OnReceived(content);
        }
    }
}
void QtRoonApi::onPairing (const ReceivedContent& content)
{
    QJsonDocument document = QJsonDocument::fromJson(content._body.toUtf8());
    QVariantMap map = document.toVariant().toMap();
    int key = map["subscription_key"].toInt();
    if (content._command == "/subscribe_pairing") {
        QVariantMap replymap;
        _subscriptions[key] = content._requestId;
        _paired = true;
        _roonState.paired_core_id = _roonCore.core_id;
        replymap["paired_core_id"] = _roonCore.core_id;
        QJsonDocument doc = QJsonDocument::fromVariant(replymap);
        QString json(doc.toJson(QJsonDocument::JsonFormat::Compact));
        reply(Subscribed, content._requestId, false, &json);
        if (_register.paired != nullptr)
            _register.paired->OnPaired(_roonCore);
        emit paired();
        saveState();
    }
    else if (content._command == "/unsubscribe_pairing") {
        _subscriptions.remove(key);
        _paired = false;
        _roonState.paired_core_id.clear();
        reply(Unsubscribed, content._requestId);
        if (_register.paired != nullptr)
            _register.paired->OnUnpaired(_roonCore);
        emit unpaired();
        saveState();
    }
}

void QtRoonApi::onStateChanged(QAbstractSocket::SocketState state) {
    qCDebug(_log) << "OnStateChanged : " << state;
}
void QtRoonApi::onError(QAbstractSocket::SocketError err) {
    qCDebug(_log) << "OnError : " << err;
    emit error ("Socket error " + QString::number(err));
}

void QtRoonApi::setRegistration()
{
    if (!_roonState.paired_core_id.isEmpty()) {
        _register.token = _roonState.tokens[_roonState.paired_core_id].toString();
    }
    QString json;
    _register.toJson(json);
    setCallback (_requestId, this);
    send(ServiceRegistry + "/register", _requestId++, &json);
}
void QtRoonApi::getRegistrationInfo()
{
    setCallback(_requestId, this);
    send(ServiceRegistry + "/info", _requestId++);
}
void QtRoonApi::OnReceived(const ReceivedContent& content)
{
    if (_log.isDebugEnabled())
        qCDebug(_log) << "OnReceived : " << content._messageType << " " << content._requestId << " " << content._service << content._command;

    QJsonDocument document = QJsonDocument::fromJson(content._body.toUtf8());
    QVariantMap map = document.toVariant().toMap();

    if (content._command == Registered) {
        // Register
        if (map.contains("token"))
            _roonState.tokens.insert(_roonCore.core_id, map["token"]);
        else
            _roonState.tokens.remove(_roonCore.core_id);
        QVariant providedServices = map["provided_services"];
        QVariant httpPort = map["http_port"];
        saveState();
    }
    else {
        // Info
        QJsonDocument document = QJsonDocument::fromJson(content._body.toUtf8());
        QVariantMap map = document.toVariant().toMap();
        _roonCore.fromVariant(map);
        setRegistration();
    }
}
bool QtRoonApi::setCallback(int requestId, IRoonCallback* callback)
{
    IRoonCallback* cb = _requests.value(requestId);
    if (cb != nullptr) {
        qCWarning(_log) << "setCallBack " << requestId << "in use";
        return false;
    }
    _requests.insert(requestId, callback);
    return true;
}

bool QtRoonApi::parseReveived(const QByteArray& data, ReceivedContent& content)
{
    content._contentLength = content._requestId = 0;
    QString		dataString (data);
    QStringList lines = dataString.split('\n', QString::KeepEmptyParts);
    if (lines.length() <= 2) {
        qWarning() << "too short : " << dataString;
        return false;
    }
    if (_log.isDebugEnabled())
        qCDebug(_log) << "receive : " << lines[0];
    if (!lines[0].startsWith("MOO"))
        return false;
    QStringList line = lines[0].split(' ');
    if (line.length() < 2)
        return false;
    content._messageType = line[1];
    if (line.length() >= 3) {
        content._command = line[2];
        if (content._messageType == MessageRequest) {
            int idx = content._command.indexOf('/');
            content._service = content._command.left(idx);
            content._command = content._command.mid(idx);
        }
    }
    int length = lines.length();
    for (int i = 1; i < length; i++) {
        line = lines[i].split(':', QString::KeepEmptyParts);
        if (line.length() >= 2) {
            QString value = line[1].trimmed();
            if (line[0] == "Request-Id")
                content._requestId = value.toInt();
            else if (line[0] == "Content-Length")
                content._contentLength = value.toInt();
            else if (line[0] == "Content-Type")
                content._contentType = value;
            }
        else {
            // End
            if (content._contentLength > 0)
                content._body = lines[length - 1];
            else
                content._body.clear();
            break;
        }
    }
    return true;
}
bool QtRoonApi::saveState() {
    QVariantMap map;
    map["tokens"] = QVariant(_roonState.tokens);
    map["paired_core_id"] = _roonState.paired_core_id;
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    QString json(doc.toJson(QJsonDocument::JsonFormat::Compact));
    QFile file(_directory + "/roonState.json");

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(_log) << "saveState can't create " << file.fileName();
        return false;
    }
    file.write(doc.toJson(QJsonDocument::JsonFormat::Compact));
    file.close();
    return true;
}
bool QtRoonApi::loadState() {
    QFile	file(_directory + "/roonState.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(_log) << "loadState can't open " << file.fileName();
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(data);
    QVariantMap map = document.toVariant().toMap();
    _roonState.tokens = map["tokens"].toMap();
    _roonState.paired_core_id = map["paired_core_id"].toString();
    return true;
}





