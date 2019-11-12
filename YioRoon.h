#pragma once
#include <QtCore/QObject>
#include "QtRoonApi.h"
#include "QtRoonTransportApi.h" 
#include "QtRoonBrowseApi.h" 
#include "../remote-software/sources/integrations/integration.h"
#include "../remote-software/sources/integrations/integrationinterface.h"

class Roon : public IntegrationInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "YIO.IntegrationInterface" FILE "roon.json")
    Q_INTERFACES(IntegrationInterface)

public:
    explicit Roon() {}

    void create (const QVariantMap& config, QObject *entities, QObject *notifications, QObject* api, QObject *configObj) override;
};


class YioRoon : public Integration, IRoonPaired
{
    Q_OBJECT
public:
    explicit	YioRoon             (QObject* parent = nullptr);
                ~YioRoon            () override;

    Q_INVOKABLE void setup  	    (const QVariantMap& config, QObject *entities, QObject *notifications, QObject* api, QObject *configObj);
    Q_INVOKABLE void connect	    ();
    Q_INVOKABLE void disconnect	    ();

    static QLoggingCategory& Log    () { return _log; }

public slots:
    void        sendCommand         (const QString& type, const QString& entity_id, const QString& command, const QVariant& param);
    void        stateHandler        (int state);

private slots:
    void        onZonesChanged      ();
    void        onZoneSeekChanged   (const QtRoonTransportApi::Zone& zone);

private:
    enum EAction  {
        ACT_NONE        = 0,
        ACT_PLAYNOW     = 0x0001,
        ACT_SHUFFLE     = 0x0002,
        ACT_ADDNEXT     = 0x0004,
        ACT_QUEUE       = 0x0008,
        ACT_STARTRADIO  = 0x0010,
        ACT_REJECT      = 0x0100,
        ACT_ACTIONLIST  = 0x0200
    };
    class Action {
    public:
         Action (const QString& roonName, const QString& yioName, EAction action, EAction forcedActions) :
             roonName(roonName),
             yioName(yioName),
             action(action),
             forcedActions(forcedActions)
         {}
         QString    roonName;
         QString    yioName;
         EAction    action;
         EAction    forcedActions;
    };

    RoonRegister                        _reg;
    QtRoonApi				_roonApi;
    QtRoonBrowseApi			_browseApi;
    QtRoonTransportApi			_transportApi;
    int					_numItems;
    int					_requestId;
    QString                             _url;
    QString                             _imageUrl;
    EntitiesInterface*                  _entities;
    QString                             _lastBrowseId;
    QStringList                         _friendlyNames;
    QStringList                         _entityIds;
    QStringList                         _zoneIds;
    QStringList                         _forcedActions;
    QList<Action>                       _actions;
    QList<QtRoonBrowseApi::BrowseItem>* _items;

    static YioRoon*                     _instance;
    static QLoggingCategory             _log;

    static void transportCallback	(int requestId, const QString& msg);
    static void loadCallback		(int requestId, const QString& err, const QtRoonBrowseApi::LoadResult& result);
    static void browseCallback		(int requestId, const QString& err, const QtRoonBrowseApi::BrowseResult& result);
    static void actionLoadCallback	(int requestId, const QString& err, const QtRoonBrowseApi::LoadResult& result);
    static void actionBrowseCallback    (int requestId, const QString& err, const QtRoonBrowseApi::BrowseResult& result);

    void	browse                  (const QString& zoneId, bool fromTop);
    void	browse                  (const QString& zoneId, const QList<QtRoonBrowseApi::BrowseItem>& items, int itemIndex, bool action);
    void	browseAction            (const QString& zoneId, const QString& item_key);
    void	browseBack              (const QString& zoneId);
    void	browseRefresh           (const QString& zoneId);

    virtual void OnPaired		(const RoonCore& core) override;
    virtual void OnUnpaired		(const RoonCore& core) override;

    void        updateItems             (const QtRoonBrowseApi::LoadResult& result);
    void        updateZone              (const QString& id, const QtRoonTransportApi::Zone& zone, bool seekChanged);
    const Action* getActionRoon         (const QString& roonName);
    const Action* getActionYio          (const QString& yioName);
    QStringList getForcedActions        (EAction forcedActions);
};
