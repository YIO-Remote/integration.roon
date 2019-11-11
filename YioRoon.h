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
    explicit	YioRoon(QObject* parent = nullptr);
                ~YioRoon() override;

    Q_INVOKABLE void setup  	    (const QVariantMap& config, QObject *entities, QObject *notifications, QObject* api, QObject *configObj);
    Q_INVOKABLE void connect	    ();
    Q_INVOKABLE void disconnect	    ();

    static QLoggingCategory& Log    () { return _log; }

public slots:
    void        sendCommand         (const QString& type, const QString& entity_id, const QString& command, const QVariant& param);
    void        stateHandler        (int state);

    void		browse              (const QString& zoneId, bool fromTop);
    void		browse              (const QString& zoneId, const QList<QtRoonBrowseApi::BrowseItem>& items, int itemIndex, bool action);
    void		browseBack          (const QString& zoneId);
    void		browseRefresh       (const QString& zoneId);

private slots:
    void        onZonesChanged      ();
    void        onZoneSeekChanged   (const QtRoonTransportApi::Zone& zone);

private:
    RoonRegister					_reg;
    QtRoonApi						_roonApi;
    QtRoonBrowseApi					_browseApi;
    QtRoonTransportApi				_transportApi;
    int								_numItems;
    int								_requestId;
    QString                         _url;
    QString                         _imageUrl;
    EntitiesInterface*              _entities;
    QString                         _lastBrowseId;
    QStringList                     _friendlyNames;
    QStringList                     _entityIds;
    QStringList                     _zoneIds;
    QStringList                     _specialActions;
    QList<QtRoonBrowseApi::BrowseItem>* _items;

    static YioRoon*                 _instance;
    static QLoggingCategory         _log;

	static void transportCallback	(int requestId, const QString& msg);
	static void loadCallback		(int requestId, const QString& err, const QtRoonBrowseApi::LoadResult& result);
	static void browseCallback		(int requestId, const QString& err, const QtRoonBrowseApi::BrowseResult& result);

	virtual void OnPaired			(const RoonCore& core) override;
	virtual void OnUnpaired			(const RoonCore& core) override;

    void updateItems (const QtRoonBrowseApi::LoadResult& result);
    void updateZone (const QString& id, const QtRoonTransportApi::Zone& zone, bool seekChanged);
};
