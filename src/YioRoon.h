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

#define NEW_VERSION 1

#include <QtCore/QObject>

#include "QtRoonApi.h"
#include "QtRoonBrowseApi.h"
#include "QtRoonDiscovery.h"
#include "QtRoonTransportApi.h"
#include "yio-interface/entities/entityinterface.h"
#include "yio-interface/notificationsinterface.h"
#include "yio-interface/plugininterface.h"
#include "yio-model/mediaplayer/albummodel_mediaplayer.h"
#include "yio-model/mediaplayer/searchmodel_mediaplayer.h"
#include "yio-plugin/integration.h"
#include "yio-plugin/plugin.h"

const bool NO_WORKER_THREAD = false;

class RoonPlugin : public Plugin {
    Q_OBJECT
    // Q_DISABLE_COPY(Roon)
    Q_INTERFACES(PluginInterface)
    Q_PLUGIN_METADATA(IID "YIO.PluginInterface" FILE "roon.json")

 public:
    RoonPlugin();

    // Plugin interface
 protected:
    Integration* createIntegration(const QVariantMap& config, EntitiesInterface* entities,
                                   NotificationsInterface* notifications, YioAPIInterface* api,
                                   ConfigInterface* configObj) override;

 public slots:  // NOLINT open issue: https://github.com/cpplint/cpplint/pull/99
    void onRoonDiscovered(QMap<QString, QVariantMap>);

 private:
    QtRoonDiscovery _discovery;
};

class YioRoon : public Integration, IRoonPaired, QtRoonBrowseApi::ICallback {
    Q_OBJECT

 public:
    explicit YioRoon(const QVariantMap& config, EntitiesInterface* entities, NotificationsInterface* notifications,
                     YioAPIInterface* api, ConfigInterface* configObj, Plugin* plugin);
    ~YioRoon() override;

    void sendCommand(const QString& type, const QString& entityId, int command, const QVariant& param) override;

 public slots:  // NOLINT open issue: https://github.com/cpplint/cpplint/pull/99
    void connect() override;
    void disconnect() override;
    void enterStandby() override;
    void leaveStandby() override;

    void onZonesChanged();
    void onZoneSeekChanged(const QtRoonTransportApi::Zone& zone);
    void onError(const QString& error);

 private:
    enum EAction {
        ACT_NONE       = 0,
        ACT_PLAYNOW    = 0x0001,
        ACT_PLAYFROM   = 0x0002,
        ACT_SHUFFLE    = 0x0004,
        ACT_ADDNEXT    = 0x0008,
        ACT_QUEUE      = 0x0010,
        ACT_STARTRADIO = 0x0020,
        ACT_REJECT     = 0x0100,
        ACT_ACTIONLIST = 0x0200
    };

    class Action {
     public:
        Action(const QString& roonName, const QString& yioName, const QString& type, const QString& parentTitle,
               EAction action, EAction forcedActions, EAction forcedChildActions)
            : roonName(roonName),
              yioName(yioName),
              type(type),
              parentTitle(parentTitle),
              action(action),
              forcedActions(forcedActions),
              forcedChildActions(forcedChildActions) {}
        QString roonName;
        QString yioName;
        QString type;
        QString parentTitle;
        EAction action;
        EAction forcedActions;
        EAction forcedChildActions;
    };
    enum BrowseMode { BROWSE, PLAY, ACTION, GOTOPATH, GETALBUM };

    struct YioContext : QtRoonBrowseApi::Context {
        YioContext() : goBack(0) {}
        YioContext(int index, const QString& entityId, const QString& friendlyName)
            : index(index),
              itemIndex(-1),
              queueFrom(false),
              gotoNext(false),
              entityId(entityId),
              friendlyName(friendlyName),
              searchModel(nullptr),
              goBack(0) {}
        int                    index;
        int                    itemIndex;
        bool                   queueFrom;
        bool                   gotoNext;
        BrowseMode             browseMode;
        QString                entityId;
        QString                friendlyName;
        QString                forcedAction;
        QStringList            path;
        QStringList            goToPath;
        QString                searchText;
        SearchModel*           searchModel;
        QStringList            searchKeys;
        QMap<QString, QString> albumMap;
        int                    goBack;
    };

    RoonRegister                        _reg;
    QtRoonApi                           _roonApi;
    QtRoonBrowseApi                     _browseApi;
    QtRoonTransportApi                  _transportApi;
    int                                 _subscriptionKey;
    int                                 _numItems;
    bool                                _playFromThere;
    QString                             _url;
    QString                             _imageUrl;
    QList<Action>                       _actions;
    QList<QtRoonBrowseApi::BrowseItem>* _items;
    QList<YioContext>                   _contexts;
    QStringList                         _notFound;
    bool                                _cmdsForItem;
    BrowseModel                         _model;
    QString                             _configPath;
    static YioRoon*                     _instance;

    static void transportCallback(int requestId, const QString& msg);
    void        browse(YioContext& ctx, bool fromTop);
    void        browse(YioContext& ctx, const QList<QtRoonBrowseApi::BrowseItem>& items, int itemIndex, bool action);
    void        browseAction(YioContext& ctx, const QString& item_key);
    void        browseBack(YioContext& ctx);
    void        browseRefresh(YioContext& ctx);
    void        playMedia(YioContext& ctx, const QString& itemKey, bool setItemIndex = false);
    void        search(YioContext& ctx, const QString& searchText, const QString& itemKey);
    void        search(YioContext& ctx, const QString& searchText);
    void        getAlbum(YioContext& ctx, const QString& itemKey);

    void OnPaired(const RoonCore& core) override;
    void OnUnpaired(const RoonCore& core) override;
    void OnBrowse(const QString& err, QtRoonBrowseApi::Context& context,
                  const QtRoonBrowseApi::BrowseResult& content) override;
    void OnLoad(const QString& err, QtRoonBrowseApi::Context& context,
                const QtRoonBrowseApi::LoadResult& content) override;

    void updateItems(YioContext& ctx, const QtRoonBrowseApi::LoadResult& result);
    bool updateSearch(YioContext& ctx, const QtRoonBrowseApi::LoadResult& result);
    bool updateSearchList(YioContext& ctx, const SearchModelItem* item, const QtRoonBrowseApi::LoadResult& result);
    void updateAlbum(YioContext& ctx, const QtRoonBrowseApi::LoadResult& result);
    void updateZone(YioContext& ctx, const QtRoonTransportApi::Zone& zone, bool seekChanged);
    void updateError(YioContext& ctx, const QString& error);
    QStringList   getForcedActions(EAction forcedActions);
    const Action* getActionRoon(const QString& roonName);
    const Action* getActionYio(const QString& yioName);
    const Action* getActionParentTitle(const QString& parentTitle);
    int           findItemIndex(const QList<QtRoonBrowseApi::BrowseItem>& items, const QString& itemKey);

    void storeState(const QVariantMap& stateMap);
};
