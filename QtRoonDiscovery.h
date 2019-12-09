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
