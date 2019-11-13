#ifndef QTROONDISCOVERY_H
#define QTROONDISCOVERY_H

#include <QObject>
#include <QtNetwork>

class QtRoonDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit        QtRoonDiscovery     (QLoggingCategory& _log, QObject *parent = nullptr);
    void            startDiscovery      (int waitMilliseconds, bool onlyOne);

signals:
    void roonDiscovered (QMap<QString, QVariantMap>);

public slots:
    void onPendingDatagrams             ();
    void onTimeout                      ();

private:
    const quint16   SOOD_PORT           = 9003;
    const QString   SOOD_MULTICAST_IP   = "239.255.90.90";
    const QString   SOOD_SID            = "00720724-5143-4a9b-abac-0e50cba674bb";
    static int      addToBuffer         (QByteArray& buffer, int index, const QString& name, const QString &value);
    static int      decodeFromBuffer    (const QByteArray& buffer, int index, QString& name, QString& value);

    QLoggingCategory&           _log;
    QUdpSocket                  _udpSocket;
    QMap<QString, QVariantMap>  _queryResults;
    QTimer                      _timer;
    bool                        _onlyFirst;
    QString                     _tid;
};

#endif // QTROONDISCOVERY_H
