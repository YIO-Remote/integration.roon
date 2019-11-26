#define PERF_TEST   0
#include "YioRoon.h"
#include "../remote-software/sources/entities/mediaplayer.h"
#include "../remote-software/sources/configinterface.h"
#include "../remote-software/sources/entities/mediaplayerinterface.h"

Roon::Roon(QObject* parent) :
    _log("roon"),
    _discovery(_log, parent)
{
}

void Roon::create(const QVariantMap &config, QObject *entities, QObject *notifications, QObject *api, QObject *configObj)
{
    QMap<QObject *, QVariant>   returnData;
    QVariantList                data;

    for (QVariantMap::const_iterator iter = config.begin(); iter != config.end(); ++iter) {
        if (iter.key() == "data") {
            data = iter.value().toList();
            break;
        }
    }
    for (int i = 0; i < data.length(); i++)
    {
        YioRoon* roon = new YioRoon(this);
        roon->setup(data[i].toMap(), entities, notifications, api, configObj);

        QVariantMap d = data[i].toMap();
        d.insert("type", config.value("type").toString());
        returnData.insert(roon, d);
    }
    if (data.length() > 0)
        emit createDone(returnData);
    else {
        connect (&_discovery, &QtRoonDiscovery::roonDiscovered, this, &Roon::onRoonDiscovered);
        _discovery.startDiscovery(1000, true);
    }
}
void Roon::onRoonDiscovered (QMap<QString, QVariantMap> soodmaps)
{
    QVariantMap roonsmap;

    int idx = 1;
    for (QMap<QString, QVariantMap>::const_iterator iter = soodmaps.begin(); iter != soodmaps.end(); ++iter) {
        QVariantMap soodmap = iter->value(iter.key()).toMap();
        QVariantMap roonmap;
        roonmap["ip"] = iter.key();
        roonmap["type"] = "roon";
        roonmap["friendly_name"] = soodmap["name"];
        roonmap["id"] = "roon" + (idx > 1 ? QString::number(idx) : "");
        roonsmap["data"] = roonmap;
        idx++;
    }
    // @@@RIC
}

QLoggingCategory YioRoon::_log("roon");
YioRoon* YioRoon::_instance = nullptr;

YioRoon::YioRoon(QObject* parent) :
    _roonApi("ws://192.168.1.100:9100/api", "C:\\Temp\\", _reg, _log),
    _browseApi(_roonApi, this),
    _transportApi(_roonApi, transportCallback),
    _numItems(50),
    _items(nullptr),
    _cmdsForItem(true)
{
    setParent (parent);

    EAction shuffleAction = static_cast<EAction> (ACT_SHUFFLE | ACT_STARTRADIO);
    EAction playAction = static_cast<EAction> (ACT_PLAYNOW | ACT_ADDNEXT | ACT_QUEUE | ACT_STARTRADIO);
    EAction playFromAction = static_cast<EAction> (playAction | ACT_PLAYFROM);

    //                      ROON                YIO COMMAND     TYPE        PARENTTITLE     ACTION          FORCEDACTION    FORCEDCHILDACTION
    _actions.append(Action ("Play Genre",       "",             "Genre",    "Genres",       ACT_ACTIONLIST, shuffleAction,  shuffleAction));
    _actions.append(Action ("Play Artist",      "",             "Artist",   "Artists",      ACT_ACTIONLIST, shuffleAction,  playAction));
    _actions.append(Action ("Play Album",       "",             "Album",    "Albums",       ACT_ACTIONLIST, playAction,     playFromAction));
    _actions.append(Action ("Play Playlist",    "",             "Playlist", "PlayLists",    ACT_ACTIONLIST, shuffleAction,  playFromAction));
    _actions.append(Action ("Play Now",         "Play Now",     "Track",    "Tracks",       ACT_PLAYNOW,    ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Play Now",         "Play From",    "",         "",             ACT_PLAYFROM,   ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Shuffle",          "Shuffle",      "",         "",             ACT_SHUFFLE,    ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Add Next",         "Add Next",     "",         "",             ACT_ADDNEXT,    ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Queue",            "Queue",        "",         "",             ACT_QUEUE,      ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Start Radio",      "Start Radio",  "",         "",             ACT_STARTRADIO, ACT_NONE,       ACT_NONE));
    //_actions.append(Action ("TIDAL",            "Search",       "",         "",             ACT_REJECT,     ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Settings",         "",             "",         "",             ACT_REJECT,     ACT_NONE,       ACT_NONE));
    _actions.append(Action ("Tags",             "",             "",         "",             ACT_REJECT,     ACT_NONE,       ACT_NONE));

    _reg.display_name = "YIO Roon Control";
    _reg.display_version = "1.1";
    _reg.email = "ric@rts.co.at";
    _reg.extension_id = "com.roon.yiocontrol";
    _reg.publisher = "Christian Riedl";
    _reg.token = "";
    _reg.website = "https://www.christian-riedl.com";
    _reg.provided_services.append(QtRoonApi::ServicePairing);
    _reg.required_services.append(QtRoonApi::ServiceBrowse);
    _reg.required_services.append(QtRoonApi::ServiceTransport);
    //_reg.required_services.append(QtRoonApi::ServiceImage);
    //_reg.provided_services.append(QtRoonApi::ServicePing);
    _reg.paired = this;

    QObject::connect(&_transportApi, &QtRoonTransportApi::zonesChanged, this, &YioRoon::onZonesChanged);
    QObject::connect(&_transportApi, &QtRoonTransportApi::zoneSeekChanged, this, &YioRoon::onZoneSeekChanged);
    QObject::connect(&_roonApi, &QtRoonApi::error, this, &YioRoon::onError);
    _instance = this;
}
YioRoon::~YioRoon()
{
    _instance = nullptr;
    if (_items != nullptr) {
        delete [] _items;
    }
}

void YioRoon::setup (const QVariantMap& config, QObject *entities, QObject *notifications, QObject* api, QObject *configObj)
{
    Q_UNUSED(notifications)
    Q_UNUSED(api)
    Q_UNUSED(configObj)

    _log.setEnabled(QtMsgType::QtDebugMsg, false);     // Default, only debug disabled
    for (QVariantMap::const_iterator iter = config.begin(); iter != config.end(); ++iter) {
        if (iter.key() == "friendly_name")
            setFriendlyName(iter.value().toString());
        else if (iter.key() == "id")
            setIntegrationId(iter.value().toString());
        else if (iter.key() == "ip") {
            QString ip = iter.value().toString();
            if (!ip.contains(':'))
                ip += ":9100";
            _url = "ws://" + ip + "/api";
            _imageUrl = "http://" + ip + "/api/image/";
        }
        else if (iter.key() == "log") {
            const QString& severity = iter.value().toString();
            if (severity == "debug") {
                _log.setEnabled(QtMsgType::QtDebugMsg, true);
            }
            else if (severity == "info") {
                _log.setEnabled(QtMsgType::QtDebugMsg, false);
                _log.setEnabled(QtMsgType::QtInfoMsg, true);
            }
            else if (severity == "warning") {
                _log.setEnabled(QtMsgType::QtDebugMsg, false);
                _log.setEnabled(QtMsgType::QtInfoMsg, false);
                _log.setEnabled(QtMsgType::QtWarningMsg, true);
            }
        }
    }
    ConfigInterface* configInterface = qobject_cast<ConfigInterface *>(configObj);
    QVariant appPath = configInterface->getContextProperty ("configPath");
    _entities = qobject_cast<EntitiesInterface *>(entities);
    _notifications = qobject_cast<NotificationsInterface *> (notifications);
    _roonApi.setup(_url, appPath.toString());
}

void YioRoon::connect()
{
#if PERF_TEST
    if (true) {        // TEST
        EntityInterface* entity = _entities->getEntityInterface("media_player.work");
        QString friendly = entity->friendly_name();
        MediaPlayerInterface* player = static_cast<MediaPlayerInterface*>(entity->getSpecificInterface());
        int index = entity->getAttrIndex("volume");
        QVariantMap lmap;
        lmap["volume"] = 55.0;
        entity->update(lmap);
        double vol = player->volume();
        entity->updateAttrByIndex(index, 66);
        vol = player->volume();
        entity->updateAttrByName("volume", 77);
        vol = player->volume();

        entity->updateAttrByName("mediaArtist", "Maxl");
        QString artist = player->mediaArtist();
    }
#endif
    if (_contexts.length() == 0) {
        QVariantList emptyList;
        QStringList emptyButtons;
        QList<QObject*> list = _entities->getByIntegration(integrationId());
        for (int i = 0; i < list.length(); i++) {
            Entity* entity = static_cast<Entity*>(list[i]);
            _contexts.append(YioContext(i, entity->entity_id(), entity->friendly_name()));
            QVariantMap     map;
            map["items"] = emptyList;
            map["playCommands"] = emptyButtons;
            QVariantMap     result;
            result["browseResult"] = map;
            _entities->update(entity->entity_id(), result);
        }
        _items = new QList<QtRoonBrowseApi::BrowseItem> [static_cast<size_t>(list.length())];
    }
    setState(CONNECTING);
    _roonApi.open();
}
void YioRoon::disconnect()
{
    setState(DISCONNECTED);
    _roonApi.close();
    _contexts.clear();
    delete [] _items;
}
void YioRoon::sendCommand(const QString& type, const QString& id, const QString& cmd, const QVariant& param)
{
/*
    "SOURCE", "APP_NAME",
    "VOLUME", "VOLUME_UP", "VOLUME_DOWN", "VOLUME_SET","MUTE","MUTE_SET",
    "MEDIA_TYPE", "MEDIA_TITLE", "MEDIA_ARTIST", "MEDIA_ALBUM", "MEDIA_DURATION", "MEDIA_POSITION", "MEDIA_IMAGE",
    "PLAY", "PAUSE", "STOP", "PREVIOUS", "NEXT", "SEEK", "SHUFFLE", "TURN_ON", "TURN_OFF"
*/
    if (_log.isDebugEnabled())
        qCDebug(_log) << "sendCommand " << type << " " << id << " " << cmd << " " << param.toString();

    int idx;
    for (idx = 0; idx < _contexts.length(); idx++) {
        if (_contexts[idx].entityId == id)
            break;
    }
    if (idx >= _contexts.length()) {
        qCWarning(_log) << "can't find id " << id;
        return;
    }
    YioContext& ctx = _contexts[idx];

    const QtRoonTransportApi::Zone& zone = _transportApi.getZone(ctx.zoneId);
    const QtRoonTransportApi::Output& output = zone.outputs[0];
    if (cmd == "VOLUME_SET") {
        _transportApi.changeVolume(output.output_id, QtRoonTransportApi::EValueMode::absolute, param.toInt());
    }
    else if (cmd == "VOLUME_UP") {
        _transportApi.changeVolume(output.output_id, QtRoonTransportApi::EValueMode::relative_step, 1);
    }
    else if (cmd == "VOLUME_DOWN") {
        _transportApi.changeVolume(output.output_id, QtRoonTransportApi::EValueMode::relative_step, -1);
    }
    else if (cmd == "MUTE_SET") {
        _transportApi.mute(output.output_id, param.toBool() ? QtRoonTransportApi::EMute::mute : QtRoonTransportApi::EMute::unmute);
    }
    else if (cmd == "PLAY") {
        _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::play);
    }
    else if (cmd == "PAUSE") {
        _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::pause);
    }
    else if (cmd == "STOP") {
        _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::stop);
    }
    else if (cmd == "PREVIOUS") {
        _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::previous);
    }
    else if (cmd == "NEXT") {
        _transportApi.control(ctx.zoneId, QtRoonTransportApi::EControl::next);
    }
    else if (cmd == "BROWSE") {
        QString cmd = param.toString();
        if (cmd == "TOP")
            browse(ctx, true);
        else if (cmd == "BACK")
            browseBack(ctx);
        else {
            const QList<QtRoonBrowseApi::BrowseItem>& items = _items[idx];
            for (int i = 0; i < items.length(); i++) {
                if (items[i].item_key == cmd) {
                    browse(ctx, items, i, false);
                    return;
                }
            }
            const Action* action = getActionYio(cmd);
            if (action == nullptr) {
                qCWarning(_log) << "Can't find Browse command " << cmd;
                return;
            }
            for (int i = 0; i < items.length(); i++) {
                if (items[i].hint == "action_list") {
                    const Action* actionList = getActionRoon(items[i].title);
                    if (actionList != nullptr) {
                       ctx.forcedAction = cmd;
                        browseAction (ctx, items[i].item_key);
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
            qCWarning(_log) << "Can't find action list" << action->roonName;
        }
    }
    else if (cmd.startsWith("play:")){
        QString pcmd = cmd.mid (5);
        const Action* action = getActionYio(pcmd);
        if (action != nullptr) {
            ctx.forcedAction = pcmd;
            playMedia(ctx, param.toString(), true);
        }
    }
    else if (cmd.startsWith("search:")){
        QString scmd = cmd.mid (7);
        search (ctx, scmd, param.toString());
    }
}
void YioRoon::browse(YioContext &ctx, bool fromTop)
{
    QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, fromTop, false, 0);
    ctx.browseMode = BROWSE;
   _browseApi.browse (opt, ctx);
}
void YioRoon::browse(YioContext &ctx, const QList<QtRoonBrowseApi::BrowseItem>& items, int itemIndex, bool action)
{
    if (itemIndex < items.count()) {
        QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, items[itemIndex].item_key, action ? -1 : 0);
        ctx.browseMode = BROWSE;
        _browseApi.browse (opt, ctx);
    }
}
void YioRoon::browseBack (YioContext &ctx)
{
    QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, 1, 0);
    ctx.browseMode = BROWSE;
    _browseApi.browse (opt, ctx);
}
void YioRoon::browseRefresh(YioContext &ctx)
{
    QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, false, true, 0);
    ctx.browseMode = BROWSE;
    _browseApi.browse (opt, ctx);
}
void YioRoon::search (YioContext &ctx, const QString& searchText, const QString& itemKey)
{
    QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, itemKey, 0);
    opt.input = searchText;
    ctx.browseMode = BROWSE;
    _browseApi.browse (opt, ctx);
}
void YioRoon::playMedia(YioContext &ctx, const QString& itemKey, bool setItemIndex)
{
    QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, itemKey);
    ctx.browseMode = PLAY;
   _browseApi.browse (opt, ctx);
   if (setItemIndex) {
       ctx.queueFrom = false;
       ctx.gotoNext = false;
       ctx.itemIndex = findItemIndex(_items[ctx.index], itemKey);
   }
}
void YioRoon::browseAction(YioContext &ctx, const QString& itemKey)
{
    QtRoonBrowseApi::BrowseOption opt (ctx.zoneId, itemKey);
    ctx.browseMode = ACTION;
    _browseApi.browse (opt, ctx);
}

void YioRoon::onZonesChanged()
{
    if (state() != CONNECTED)
        setState(CONNECTED);

    QMap<QString, QtRoonTransportApi::Zone>& zones = _transportApi.zones();

    for (QMap<QString, QtRoonTransportApi::Zone>::iterator i = zones.begin(); i != zones.end(); ++i) {
        const QtRoonTransportApi::Zone& zone = i.value();
        int idx;
        for (idx = 0; idx < _contexts.length(); idx++) {
            if (_contexts[idx].friendlyName == zone.display_name)
                break;
        }
        if (idx >= _contexts.length()) {
            if (_notFound.indexOf(zone.zone_id) < 0) {
                _notFound.append(zone.zone_id);
                qCWarning(_log) << "can't find zone " << zone.display_name;
            }
        }
        else {
           _contexts[idx].zoneId = zone.zone_id;
            updateZone(_contexts[idx], zone, false);
        }
    }
}

void YioRoon::onZoneSeekChanged(const QtRoonTransportApi::Zone& zone)
{
    int idx;
    for (idx = 0; idx < _contexts.length(); idx++) {
        if (_contexts[idx].zoneId == zone.zone_id)
            break;
    }
    if (idx >= _contexts.length()) {
        if (_notFound.indexOf(zone.zone_id) < 0) {
            _notFound.append(zone.zone_id);
            qCWarning(_log) << "can't find seek zone" << zone.display_name;
        }
    }
    else
        updateZone (_contexts[idx], zone, true);
}
void YioRoon::onError (const QString& error)
{
    _notifications->add(true, "Cannot connect ROON : " + error, tr("Reconnect"), "roon");
}

void YioRoon::OnPaired(const RoonCore& core)
{
    Q_UNUSED(core)
    _transportApi.subscribeZones();
}

void YioRoon::OnUnpaired(const RoonCore& core)
{
    Q_UNUSED(core)
}

void YioRoon::transportCallback(int requestId, const QString& msg)
{
    Q_UNUSED(requestId)
    Q_UNUSED(msg)
}

void YioRoon::OnBrowse (const QString& err, QtRoonBrowseApi::Context& context, const QtRoonBrowseApi::BrowseResult& result)
{
    Q_UNUSED(err)

    YioContext& ctx = static_cast<YioContext&>(context);
    QtRoonBrowseApi::LoadOption opt;

    if (!err.isEmpty()) {
        updateError(ctx, err);
        return;
    }

    if (result.action == "list" && result.list != nullptr) {
        // Adjust path
        while (result.list->level < ctx.path.length())
            ctx.path.removeLast();
        ctx.path.append(result.list->title);
        if (_log.isDebugEnabled())
            qCDebug(_log) << "PATH: " << ctx.path.join('/');

        int listoffset = result.list->display_offset > 0 ? result.list->display_offset : 0;
        switch (ctx.browseMode) {
            case BROWSE:
            case ACTION:
                opt.offset = listoffset;
                opt.count = _numItems;
                opt.set_display_offset = listoffset;
                _browseApi.load (opt, ctx);
                break;
            case PLAY:
                opt.offset = 0;
                opt.count = 5;      // Max number of actions
               _browseApi.load(opt, ctx);
                break;
        }
    }
}

void YioRoon::OnLoad   (const QString& err, QtRoonBrowseApi::Context& context, const QtRoonBrowseApi::LoadResult& result)
{
    Q_UNUSED(err)

    const Action* action;
    YioContext& ctx = static_cast<YioContext&>(context);

    switch (ctx.browseMode) {
        case BROWSE:
            updateItems(static_cast<YioContext&>(context), result);
            break;
        case PLAY:
            if (result.items.length() > 0) {
                action = getActionYio(ctx.forcedAction);
                if (action != nullptr && !ctx.gotoNext) {
                    QString roonName = ctx.queueFrom ? "Queue" : action->roonName;
                    for (int j = 0; j < result.items.length(); j++) {
                        if (result.items[j].title == roonName) {
                            playMedia (ctx, result.items[j].item_key);
                            if (!!(action->action & ACT_PLAYFROM) && ctx.itemIndex < _items[ctx.index].length() - 1) {
                                ctx.gotoNext = true;
                                ctx.queueFrom = true;
                                return;     // Dont clear forced
                            }
                            ctx.forcedAction = "";
                            return;
                        }
                    }
                    // Not found
                    const Action* playAction = getActionRoon(result.items[0].title);
                    if (playAction != nullptr) {
                        playMedia (ctx, result.items[0].item_key);
                        ctx.goBack = 1;      // requires go back
                        return;
                    }
                }
                // Handle play from
                if (ctx.gotoNext && action != nullptr && !!(action->action & ACT_PLAYFROM)) {
                    QList<QtRoonBrowseApi::BrowseItem>& items = _items[ctx.index];
                    ctx.itemIndex++;
                    ctx.gotoNext = false;
                    if (ctx.itemIndex < items.length())
                        playMedia (ctx, items[ctx.itemIndex].item_key);
                    else
                        ctx.forcedAction = "";
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
    }
}


void YioRoon::updateZone (YioContext& ctx, const QtRoonTransportApi::Zone& zone, bool seekChanged) {
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

    EntityInterface* entity = static_cast<EntityInterface*>(_entities->getEntityInterface(ctx.entityId));
    if (!seekChanged) {
        MediaPlayerDef::States state;
        switch (zone.state) {
            case QtRoonTransportApi::EState::unknown:
            case QtRoonTransportApi::EState::stopped:   state = MediaPlayerDef::OFF;       break;
            case QtRoonTransportApi::EState::playing:   state = MediaPlayerDef::PLAYING;   break;
            case QtRoonTransportApi::EState::paused:    state = MediaPlayerDef::ON;        break;
            case QtRoonTransportApi::EState::loading:   state = MediaPlayerDef::IDLE;      break;
        }
        entity->updateAttrByIndex(MediaPlayerDef::STATE, static_cast<int>(state));
        if (zone.outputs.length() > 0 && zone.outputs[0].volume != nullptr) {
            const QtRoonTransportApi::Volume* volume = zone.outputs[0].volume;
            entity->updateAttrByIndex(MediaPlayerDef::VOLUME, volume->value);
            entity->updateAttrByIndex(MediaPlayerDef::MUTED, volume->is_muted);
        }
        if (zone.now_playing != nullptr) {
            const QtRoonTransportApi::NowPlaying* np = zone.now_playing;
            if (!np->image_key.isEmpty())
                entity->updateAttrByIndex(MediaPlayerDef::MEDIAIMAGE, _imageUrl + np->image_key + "?scale=fit&width=256&height=256");
            else
                entity->updateAttrByIndex(MediaPlayerDef::MEDIAIMAGE, "");
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAARTIST, np->two_line->line2);
            entity->updateAttrByIndex(MediaPlayerDef::MEDIATITLE, np->two_line->line1);
        }
        else {
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAIMAGE, "");
            entity->updateAttrByIndex(MediaPlayerDef::MEDIAARTIST, "");
            entity->updateAttrByIndex(MediaPlayerDef::MEDIATITLE, "");
        }
        entity->updateAttrByIndex(MediaPlayerDef::SOURCE, "");
        entity->updateAttrByIndex(MediaPlayerDef::MEDIATYPE, "");
    }
}
void YioRoon::updateItems (YioContext& ctx, const QtRoonBrowseApi::LoadResult& result) {
    QVariantList    list;
    QStringList     playCommands;
    int             level = 0;

    int idx = ctx.index;

    QList<QtRoonBrowseApi::BrowseItem>& items = _items[idx];
    items.clear();
    QString     browseType;

    if (_cmdsForItem && result.list != nullptr) {
        const Action * action = getActionParentTitle(result.list->title);
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
                    if (action->action & ACT_REJECT)
                        continue;
                    if ((action->action & ACT_ACTIONLIST)) {
                        QStringList btns = getForcedActions(action->forcedChildActions);
                        playCommands.append(btns);
                        browseType = action->type;
                        continue;
                    }
                }
            }
        }
        else {
            if (item.hint == "action_list" || level == 0) {
                const Action* action = getActionRoon(item.title);
                if (action != nullptr) {
                    if (action->action & ACT_REJECT)
                        continue;
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
        QVariantMap entityItem;
        entityItem["item_key"] = item.item_key;
        entityItem["title"] = item.title;
        entityItem["sub_title"] = item.subtitle;
        if (item.input_prompt != nullptr)
            entityItem["input_prompt"] = item.input_prompt->prompt;
        if (!item.image_key.isEmpty())
            entityItem["image_url"] = _imageUrl + item.image_key + "?scale=fit&width=64&height=64";
        else
            entityItem["image_url"] = "";
        list.append(entityItem);
    }

    QVariantMap     map;
    map["items"] = list;
    map["playCommands"] = playCommands;
    map["type"] = browseType;
    if (result.list != nullptr) {
        map["title"] = result.list->title;
        map["level"] = result.list->level;
    }
    else {
        map["title"] = "";
        map["level"] = -1;
    }
    EntityInterface* entity = static_cast<EntityInterface*>(_entities->getEntityInterface(ctx.entityId));
    entity->updateAttrByIndex(MediaPlayerDef::BROWSERESULT, map);
    /*
    QVariantMap     resultMap;
    resultMap["browseResult"] = map;
    _entities->update(ctx.entityId, resultMap);
    */
}
void YioRoon::updateError (YioContext& ctx, const QString& error)
{
    Q_UNUSED(ctx)
    _notifications->add(true, "ROON browsing error : " + error);
}

const YioRoon::Action* YioRoon::getActionRoon (const QString& roonName) {
    if (roonName == "")
        return nullptr;
    for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
        if (iter->roonName == roonName)
            return &*iter;
    }
    return nullptr;
}
const YioRoon::Action* YioRoon::getActionYio (const QString& yioName) {
    if (yioName == "")
        return nullptr;
    for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
        if (iter->yioName == yioName)
            return &*iter;
    }
    return nullptr;
}
const YioRoon::Action* YioRoon::getActionParentTitle(const QString& parentTitle) {
    if (parentTitle == "")
        return nullptr;
    for (QList<Action>::const_iterator iter = _actions.begin(); iter != _actions.end(); ++iter) {
        if (iter->parentTitle == parentTitle)
            return &*iter;
    }
    return nullptr;
}
QStringList YioRoon::getForcedActions (EAction forcedActions) {
    EAction actions[] = { ACT_PLAYNOW, ACT_PLAYFROM, ACT_SHUFFLE, ACT_ADDNEXT, ACT_QUEUE, ACT_STARTRADIO };
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
int YioRoon::findItemIndex (const QList<QtRoonBrowseApi::BrowseItem>& items, const QString& itemKey)
{
    for (int i = 0; i < items.length(); i++)
        if (items[i].item_key == itemKey)
            return i;
    return -1;
}

