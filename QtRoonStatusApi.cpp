#include <QJsonDocument>
#include "QtRoonStatusApi.h" 

void	QtRoonStatusApi::Status::toVariant(QVariantMap& map) const
{
    map["message"] = message;
    map["is_error"] = is_error;
}
void QtRoonStatusApi::Status::toJson(QString& json) const
{
    QVariantMap map;
    toVariant(map);
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    json = doc.toJson(QJsonDocument::JsonFormat::Compact);
}

QtRoonStatusApi::QtRoonStatusApi(QtRoonApi& roonApi, QObject* parent) :
    QObject(parent),
    _roonApi(roonApi)
{
    _roonApi.addService(QtRoonApi::ServiceStatus, this);
}
void QtRoonStatusApi::OnReceived(const ReceivedContent& content)
{
    if (_roonApi.Log().isDebugEnabled())
        qCDebug(_roonApi.Log()) << "Status.OnReceived : " << content._messageType << " " << content._requestId << " " << content._service << content._command;

    QJsonDocument document = QJsonDocument::fromJson(content._body.toUtf8());
    QVariantMap map = document.toVariant().toMap();
    int key = map["subscription_key"].toInt();
    if (content._command == "/subscribe_status") {
        _subscriptions[key] = content._requestId;
        QString json;
        _status.toJson(json);
        _roonApi.reply(QtRoonApi::Subscribed, content._requestId, false, &json);
    }
    else if (content._command == "/unsubscribe_status") {
        _subscriptions.remove(key);
        _roonApi.reply(QtRoonApi::Unsubscribed, content._requestId);
    }
    else if (content._command == "/get_status") {
        QString json;
        _status.toJson(json);
        _roonApi.reply(QtRoonApi::Success, content._requestId, false, &json);
    }
}
void QtRoonStatusApi::setStatus(const QString& message, bool isError)
{
    _status.message = message;
    _status.is_error = isError;

    QString json;
    _status.toJson(json);
    _roonApi.replyAll(_subscriptions, QtRoonApi::Changed, &json);
}

