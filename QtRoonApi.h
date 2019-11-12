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
    void		setup			(const QString& url);
    void		open			();
    void		send			(const QString& path, int requestId, const QString* body = nullptr);
    int			send			(const QString& path, IRoonCallback* callback, const QString* body = nullptr);
    void		reply			(const QString& command, int requestId, bool cont = false, QString* body = nullptr);
    void		replyAll		(QMap<int,int> subscriptions, const QString& command, QString* body = nullptr);
    void		addService		(const QString& serviceName, IRoonCallback* service);
    void		sendSubscription	(const QString& path, IRoonCallback* callback);

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

Q_SIGNALS:
    void		closed();
    void		paired();
    void		unpaired();

private Q_SLOTS:
    void		onConnected();
    void		onDisconnected();
    void		onBinaryMessageReceived(const QByteArray& message);
    void		onStateChanged(QAbstractSocket::SocketState state);

private:
    struct RoonState
    {
        QMap<QString, QVariant>	tokens;
        QString			paired_core_id;
    };

    void		setRegistration();
    void		getRegistrationInfo();
    bool		setCallback(int requestId, IRoonCallback* callback);
    bool		parseReveived(const QByteArray& data, ReceivedContent&  content);
    bool		saveState();
    bool		loadState();

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
