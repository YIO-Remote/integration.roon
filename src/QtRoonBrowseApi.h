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

class QtRoonBrowseApi : public QObject, public IRoonCallback
{
    Q_OBJECT
public:
    struct BrowseInput {
        BrowseInput();
        BrowseInput(const BrowseInput& other);
        BrowseInput(const QVariantMap& map);
        QString			prompt;
        QString			action;
        QString			value;
        bool			is_password;
    };
    struct BrowseItem {
        BrowseItem();
        ~BrowseItem();
        BrowseItem(const BrowseItem& other);
        BrowseItem(const QVariantMap& map);
        BrowseItem& operator=(const BrowseItem& other);
        QString			title;
        QString			subtitle;
        QString			item_key;
        QString			image_key;
        QString			hint;
        BrowseInput*            input_prompt;
    };
    struct BrowseOption {
        BrowseOption() {
            pop_levels = set_display_offset = -1; pop_all = refresh_list = false;
            hierarchy = "browse";
        }
        BrowseOption(const QString &id, bool popAll, bool refreshList = false, int setDisplayOffset = -1) {
            hierarchy = "browse"; zone_or_output_id = id; set_display_offset = setDisplayOffset;
            pop_levels = -1; pop_all = popAll; refresh_list = refreshList;
        }
        BrowseOption(const QString &id, int popLevels, int setDisplayOffset = -1) {
            hierarchy = "browse"; zone_or_output_id = id;
            pop_levels = popLevels; set_display_offset = setDisplayOffset;
            pop_all = refresh_list = false;
        }
        BrowseOption(const QString &id, const QString& itemKey, int setDisplayOffset = -1) {
            hierarchy = "browse"; zone_or_output_id = id; item_key = itemKey; set_display_offset = setDisplayOffset;
            pop_levels = -1; pop_all = refresh_list = false;
        }
        QString			hierarchy;
        QString			zone_or_output_id;
        QString			multi_session_key;
        QString			item_key;
        QString			input;
        int			pop_levels;
        bool			pop_all;
        bool			refresh_list;
        int			set_display_offset;
        void			toVariant(QVariantMap& map) const;
        void			toJson(QString& json) const;
    };
    struct BrowseList {
        BrowseList();
        BrowseList(const QVariantMap& map);
        int			count;
        int			level;
        int			display_offset;
        QString			title;
        QString			subtitle;
        QString			image_key;
        QString			hint;
    };
    struct BrowseResult {
        BrowseResult();
        BrowseResult(const QVariantMap& map);
        ~BrowseResult();
        QString			action;
        BrowseList*		list;
        BrowseItem*		item;
        bool			is_error;
        QString			message;
    };
    struct LoadOption {
        LoadOption() { level = offset = count = set_display_offset = -1; hierarchy = "browse"; }
        QString			hierarchy;
        int			level;
        int			offset;
        int			count;
        int			set_display_offset;
        QString			multi_session_key;
        void			toVariant(QVariantMap& map) const;
        void			toJson(QString& json) const;
    };
    struct LoadResult {
        LoadResult();
        LoadResult(const QVariantMap& map);
        ~LoadResult();
        QList<BrowseItem>	items;
        int			offset;
        BrowseList*		list;
    };

    struct Context {
        int     requestId;
        QString zoneId;
        bool    isLoad;
    };

    struct ICallback {
        virtual ~ICallback();
        virtual void OnBrowse (const QString& err, Context& context, const BrowseResult& content) = 0;
        virtual void OnLoad   (const QString& err, Context& context, const LoadResult& content) = 0;
    };

    explicit	QtRoonBrowseApi(QtRoonApi& roonApi, ICallback* callBack = nullptr, QObject* parent = nullptr);
    virtual	void OnReceived(const ReceivedContent& content) override;

    int browse	(const BrowseOption& option, void (*func) (int requestId, const QString& err, const BrowseResult& result));
    int load	(const LoadOption& option, void (*func) (int requestId, const QString& err, const LoadResult& result));

    int browse	(const BrowseOption& option, Context& context);
    int load	(const LoadOption& option, Context& context);

private:
    static const int        NUMCONTEXTS = 10;
    QtRoonApi&              _roonApi;
    ICallback*              _callback;
    Context*                _context[NUMCONTEXTS];
    void                    (*_browseCallback) (int requestId, const QString& err, const BrowseResult& result);
    void                    (*_loadCallback)   (int requestId, const QString& err, const LoadResult& result);
};
