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

class QtRoonTransportApi : public QObject, public IRoonCallback, public IRoonPaired
{
    Q_OBJECT
    public:
    enum class EState
    {
        unknown = 0,
        stopped, playing, paused, loading
    };
    Q_ENUM(EState)
    enum class EControl
    {
        stop = 0,
        play, pause, playpause, previous, next
    };
    Q_ENUM(EControl)
    enum class ELoop
    {
        unknown = 0,
        disabled, loop, loop_one, next
    };
    Q_ENUM(ELoop)
    enum class ESourceControlStatus {
        indeterminate = 0,
        selected, deselected, standby
    };
    Q_ENUM(ESourceControlStatus)
    enum class EValueMode {
        absolute = 0,
        relative, relative_step
    };
    Q_ENUM(EValueMode)
    enum class EMute {
        mute = 0,
        unmute
    };
    Q_ENUM(EMute)

    struct SourceControl {
        SourceControl(const SourceControl& other);
        SourceControl(const QVariantMap& map);
        QString			control_key;
        QString			display_name;
        ESourceControlStatus	status;
        bool			supports_standby;
    };
    struct Volume {
        Volume(const QVariantMap& map);
        Volume(const Volume& other);
        QString                 type;
        int			min;
        int			max;
        int			value;
        int			step;
        bool			is_muted;
    };
    struct Output {
        Output();
        Output(const Output& other);
        Output(const QVariantMap& map);
        Output& operator=(const Output& other);
        ~Output();
        QString			output_id;
        QString			zone_id;
        QString			display_name;
        QStringList		can_group_with_output_ids;
        EState			state;
        QList<SourceControl>	source_controls;
        Volume*                 volume;
    };
    struct ZoneSettings {
        ZoneSettings(const QVariantMap& map);
        ZoneSettings(const ZoneSettings& src);
        bool    		auto_radio;
        ELoop			loop;
        bool			shuffle;
    };
    struct Lines {
        Lines(const QVariantMap& map);
        Lines(const Lines& other);
        QString			line1;
        QString			line2;
        QString			line3;
    };
    struct NowPlaying {
        NowPlaying();
        NowPlaying(const NowPlaying& other);
        NowPlaying(const QVariantMap& map);
        NowPlaying& operator=(const NowPlaying& other);
        ~NowPlaying();
        int			seek_position;
        int			length;
        QString			image_key;
        Lines*			one_line;
        Lines*			two_line;
        Lines*			three_line;
    };
    struct Zone {
        Zone();
        Zone(const Zone& other);
        Zone(const QVariantMap& map);
        Zone& operator=(const Zone& other);
        ~Zone();
        QString			zone_id;
        EState			state;
        QString			display_name;
        bool			is_next_allowed;
        bool			is_pause_allowed;
        bool			is_play_allowed;
        bool			is_previous_allowed;
        bool			is_seek_allowed;
        int			seek_position;
        QList<Output>		outputs;
        int			queue_items_remaining;
        int			queue_time_remaining;
        ZoneSettings*		settings;
        NowPlaying*		now_playing;
    };
    struct ZoneChanged {
        ZoneChanged(const QVariantMap& map);
        ZoneChanged(const ZoneChanged& other);
        QString			zone_id;
        int			seek_position;
        int			queue_time_remaining;
    };
    struct TransportMessage {
        TransportMessage();
        TransportMessage(const QVariantMap& map);
        ~TransportMessage();
        QList<Zone>*            zones;
        QList<Zone>*            zones_removed;
        QList<Zone>*            zones_added;
        QList<Zone>*            zones_changed;
        QList<ZoneChanged>*     zones_seek_changed;
    };

    typedef void (*TCallback)   (int requestId, const QString& msg);

    explicit QtRoonTransportApi(QtRoonApi& roonApi, TCallback callback = nullptr, QObject* parent = nullptr);
        virtual	void            OnReceived(const ReceivedContent& content) override;
        int			subscribeZones	();
        void			unsubscribeZones(int subscriptionKey);
        QList<Zone>*		getZones	();
        QMap<QString, Zone>&    zones		() { return _zones; }
        const Zone&		getZone		(const QString & zone_id);
        int			control		(const QString& zone_or_output_id, EControl control, TCallback callback = nullptr);
        int			changeSettings	(const QString& zone_or_output_id, bool shuffle, bool autoRadio, ELoop loop, TCallback callback = nullptr);
        int			changeVolume	(const QString& output_id, EValueMode how, int value, TCallback callback = nullptr);
        int			mute		(const QString& output_id, EMute how, TCallback callback = nullptr);
        int			muteAll		(EMute how, TCallback callback = nullptr);
        int			pauseAll	(TCallback callback = nullptr);
        int			seek		(const QString& zone_or_output_id, EValueMode how, int second, TCallback callback = nullptr);

        static QString		toString        (EControl control);
        static QString		toString        (ELoop loop);
        static QString		toString        (EValueMode mode);
        static QString		toString        (EMute mute);
        static EState		toEState        (const QString& str);
        static ELoop		toELoop         (const QString& str);
	static ESourceControlStatus	toESourceControlStatus(const QString& str);

    Q_SIGNALS:
        void                    zonesChanged();
        void                    zoneSeekChanged (const Zone& zone);

    private:
        QtRoonApi&		_roonApi;
        QMap<int, int>		_subscriptions;
        QMap<QString, Zone>	_zones;
        TCallback		_permanentCallback;
        TCallback		_callback;

        int			send            (const QString& path, const QVariantMap& map, TCallback callback);

        virtual void            OnPaired        (const RoonCore& core) override;
        virtual void            OnUnpaired      (const RoonCore& core) override;

        static QMetaEnum        getMetaEnum     (const char* enumName);
};
