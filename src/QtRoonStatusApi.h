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
