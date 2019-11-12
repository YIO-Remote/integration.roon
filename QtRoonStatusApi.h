#pragma once
#include <QtCore/QObject>
#include "QtRoonApi.h"

class QtRoonStatusApi : public QObject, IRoonCallback
{
    Q_OBJECT
public:
    explicit	QtRoonStatusApi(QtRoonApi& roonApi, QObject* parent = nullptr);
    virtual	void OnReceived (const ReceivedContent& content) override;
    void	setStatus	(const QString& message, bool isError);

private:
    class Status
    {
    public:
        QString message;
        bool	is_error;
        void	toVariant(QVariantMap& map) const;
        void	toJson(QString& json) const;
    };
    Status			_status;
    QtRoonApi&			_roonApi;
    QMap<int, int>		_subscriptions;
};
