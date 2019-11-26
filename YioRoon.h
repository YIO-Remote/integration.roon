#define USE_MODEL 1
#pragma once
#include <QtCore/QObject>
#include "QtRoonApi.h"
#include "QtRoonTransportApi.h" 
#include "QtRoonBrowseApi.h" 
#include "QtRoonDiscovery.h"
#include "../remote-software/sources/integrations/integration.h"
#include "../remote-software/sources/integrations/integrationinterface.h"
#include "../remote-software/sources/entities/entitiesinterface.h"
#include "../remote-software/sources/notificationsinterface.h"

#if USE_MODEL
#include "BrowseModel.h"
#endif

class Roon : public IntegrationInterface
{
    Q_OBJECT
    //Q_DISABLE_COPY(Roon)
    Q_PLUGIN_METADATA(IID "YIO.IntegrationInterface" FILE "roon.json")
    Q_INTERFACES(IntegrationInterface)

public:
    explicit Roon(QObject* parent = nullptr);
    virtual ~Roon() override {
    }

    void        create              (const QVariantMap& config, QObject *entities, QObject *notifications, QObject* api, QObject *configObj) override;

public slots:
    void        onRoonDiscovered    (QMap<QString, QVariantMap>);

private:
    QLoggingCategory    _log;
    QtRoonDiscovery     _discovery;
};


class YioRoon : public Integration, IRoonPaired, QtRoonBrowseApi::ICallback
{
    Q_OBJECT
public:
    explicit	YioRoon             (QObject* parent = nullptr);
    virtual     ~YioRoon            () override;

    Q_INVOKABLE void setup  	    (const QVariantMap& config, QObject *entities, QObject *notifications, QObject* api, QObject *configObj);
    Q_INVOKABLE void connect	    ();
    Q_INVOKABLE void disconnect	    ();

    static QLoggingCategory& Log    () { return _log; }


public slots:
    void        sendCommand         (const QString& type, const QString& entity_id, const QString& command, const QVariant& param);
    void        onZonesChanged      ();
    void        onZoneSeekChanged   (const QtRoonTransportApi::Zone& zone);
    void        onError             (const QString& error);

private:
    enum EAction  {
        ACT_NONE        = 0,
        ACT_PLAYNOW     = 0x0001,
        ACT_PLAYFROM    = 0x0002,
        ACT_SHUFFLE     = 0x0004,
        ACT_ADDNEXT     = 0x0008,
        ACT_QUEUE       = 0x0010,
        ACT_STARTRADIO  = 0x0020,
        ACT_REJECT      = 0x0100,
        ACT_ACTIONLIST  = 0x0200
    };
    class Action {
    public:
         Action (const QString& roonName, const QString& yioName, const QString& type, const QString& parentTitle, EAction action, EAction forcedActions, EAction forcedChildActions) :
             roonName(roonName),
             yioName(yioName),
             type(type),
             parentTitle(parentTitle),
             action(action),
             forcedActions(forcedActions),
             forcedChildActions(forcedChildActions)
         {}
         QString    roonName;
         QString    yioName;
         QString    type;
         QString    parentTitle;
         EAction    action;
         EAction    forcedActions;
         EAction    forcedChildActions;
    };
    enum BrowseMode {
        BROWSE,
        PLAY,
        ACTION
    };

    struct YioContext : QtRoonBrowseApi::Context {
        YioContext() :
            goBack(0)
        {}
        YioContext (int index, const QString& entityId, const QString& friendlyName) :
            index(index),
            itemIndex(-1),
            queueFrom(false),
            gotoNext(false),
            entityId(entityId),
            friendlyName(friendlyName),
            goBack(0)
        {}
        int         index;
        int         itemIndex;
        bool        queueFrom;
        bool        gotoNext;
        BrowseMode  browseMode;
        QString     entityId;
        QString     friendlyName;
        QString     forcedAction;
        QStringList path;
        int         goBack;
    };

    RoonRegister                        _reg;
    QtRoonApi				_roonApi;
    QtRoonBrowseApi			_browseApi;
    QtRoonTransportApi			_transportApi;
    int					_numItems;
    bool                                _playFromThere;
    QString                             _url;
    QString                             _imageUrl;
    EntitiesInterface*                  _entities;
    NotificationsInterface*             _notifications;
    QList<Action>                       _actions;
    QList<QtRoonBrowseApi::BrowseItem>* _items;
    QList<YioContext>                   _contexts;
    QStringList                         _notFound;
    bool                                _cmdsForItem;
#if USE_MODEL
    BrowseModel                         _model;
#endif
    static YioRoon*                     _instance;
    static QLoggingCategory             _log;

    static void     transportCallback       (int requestId, const QString& msg);
    void            browse                  (YioContext& ctx, bool fromTop);
    void            browse                  (YioContext& ctx, const QList<QtRoonBrowseApi::BrowseItem>& items, int itemIndex, bool action);
    void            browseAction            (YioContext& ctx, const QString& item_key);
    void            browseBack              (YioContext& ctx);
    void            browseRefresh           (YioContext& ctx);
    void            playMedia               (YioContext& ctx, const QString& itemKey, bool setItemIndex = false);
    void            search                  (YioContext& ctx, const QString& searchText, const QString& itemKey);

    virtual void    OnPaired                (const RoonCore& core) override;
    virtual void    OnUnpaired              (const RoonCore& core) override;
    virtual void    OnBrowse                (const QString& err, QtRoonBrowseApi::Context& context, const QtRoonBrowseApi::BrowseResult& content) override;
    virtual void    OnLoad                  (const QString& err, QtRoonBrowseApi::Context& context, const QtRoonBrowseApi::LoadResult& content) override;

    void            updateItems             (YioContext& ctx, const QtRoonBrowseApi::LoadResult& result);
    void            updateZone              (YioContext& ctx, const QtRoonTransportApi::Zone& zone, bool seekChanged);
    void            updateError             (YioContext& ctx, const QString& error);
    QStringList     getForcedActions        (EAction forcedActions);
    const Action*   getActionRoon           (const QString& roonName);
    const Action*   getActionYio            (const QString& yioName);
    const Action*   getActionParentTitle    (const QString& parentTitle);
    int             findItemIndex           (const QList<QtRoonBrowseApi::BrowseItem>& items, const QString& itemKey);
};
