#include "QtRoonDiscovery.h"

QtRoonDiscovery::QtRoonDiscovery(QLoggingCategory& log, QObject *parent) :
    QObject(parent),
    _log(log),
    _timer(parent),
    _onlyFirst(false)
{

    connect(&_timer, &QTimer::timeout, this, &QtRoonDiscovery::onTimeout);
}

void QtRoonDiscovery::startDiscovery (int waitMilliseconds, bool onlyOne)
{
    QByteArray  buffer;
    int         index;

    _timer.setSingleShot(true);
    _timer.setInterval(waitMilliseconds);
    _timer.start();
    _onlyFirst = onlyOne;
    _tid = QUuid::createUuid().toString();

    buffer.append("SOOD", 4);
    index = buffer.length();
    buffer[index++] = 2;
    buffer[index++] = QString("Q").toUtf8()[0];

    index = addToBuffer(buffer, index, "query_service_id", SOOD_SID);
    index = addToBuffer(buffer, index, "_tid", _tid);

    QHostAddress ipAddress;
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
             ipAddress = address;
    }

    _udpSocket.bind(ipAddress, 0);
    if (_udpSocket.state() != QUdpSocket::BoundState) {
        qCWarning(_log) << "Discovery UDP not bound : " << _udpSocket.state();
        return;
    }
    connect(&_udpSocket, &QUdpSocket::readyRead, this, &QtRoonDiscovery::onPendingDatagrams);

    _udpSocket.writeDatagram (buffer, QHostAddress(SOOD_MULTICAST_IP), SOOD_PORT);

}
void QtRoonDiscovery::onPendingDatagrams()
{
    // Now read all available datagrams from the socket
    while (_udpSocket.hasPendingDatagrams()) {
        QByteArray      buffer;
        QHostAddress    address;
        quint16         port;

        buffer.resize(static_cast<int>(_udpSocket.pendingDatagramSize()));
        _udpSocket.readDatagram(buffer.data(), buffer.size(), &address, &port);

        if (buffer.size() < 10)
            continue;

        int index = 0;
        QString name  = QLatin1String (buffer.data() + index, 4);
        index += 4;
        int mark = buffer[index++];
        QString value = QLatin1String (buffer.data() + index, 1);
        index += 1;

        if (name == "SOOD" && mark == 2 && value == "R")
        {
            QString ipAddress = address.toString();
            if (_queryResults.contains(ipAddress))
                continue;
            QVariantMap map;
            while (index < buffer.length())
            {
                index = decodeFromBuffer(buffer, index, name, value);
                map[name] = value;
            }
            qCDebug(_log) << "Discovery found : " << ipAddress;
            _queryResults.insert(ipAddress, map);
            if (_onlyFirst)
                onTimeout();
        }
    }
}

void QtRoonDiscovery::onTimeout() {
    _timer.stop();
    _udpSocket.close();
    emit roonDiscovered (_queryResults);
}


int QtRoonDiscovery::addToBuffer (QByteArray& buffer, int index, const QString& name, const QString &value)
{
    QByteArray arr = name.toUtf8();
    buffer[index++] = static_cast<char>(arr.length());
    for (int i = 0; i < arr.length(); i++)
        buffer[index++] = arr[i];
    arr = value.toUtf8();
    buffer[index++] = static_cast<char>(arr.length() >> 8);
    buffer[index++] = static_cast<char>(arr.length() & 0xff);
    for (int i = 0; i < arr.length(); i++)
        buffer[index++] = arr[i];
    return index;
}
int QtRoonDiscovery::decodeFromBuffer  (const QByteArray& buffer, int index, QString& name, QString& value)
{
    if (index >= buffer.length())
        return buffer.length();
    int length = buffer[index++];
    name = QLatin1String (buffer.data() + index, length);
    index += length;
    if (index >= buffer.length())
        return buffer.length();
    length = (buffer[index++] << 8);
    length += buffer[index++];
    value = QLatin1String (buffer.data() + index, length);
    index += length;
    return index;

}

