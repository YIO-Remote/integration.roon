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

#include "YioRoon.h"

#include <QStandardPaths>

#include "yio-interface/configinterface.h"
#include "yio-interface/entities/mediaplayerinterface.h"

RoonPlugin::RoonPlugin() : Plugin("roon", NO_WORKER_THREAD), _discovery(m_logCategory) {}

Integration* RoonPlugin::createIntegration(const QVariantMap& config, EntitiesInterface* entities, NotificationsInterface* notifications,
                        YioAPIInterface* api, ConfigInterface* configObj) {
    qCInfo(m_logCategory) << "Creating Roon integration plugin" << PLUGIN_VERSION;

    return new YioRoon(config, entities, notifications, api, configObj, this);
}

void RoonPlugin::onRoonDiscovered(QMap<QString, QVariantMap> soodmaps) {
    QVariantMap roonsmap;

    int idx = 1;
    for (QMap<QString, QVariantMap>::const_iterator iter = soodmaps.begin(); iter != soodmaps.end(); ++iter) {
        QVariantMap soodmap = iter->value(iter.key()).toMap();
        QVariantMap roonmap;
        roonmap["ip"]            = iter.key();
        roonmap["type"]          = "roon";
        roonmap["friendly_name"] = soodmap["name"];
        roonmap["id"]            = "roon" + (idx > 1 ? QString::number(idx) : "");
        roonsmap["data"]         = roonmap;
        idx++;
    }
    // @@@RIC
    // FIXME emit discovered instance with: emit createDone(returnData);
}

YioRoon* YioRoon::_instance = nullptr;

YioRoon::YioRoon(const QVariantMap& config, EntitiesInterface* entities, NotificationsInterface* notifications,
                 YioAPIInterface* api, ConfigInterface* configObj, Plugin* plugin)
    : Integration(config, entities, notifications, api, configObj, plugin),
      _roonApi("", "", _reg, m_logCategory),
      _browseApi(_roonApi, this),
      _transportApi(_roonApi, transportCallback),
      _subscriptionKey(-1),
      _numItems(500),
      _items(nullptr),
      _cmdsForItem(true) {
    EAction shuffleAction  = static_cast<EAction>(ACT_SHUFFLE | ACT_STARTRADIO);
    EAction playAction     = static_cast<EAction>(ACT_PLAYNOW | ACT_ADDNEXT | ACT_QUEUE | ACT_STARTRADIO);
    EAction playFromAction = static_cast<EAction>(playAction | ACT_PLAYFROM);

    //                      ROON                YIO COMMAND     TYPE        PARENTTITLE     ACTION          FORCEDACTION
    //                      FORCEDCHILDACTION
    _actions.append(Action("Play Genre", "", "Genre", "Genres", ACT_ACTIONLIST, shuffleAction, shuffleAction));
    _actions.append(Action("Play Artist", "", "Artist", "Artists", ACT_ACTIONLIST, shuffleAction, playAction));
    _actions.append(Action("Play Album", "", "Album", "Albums", ACT_ACTIONLIST, playAction, playFromAction));
    _actions.append(
        Action("Play Playlist", "", "Playlist", "PlayLists", ACT_ACTIONLIST, shuffleAction, playFromAction));
    _actions.append(Action("Play Now", "Play Now", "Track", "Tracks", ACT_PLAYNOW, ACT_NONE, ACT_NONE));
    _actions.append(Action("Play Now", "Play From", "", "", ACT_PLAYFROM, ACT_NONE, ACT_NONE));
    _actions.append(Action("Shuffle", "Shuffle", "", "", ACT_SHUFFLE, ACT_NONE, ACT_NONE));
    _actions.append(Action("Add Next", "Add Next", "", "", ACT_ADDNEXT, ACT_NONE, ACT_NONE));
    _actions.append(Action("Queue", "Queue", "", "", ACT_QUEUE, ACT_NONE, ACT_NONE));
    _actions.append(Action("Start Radio", "Start Radio", "", "", ACT_STARTRADIO, ACT_NONE, ACT_NONE));
    //_actions.append(Action ("TIDAL",            "Search",       "",         "",             ACT_REJECT,     ACT_NONE,
    // ACT_NONE));
    _actions.append(Action("Settings", "", "", "", ACT_REJECT, ACT_NONE, ACT_NONE));
    _actions.append(Action("Tags", "", "", "", ACT_REJECT, ACT_NONE, ACT_NONE));

    _reg.display_name    = "YIO Roon Control";
    _reg.display_version = "1.1";
    _reg.email           = "ric@rts.co.at";
    _reg.extension_id    = "com.roon.yiocontrol";
    _reg.publisher       = "Christian Riedl";
    _reg.token           = "";
    _reg.website         = "https://www.christian-riedl.com";
    _reg.provided_services.append(QtRoonApi::ServicePairing);
    _reg.required_services.append(QtRoonApi::ServiceBrowse);
    _reg.required_services.append(QtRoonApi::ServiceTransport);
    //_reg.required_services.append(QtRoonApi::ServiceImage);
    //_reg.provided_services.append(QtRoonApi::ServicePing);
    _reg.paired = this;

    QObject::connect(&_transportApi, &QtRoonTransportApi::zonesChanged, this, &YioRoon::onZonesChanged);
    QObject::connect(&_transportApi, &QtRoonTransportApi::zoneSeekChanged, this, &YioRoon::onZoneSeekChanged);
    QObject::connect(&_roonApi, &QtRoonApi::error, this, &YioRoon::onError);
    QObject::connect(&_roonApi, &QtRoonApi::stateChanged, this, &YioRoon::storeState);
    _instance = this;


    QVariantMap roonConfig;
    for (QVariantMap::const_iterator iter = config.begin(); iter != config.end(); ++iter) {
        if (iter.key() == Integration::OBJ_DATA) {
            QVariantMap map = iter.value().toMap();

            QString ip = map.value(Integration::KEY_DATA_IP).toString();
            if (!ip.contains(':')) {
                ip += ":9100";
            }
            _url      = "ws://" + ip + "/api";
            _imageUrl = "http://" + ip + "/api/image/";

            roonConfig = map.value("state").toMap();
        }
    }

    if (roonConfig.isEmpty()) {
        // For Compatibility reason, read the old state file
        // TODO: Remove in  the future
        _configPath = configObj->getSettings().value("configPath").toString() + "/roon";
        if (QDir(_configPath).exists()) {
            QFile file(_configPath + "/roonState.json");
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCWarning(m_logCategory) << "loadState can't open " << file.fileName();
            } else {
                QByteArray data = file.readAll();
                file.close();
                QJsonDocument document = QJsonDocument::fromJson(data);
                roonConfig = document.toVariant().toMap();

                storeState(roonConfig);
            }
        }
    }
    _roonApi.setup(_url, roonConfig);

    setState(CONNECTING);
    _roonApi.open();
}

YioRoon::~YioRoon() {
    _instance = nullptr;
    if (_items != nullptr) {
        delete[] _items;
    }
}

void YioRoon::connect() {
    if (_contexts.length() == 0) {
        QVariantList            emptyList;
        QStringList             emptyButtons;
        QList<EntityInterface*> list = m_entities->getByIntegration(integrationId());
        for (int i = 0; i < list.length(); i++) {
            EntityInterface* entity = list[i];
            _contexts.append(YioContext(i, entity->entity_id(), entity->friendly_name()));
            MediaPlayerInterface* mpIf = static_cast<MediaPlayerInterface*>(entity->getSpecificInterface());
            mpIf->setBrowseModel(&_model);
        }
        _items = new QList<QtRoonBrowseApi::BrowseItem>[static_cast<size_t>(list.length())];
    }
    setState(CONNECTING);
    _roonApi.open();
}

void YioRoon::disconnect() {
    setState(DISCONNECTED);
    _roonApi.close();
    _contexts.clear();
    delete[] _items;
}

void YioRoon::enterStandby() { _transportApi.unsubscribeZones(_subscriptionKey); }

void YioRoon::leaveStandby() { _subscriptionKey = _transportApi.subscribeZones(); }

void YioRoon::sendCommand(const QString& type, const QString& entityId, int cmd, const QVariant& param) {
    /*
        "SOURCE", "APP_NAME",
        "VOLUME", "VOLUME_UP", "VOLUME_DOWN", "VOLUME_SET","MUTE","MUTE_SET",
        "MEDIA_TYPE", "MEDIA_TITLE", "MEDIA_ARTIST", "MEDIA_ALBUM", "MEDIA_DURATION", "MEDIA_POSITION", "MEDIA_IMAGE",
        "PLAY", "PAUSE", "STOP", "PREVIOUS", "NEXT", "SEEK", "SHUFFLE", "TURN_ON", "TURN_OFF"
    */
    if (m_logCategory.isDebugEnabled())
        qCDebug(m_logCategory) << "sendCommand " << type << " " << entityId << " " << cmd << " " << param.toString();

    int idx;
    for (idx = 0; idx < _contexts.length(); idx++) {
        if (_contexts[idx].entityId == entityId)
            break;
    }
    if (idx >= _contexts.length()) {
        qCWarning(m_logCategory) << "can't find id " << entityId;
        return;
    }
    YioContext& ctx = _contexts[idx];

    const QtRoonTransportApi::Zone& zone = _transportApi.getZone(ctx.zoneId);
    if (zone.outputs.count() == 0) {
        qCWarning(m_logCategory) << "No output for zone" << ctx.zoneId;
        return;
    }
    const QtRoonTransportApi::Output& output = zone.outputs[0];
    switch (static_cast<MediaPlayerDef::Commands>(cmd)) {
        case MediaPlayerDef::C_VOLUME_SET:
            _transportApi.changeVolume(output.output_id, QtRoonTransportApi::EValueMode::absolute, param.toInt());
            break;
        case MediaPlayerDef::C_VOLUME_UP:
            _transportApi.changeVolume(output.output_id, QtRoonTransportApi::EValueMode::relative_step, 1);
            break;
        case MediaPlayerDef::C_VOLUME_DOWN:
            _transportApi.changeVolume(output.output_id, QtRoonTransportApi::EValueMode::relative_step, -1);
            break;
        case MediaPlayerDef::C_MUTE:
            _transportApi.mute(output.output_id,
                               param.toBool() ? QtRoonTransportApi::EMute::mute : QtRoonTransportApi::EMute::unmute);
            break;
        case MediaPlayerDef::C_PLAY:
            _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::play);
            break;
        case MediaPlayerDef::C_PAUSE:
            _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::pause);
            break;
        case MediaPlayerDef::C_STOP:
            _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::stop);
            break;
        case MediaPlayerDef::C_PREVIOUS:
            _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::previous);
            break;
        case MediaPlayerDef::C_NEXT:
            _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::next);
            break;
        case MediaPlayerDef::C_BROWSE: {
            QString cmd = param.toString();
            if (cmd == "TOP") {
                browse(ctx, true);
            } else if (cmd == "BACK") {
                browseBack(ctx);
            } else {
                const QList<QtRoonBrowseApi::BrowseItem>& items = _items[idx];
                for (int i = 0; i < items.length(); i++) {
                    if (items[i].item_key == cmd) {
                        browse(ctx, items, i, false);
                        return;
                    }
                }
                const Action* action = getActionYio(cmd);
                if (action == nullptr) {
                    qCWarning(m_logCategory) << "Can't find Browse command " << cmd;
                    return;
                }
                for (int i = 0; i < items.length(); i++) {
                    if (items[i].hint == "action_list") {
                        const Action* actionList = getActionRoon(items[i].title);
                        if (actionList != nullptr) {
                            ctx.forcedAction = cmd;
                            browseAction(ctx, items[i].item_key);
                            return;
                        }
                    }
                    if (items[i].hint == "action") {
                        if (items[i].title == action->roonName) {
                            browse(ctx, items, i, true);
                            return;
                        }
                    }
                }
                qCWarning(m_logCategory) << "Can't find action list" << action->roonName;
            }
        } break;
        case MediaPlayerDef::C_PLAY_ITEM: {
            QVariantMap map      = param.toMap();
            QString     pcmd     = map["command"].toString();
            pcmd                 = "Play Now";  // @@@ temporary
            QString       type   = map["type"].toString();
            QString       key    = map["id"].toString();
            const Action* action = getActionYio(pcmd);
            if (action != nullptr) {
                ctx.forcedAction = pcmd;
                playMedia(ctx, key, true);
            }
        } break;
        case MediaPlayerDef::C_SEARCH_ITEM: {
            QVariantMap map  = param.toMap();
            QString     scmd = map["text"].toString();
            QString     key  = map["key"].toString();
            search(ctx, scmd, key);
        } break;
        case MediaPlayerDef::C_SEARCH:
            search(ctx, param.toString());
            break;
        case MediaPlayerDef::C_GETALBUM:
            getAlbum(ctx, param.toString());
            break;
        default: {
            EntityInterface* entity = m_entities->getEntityInterface(ctx.entityId);
            qCWarning(m_logCategory) << "Illegal command" << entity->getCommandName(cmd);
        } break;
    }
}

void YioRoon::browse(YioContext& ctx, bool fromTop) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, fromTop, false, 0);
    ctx.browseMode = BROWSE;
    _browseApi.browse(opt, ctx);
}

void YioRoon::browse(YioContext& ctx, const QList<QtRoonBrowseApi::BrowseItem>& items, int itemIndex, bool action) {
    if (itemIndex < items.count()) {
        QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, items[itemIndex].item_key, action ? -1 : 0);
        ctx.browseMode = BROWSE;
        _browseApi.browse(opt, ctx);
    }
}

void YioRoon::browseBack(YioContext& ctx) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, 1, 0);
    ctx.browseMode = BROWSE;
    _browseApi.browse(opt, ctx);
}

void YioRoon::browseRefresh(YioContext& ctx) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, false, true, 0);
    ctx.browseMode = BROWSE;
    _browseApi.browse(opt, ctx);
}

void YioRoon::search(YioContext& ctx, const QString& searchText, const QString& itemKey) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, itemKey, 0);
    opt.input      = searchText;
    ctx.browseMode = BROWSE;
    _browseApi.browse(opt, ctx);
}

void YioRoon::search(YioContext& ctx, const QString& searchText) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, true, false, 0);
    ctx.browseMode  = GOTOPATH;
    ctx.searchText  = searchText;
    ctx.searchModel = nullptr;  // @@@ delete ?
    ctx.goToPath    = QStringList({"Library", "Search"});
    ctx.albumMap.clear();
    _browseApi.browse(opt, ctx);
}

void YioRoon::getAlbum(YioContext& ctx, const QString& itemKey) {
    QString albumTitle = ctx.albumMap.value(itemKey);
    if (!albumTitle.isEmpty()) {
        QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, true, false, 0);
        ctx.goToPath   = QStringList({"Library", "Albums", albumTitle});
        ctx.browseMode = GETALBUM;
        _browseApi.browse(opt, ctx);
    }
}

void YioRoon::playMedia(YioContext& ctx, const QString& itemKey, bool setItemIndex) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, itemKey);
    ctx.browseMode = PLAY;
    _browseApi.browse(opt, ctx);
    if (setItemIndex) {
        ctx.queueFrom = false;
        ctx.gotoNext  = false;
        ctx.itemIndex = findItemIndex(_items[ctx.index], itemKey);
    }
}

void YioRoon::browseAction(YioContext& ctx, const QString& itemKey) {
    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, itemKey);
    ctx.browseMode = ACTION;
    _browseApi.browse(opt, ctx);
}

void YioRoon::onZonesChanged() {
    if (state() != CONNECTED) {
        setState(CONNECTED);
    }

    QMap<QString, QtRoonTransportApi::Zone>& zones = _transportApi.zones();
    for (QMap<QString, QtRoonTransportApi::Zone>::iterator i = zones.begin(); i != zones.end(); ++i) {
        const QtRoonTransportApi::Zone& zone = i.value();

        QStringList supportedFeatures;
        addAvailableEntity(
            zone.zone_id,
            "media_player",
            integrationId(),
            zone.display_name,
            supportedFeatures);
    }

    for (QMap<QString, QtRoonTransportApi::Zone>::iterator i = zones.begin(); i != zones.end(); ++i) {
        const QtRoonTransportApi::Zone& zone = i.value();

        int idx;
        for (idx = 0; idx < _contexts.length(); idx++) {
            if (_contexts[idx].friendlyName == zone.display_name) {
                break;
            }
        }

        if (idx >= _contexts.length()) {
            if (_notFound.indexOf(zone.zone_id) < 0) {
                _notFound.append(zone.zone_id);
                qCWarning(m_logCategory) << "can't find zone " << zone.display_name;
            }
        } else {
            _contexts[idx].zoneId = zone.zone_id;
            updateZone(_contexts[idx], zone, false);
        }
    }
}

void YioRoon::onZoneSeekChanged(const QtRoonTransportApi::Zone& zone) {
    int idx;
    for (idx = 0; idx < _contexts.length(); idx++) {
        if (_contexts[idx].zoneId == zone.zone_id) {
            break;
        }
    }
    if (idx >= _contexts.length()) {
        if (_notFound.indexOf(zone.zone_id) < 0) {
            _notFound.append(zone.zone_id);
            qCWarning(m_logCategory) << "can't find seek zone" << zone.display_name;
        }
    } else {
        updateZone(_contexts[idx], zone, true);
    }
}
void YioRoon::onError(const QString& error) {
    Q_UNUSED(error)
    QObject* param = this;
    m_notifications->add(
        true, tr("Cannot connect to ").append(friendlyName()).append("."), tr("Reconnect"),
        [](QObject* param) {
            Integration* i = qobject_cast<Integration*>(param);
            i->connect();
        },
        param);
}

void YioRoon::OnPaired(const RoonCore& core) {
    Q_UNUSED(core)
    _subscriptionKey = _transportApi.subscribeZones();
}

void YioRoon::OnUnpaired(const RoonCore& core) { Q_UNUSED(core) }

void YioRoon::transportCallback(int requestId, const QString& msg) {
    Q_UNUSED(requestId)
    Q_UNUSED(msg)
}

void YioRoon::OnBrowse(const QString& err, QtRoonBrowseApi::Context& context,
                       const QtRoonBrowseApi::BrowseResult& result) {
    Q_UNUSED(err)

    YioContext&                 ctx = static_cast<YioContext&>(context);
    QtRoonBrowseApi::LoadOption opt;

    if (!err.isEmpty()) {
        updateError(ctx, err);
        return;
    }

    if (result.action == "list" && result.list != nullptr) {
        // Adjust path
        while (result.list->level < ctx.path.length()) {
            ctx.path.removeLast();
        }
        ctx.path.append(result.list->title);
        if (m_logCategory.isDebugEnabled()) {
            qCDebug(m_logCategory) << "PATH: " << ctx.path.join('/');
        }

        int listoffset = result.list->display_offset > 0 ? result.list->display_offset : 0;
        switch (ctx.browseMode) {
            case BROWSE:
            case ACTION:
            case GOTOPATH:
            case GETALBUM:
                opt.offset             = listoffset;
                opt.count              = _numItems;
                opt.set_display_offset = listoffset;
                _browseApi.load(opt, ctx);
                break;
            case PLAY:
                opt.offset = 0;
                opt.count  = 5;  // Max number of actions
                _browseApi.load(opt, ctx);
                break;
        }
    }
}

void YioRoon::OnLoad(const QString& err, QtRoonBrowseApi::Context& context, const QtRoonBrowseApi::LoadResult& result) {
    Q_UNUSED(err)

    const Action* action;
    YioContext&   ctx = static_cast<YioContext&>(context);

    switch (ctx.browseMode) {
        case BROWSE:
            updateItems(ctx, result);
            break;
        case PLAY:
            if (result.items.length() > 0) {
                action = getActionYio(ctx.forcedAction);
                if (action != nullptr && !ctx.gotoNext) {
                    QString roonName = ctx.queueFrom ? "Queue" : action->roonName;
                    for (int j = 0; j < result.items.length(); j++) {
                        if (result.items[j].title == roonName) {
                            playMedia(ctx, result.items[j].item_key);
                            if (!!(action->action & ACT_PLAYFROM) && ctx.itemIndex < _items[ctx.index].length() - 1) {
                                ctx.gotoNext  = true;
                                ctx.queueFrom = true;
                                return;  // Dont clear forced
                            }
                            ctx.forcedAction = "";
                            return;
                        }
                    }
                    // Not found
                    const Action* playAction = getActionRoon(result.items[0].title);
                    if (playAction != nullptr) {
                        playMedia(ctx, result.items[0].item_key);
                        ctx.goBack = 1;  // requires go back
                        return;
                    }
                }
                // Handle play from
                if (ctx.gotoNext && action != nullptr && !!(action->action & ACT_PLAYFROM)) {
                    QList<QtRoonBrowseApi::BrowseItem>& items = _items[ctx.index];
                    ctx.itemIndex++;
                    ctx.gotoNext = false;
                    if (ctx.itemIndex < items.length()) {
                        playMedia(ctx, items[ctx.itemIndex].item_key);
                    } else {
                        ctx.forcedAction = "";
                    }
                }
                // perform necessary go back
                if (ctx.goBack > 0) {
                    browseBack(ctx);
                    ctx.goBack = 0;
                }
            }
            break;
        case ACTION:
            action = _instance->getActionYio(ctx.forcedAction);
            if (action != nullptr) {
                for (int j = 0; j < result.items.length(); j++) {
                    const QtRoonBrowseApi::BrowseItem& item = result.items[j];
                    if (item.title == action->roonName) {
                        // thats it
                        browse(ctx, result.items, j, true);
                        ctx.forcedAction = "";
                    }
                }
            }
            break;
        case GOTOPATH:
            if (ctx.goToPath.length() == 0) {
                // return of search operation
                if (ctx.searchModel == nullptr) {
                    ctx.searchModel = new SearchModel();
                    for (int j = 0; j < result.items.length(); j++) {
                        const QtRoonBrowseApi::BrowseItem& item = result.items.at(j);
                        if (item.title == "Artists") {
                            ctx.searchKeys.append(item.item_key);
                            ctx.searchModel->append(new SearchModelItem("artists", new SearchModelList()));
                        }
                        if (item.title == "Albums") {
                            ctx.searchKeys.append(item.item_key);
                            ctx.searchModel->append(new SearchModelItem("albums", new SearchModelList()));
                        }
                        if (item.title == "Tracks") {
                            ctx.searchKeys.append(item.item_key);
                            ctx.searchModel->append(new SearchModelItem("tracks", new SearchModelList()));
                        }
                    }
                } else {
                    // return of type search
                    if (updateSearch(ctx, result)) {
                        // End
                        ctx.searchModel = nullptr;
                        ctx.searchText  = "";
                    }
                }
                if (ctx.searchKeys.length() > 0) {
                    QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, ctx.searchKeys[0]);
                    ctx.searchKeys.removeAt(0);
                    _browseApi.browse(opt, ctx);
                }
                return;
            }
            for (int j = 0; j < result.items.length(); j++) {
                const QtRoonBrowseApi::BrowseItem& item = result.items[j];
                if (item.title == ctx.goToPath[0]) {
                    // thats it
                    ctx.goToPath.removeAt(0);
                    if (ctx.goToPath.length() > 0) {
                        // continue search
                        QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, item.item_key);
                        _browseApi.browse(opt, ctx);
                    } else {
                        // found
                        QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, item.item_key, 0);
                        if (!ctx.searchText.isEmpty())
                            opt.input = ctx.searchText;
                        _browseApi.browse(opt, ctx);
                    }
                    break;
                }
            }
            break;
        case GETALBUM:
            if (ctx.goToPath.length() == 0) {
                updateAlbum(ctx, result);
            } else {
                for (int j = 0; j < result.items.length(); j++) {
                    const QtRoonBrowseApi::BrowseItem& item = result.items[j];
                    if (item.title == ctx.goToPath[0]) {
                        // thats it
                        ctx.goToPath.removeAt(0);
                        if (ctx.goToPath.length() > 0) {
                            // continue search
                            QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, item.item_key);
                            _browseApi.browse(opt, ctx);
                        } else {
                            // found
                            QtRoonBrowseApi::BrowseOption opt(ctx.zoneId, item.item_key, 0);
                            _browseApi.browse(opt, ctx);
                        }
                        break;
                    }
                }
            }
            break;
    }
}

void YioRoon::updateZone(YioContext& ctx, const QtRoonTransportApi::Zone& zone, bool seekChanged) {
    /*
        Q_PROPERTY  (states         state       READ    state       NOTIFY      stateChanged)
        Q_PROPERTY  (double         volume      READ    volume      NOTIFY      volumeChanged)
        Q_PROPERTY  (bool           muted       READ    muted       NOTIFY      mutedChanged)
        Q_PROPERTY  (QString        mediaType   READ    mediaType   NOTIFY      mediaTypeChanged)
        Q_PROPERTY  (QString        mediaTitle  READ    mediaTitle  NOTIFY      mediaTitleChanged)
        Q_PROPERTY  (QString        mediaArtist READ    mediaArtist NOTIFY      mediaArtistChanged)
        Q_PROPERTY  (QString        mediaImage  READ    mediaImage  NOTIFY      mediaImageChanged)
        Q_PROPERTY  (QString        source      READ    source      NOTIFY      sourceChanged)
    */

    EntityInterface* entity = m_entities->getEntityInterface(ctx.entityId);
    if (!seekChanged) {
        MediaPlayerDef::States state;
        switch (zone.state) {
            case QtRoonTransportApi::EState::unknown:
            case QtRoonTransportApi::EState::stopped:
                state = MediaPlayerDef::OFF;
                break;
            case QtRoonTransportApi::EState::playing:
                state = MediaPlayerDef::PLAYING;
                break;
            case QtRoonTransportApi::EState::paused:
                state = MediaPlayerDef::ON;
                break;
            case QtRoonTransportApi::EState::loading:
                state = MediaPlayerDef::IDLE;
                break;
        }
        entity->updateAttrByIndex(MediaPlayerDef::STATE, static_cast<int>(state));
        if (zone.outputs.length() > 0 && zone.outputs[0].volume != nullptr) {
            const QtRoonTransportApi::Volume* volume = zone.outputs[0].volume;
            entity->updateAttrByIndex(MediaPlayerDef::VOLUME, volume->value);
            entity->updateAttrByIndex(MediaPlayerDef::MUTED, volume->is_muted);
        }
        if (zone.now_playing != nullptr) {
            const QtRoonTransportApi::NowPlaying* np = zone.now_playing;
            if (!np->image_key.isEmpty()) {
                entity->updateAttrByIndex(MediaPlayerDef::MEDIAIMAGE,
                                          _imageUrl + np->image_key + "?scale=fit&width=256&height=256");
            } else {
                entity->updateAttrByIndex(MediaPlayerDef::MEDIAIMAGE, "");
            }
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAARTIST, np->two_line->line2);
            entity->updateAttrByIndex(MediaPlayerDef::MEDIATITLE, np->two_line->line1);
            entity->updateAttrByIndex(MediaPlayerDef::MEDIADURATION, np->length);
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAPROGRESS, np->seek_position);
        } else {
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAIMAGE, "");
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAARTIST, "");
            entity->updateAttrByIndex(MediaPlayerDef::MEDIATITLE, "");
            entity->updateAttrByIndex(MediaPlayerDef::MEDIADURATION, 0);
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAPROGRESS, zone.seek_position);
        }
        entity->updateAttrByIndex(MediaPlayerDef::SOURCE, "");
        entity->updateAttrByIndex(MediaPlayerDef::MEDIATYPE, "");
    } else {
        entity->updateAttrByIndex(MediaPlayerDef::MEDIAPROGRESS, zone.seek_position);
        if (zone.now_playing != nullptr) {
            const QtRoonTransportApi::NowPlaying* np = zone.now_playing;
            entity->updateAttrByIndex(MediaPlayerDef::MEDIADURATION, np->length);
        }
    }
}
void YioRoon::updateItems(YioContext& ctx, const QtRoonBrowseApi::LoadResult& result) {
    QStringList playCommands;
    int         level = 0;

    int                                 idx   = ctx.index;
    QList<QtRoonBrowseApi::BrowseItem>& items = _items[idx];
    items.clear();
    QString browseType;

    if (_cmdsForItem && result.list != nullptr) {
        const Action* action = getActionParentTitle(result.list->title);
        if (action != nullptr) {
            QStringList btns = getForcedActions(action->forcedActions);
            playCommands.append(btns);
        }
    }

    for (int i = 0; i < result.items.length(); i++) {
        const QtRoonBrowseApi::BrowseItem& item = result.items[i];
        items.append(item);

        if (_cmdsForItem) {
            if (item.hint == "action_list" || level == 0) {
                const Action* action = getActionRoon(item.title);
                if (action != nullptr) {
                    if (action->action & ACT_REJECT) {
                        continue;
                    }
                    if ((action->action & ACT_ACTIONLIST)) {
                        QStringList btns = getForcedActions(action->forcedChildActions);
                        playCommands.append(btns);
                        browseType = action->type;
                        continue;
                    }
                }
            }
        } else {
            if (item.hint == "action_list" || level == 0) {
                const Action* action = getActionRoon(item.title);
                if (action != nullptr) {
                    if (action->action & ACT_REJECT) {
                        continue;
                    }
                    if ((action->action & ACT_ACTIONLIST)) {
                        QStringList btns = getForcedActions(action->forcedActions);
                        playCommands.append(btns);
                        browseType = action->type;
                        continue;
                    }
                }
            }
            if (item.hint == "action") {
                const Action* action = getActionRoon(item.title);
                if (action != nullptr) {
                    playCommands.append(action->yioName);
                    browseType = action->type;
                    continue;
                }
            }
        }

        QString url = "";
        if (item.image_key.isEmpty()) {
            url = "qrc:/images/mini-music-player/no_image.png";
        } else {
            url = _imageUrl + item.image_key + "?scale=fit&width=64&height=64";
        }
        _model.addItem(item.item_key, item.title, item.subtitle, browseType, url, playCommands);
    }

    EntityInterface*      entity      = m_entities->getEntityInterface(ctx.entityId);
    MediaPlayerInterface* mediaPlayer = static_cast<MediaPlayerInterface*>(entity->getSpecificInterface());
    mediaPlayer->setBrowseModel(&_model);
}
bool YioRoon::updateSearch(YioContext& ctx, const QtRoonBrowseApi::LoadResult& result) {
    //    QList<SearchModelItem*> items = ctx.searchModel->items();
    SearchModel* items = ctx.searchModel;

    int count = 0;
    for (int i = 0; i < items->count(); i++) {
        const SearchModelItem* item = items->get(i);
        if (updateSearchList(ctx, item, result)) {
            count++;
        }
    }

    if (count == items->count()) {
        EntityInterface*      entity      = m_entities->getEntityInterface(ctx.entityId);
        MediaPlayerInterface* mediaPlayer = static_cast<MediaPlayerInterface*>(entity->getSpecificInterface());
        mediaPlayer->setSearchModel(ctx.searchModel);
        return true;
    }

    return false;
}

bool YioRoon::updateSearchList(YioContext& ctx, const SearchModelItem* item,
                               const QtRoonBrowseApi::LoadResult& result) {
    SearchModelList* list = qobject_cast<SearchModelList*>(item->item_model());
    if (list->rowCount() > 0) {
        return true;  // already filled
    }
    if (result.list != nullptr && result.list->title.toLower() == item->item_type()) {
        // Fill list
        QString url;
        if (item->item_type() == "artists") {
            QStringList commands = {"ARTISTRADIO"};
            for (int i = 0; i < result.items.length(); i++) {
                const QtRoonBrowseApi::BrowseItem& item = result.items.at(i);
                if (item.image_key.isEmpty()) {
                    url = "qrc:/images/mini-music-player/no_image.png";
                } else {
                    url = _imageUrl + item.image_key + "?scale=fit&width=64&height=64";
                }
                SearchModelListItem it =
                    SearchModelListItem(item.item_key, "artist", item.title, item.subtitle, url, commands);
                list->append(it);
            }
            return true;
        }
        if (item->item_type() == "albums") {
            QStringList commands = {"PLAY"};
            for (int i = 0; i < result.items.length(); i++) {
                const QtRoonBrowseApi::BrowseItem& item = result.items.at(i);
                ctx.albumMap.insert(item.item_key, item.title);
                if (item.image_key.isEmpty()) {
                    url = "qrc:/images/mini-music-player/no_image.png";
                } else {
                    url = _imageUrl + item.image_key + "?scale=fit&width=64&height=64";
                }
                SearchModelListItem it =
                    SearchModelListItem(item.item_key, "album", item.title, item.subtitle, url, commands);
                list->append(it);
            }
            return true;
        }
        if (item->item_type() == "tracks") {
            QStringList commands = {"PLAY"};
            for (int i = 0; i < result.items.length(); i++) {
                const QtRoonBrowseApi::BrowseItem& item = result.items.at(i);
                if (item.image_key.isEmpty()) {
                    url = "qrc:/images/mini-music-player/no_image.png";
                } else {
                    url = _imageUrl + item.image_key + "?scale=fit&width=64&height=64";
                }
                SearchModelListItem it =
                    SearchModelListItem(item.item_key, "track", item.title, item.subtitle, url, commands);
                list->append(it);
            }
            return true;
        }
    }
    return false;
}

void YioRoon::updateAlbum(YioContext& ctx, const QtRoonBrowseApi::LoadResult& result) {
    if (result.list == nullptr) {
        return;
    }
    QStringList commands = {"PLAY", "SONGRADIO"};

    QString url;
    if (result.list->image_key.isEmpty()) {
        url = "qrc:/images/mini-music-player/no_image.png";
    } else {
        url = _imageUrl + result.list->image_key + "?scale=fit&width=64&height=64";
    }
    BrowseModel* album =
        new BrowseModel(nullptr, "", result.list->title, result.list->subtitle, "album", url, commands);

    for (int i = 0; i < result.items.length(); i++) {
        const QtRoonBrowseApi::BrowseItem& item = result.items.at(i);
        if (item.title == "Play Album") {
            continue;
        }
        album->addItem(item.item_key, item.subtitle, item.title, "track", "", commands);
    }
    // update the entity
    EntityInterface* entity = m_entities->getEntityInterface(ctx.entityId);
    if (entity != nullptr) {
        MediaPlayerInterface* me = static_cast<MediaPlayerInterface*>(entity->getSpecificInterface());
        me->setBrowseModel(album);
    }
}

void YioRoon::updateError(YioContext& ctx, const QString& error) {
    Q_UNUSED(ctx)
    m_notifications->add(true, "ROON browsing error : " + error);
}

const YioRoon::Action* YioRoon::getActionRoon(const QString& roonName) {
    if (roonName == "") {
        return nullptr;
    }
    for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
        if (iter->roonName == roonName) {
            return &*iter;
        }
    }
    return nullptr;
}

const YioRoon::Action* YioRoon::getActionYio(const QString& yioName) {
    if (yioName == "") {
        return nullptr;
    }
    for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
        if (iter->yioName == yioName) {
            return &*iter;
        }
    }
    return nullptr;
}

const YioRoon::Action* YioRoon::getActionParentTitle(const QString& parentTitle) {
    if (parentTitle == "") {
        return nullptr;
    }
    for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
        if (iter->parentTitle == parentTitle) {
            return &*iter;
        }
    }
    return nullptr;
}

QStringList YioRoon::getForcedActions(EAction forcedActions) {
    EAction     actions[] = {ACT_PLAYNOW, ACT_PLAYFROM, ACT_SHUFFLE, ACT_ADDNEXT, ACT_QUEUE, ACT_STARTRADIO};
    QStringList list;
    for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) {
        if (forcedActions & actions[i]) {
            for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
                if (iter->action == actions[i]) {
                    list.append(iter->yioName);
                    break;
                }
            }
        }
    }
    return list;
}

int YioRoon::findItemIndex(const QList<QtRoonBrowseApi::BrowseItem>& items, const QString& itemKey) {
    for (int i = 0; i < items.length(); i++) {
        if (items[i].item_key == itemKey)
            return i;
    }
    return -1;
}

void YioRoon::storeState(const QVariantMap& stateMap) {
    QVariantMap config = m_config->getConfig();

    QVariantMap integrations  = config.value("integrations").toMap();
    QVariantMap roon = integrations.value("roon").toMap();
    QVariantList data = roon.value(Integration::OBJ_DATA).toList();
    QVariantList newData;
    for (QVariantList::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
        QVariantMap integration = iter->toMap();
        if (integration.value(Integration::KEY_ID) == integrationId()) {
            QVariantMap data_data = integration.value(Integration::OBJ_DATA).toMap();
            data_data["state"] = stateMap;
            integration.insert(Integration::OBJ_DATA, data_data);
        }
        newData.append(integration);
    }

    roon.insert(Integration::OBJ_DATA, newData);
    integrations.insert("roon", roon);
    config.insert("integrations", integrations);

    m_config->setConfig(config);
}
