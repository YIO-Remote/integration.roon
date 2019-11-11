#include <QJsonDocument>
#include <QMetaEnum>
#include "QtRoonTransportApi.h" 

QtRoonTransportApi::SourceControl::SourceControl(const QVariantMap& map)
{
	control_key = map["control_key"].toString();
	display_name = map["display_name"].toString();
	status = toESourceControlStatus(map["status"].toString());
	supports_standby = map["supports_standby"].toBool();
}
QtRoonTransportApi::SourceControl::SourceControl(const SourceControl& other) :
	control_key(other.control_key),
	display_name(other.display_name),
	status(other.status),
	supports_standby(other.supports_standby)
{}

QtRoonTransportApi::Volume::Volume(const QVariantMap& map)
{
	type = map["type"].toString();
	min = map["min"].toInt();
	max = map["max"].toInt();
	value = map["value"].toInt();
	step = map["step"].toInt();
	is_muted = map["is_muted"].toBool();
}
QtRoonTransportApi::Volume::Volume(const Volume& other) :
	type(other.type),
	min(other.min),
	max(other.max),
	value(other.value),
	step(other.step),
	is_muted(other.is_muted)
{}

QtRoonTransportApi::Output::Output(const QVariantMap& map)
{
    volume = nullptr;
	output_id = map["output_id"].toString();
	zone_id = map["zone_id"].toString();
	display_name = map["display_name"].toString();
	can_group_with_output_ids = map["can_group_with_output_ids"].toStringList();
	state = toEState(map["state"].toString());
	QVariantList list = map["source_controls"].toList();
	for (int i = 0; i < list.count(); ++i)
	{
		source_controls.append (SourceControl(list[i].toMap()));
	}
	QVariantMap volmap = map["volume"].toMap();
	if (!volmap.isEmpty())
		volume = new Volume(volmap);
}
QtRoonTransportApi::Output::Output(const Output& other) :
	output_id(other.output_id),
	zone_id(other.zone_id),
	display_name(other.display_name),
	can_group_with_output_ids(other.can_group_with_output_ids),
	state(other.state),
	source_controls(other.source_controls),
    volume(other.volume == nullptr ? nullptr : new Volume(*other.volume))
{}
QtRoonTransportApi::Output& QtRoonTransportApi::Output::operator= (const QtRoonTransportApi::Output& other) {
    output_id = other.output_id;
    zone_id= other.zone_id;
    display_name = other.display_name;
    can_group_with_output_ids =other.can_group_with_output_ids;
    state = other.state;
    source_controls = other.source_controls;
    volume = other.volume == nullptr ? nullptr : new Volume(*other.volume);
    return *this;
}

QtRoonTransportApi::Output::Output() {
    volume = nullptr;
}
QtRoonTransportApi::Output::~Output() {
    if (volume != nullptr) delete volume;
    volume = nullptr;
}

QtRoonTransportApi::ZoneSettings::ZoneSettings(const QVariantMap& map) {
	auto_radio = map["auto_radio"].toBool();
	shuffle = map["shuffle"].toBool();
	loop = toELoop(map["loop"].toString());
}
QtRoonTransportApi::ZoneSettings::ZoneSettings(const ZoneSettings& src) :
	auto_radio(src.auto_radio),
	loop(src.loop),
	shuffle(src.shuffle)
{}

QtRoonTransportApi::Lines::Lines(const QVariantMap& map)
{
	line1 = map["line1"].toString();
	line2 = map["line2"].toString();
	line3 = map["line3"].toString();
}
QtRoonTransportApi::Lines::Lines(const Lines& other) :
	line1(other.line1),
	line2(other.line2),
	line3(other.line3)
{}

QtRoonTransportApi::NowPlaying::NowPlaying(const QVariantMap& map) {
    one_line = two_line = three_line = nullptr;
	seek_position = map["seek_position"].toInt();
	length = map["length"].toInt();
	image_key = map["image_key"].toString();
	if (map.contains("one_line"))
		one_line = new Lines(map["one_line"].toMap());
	if (map.contains("two_line"))
		two_line = new Lines(map["two_line"].toMap());
    if (map.contains("three_line"))
		three_line = new Lines(map["three_line"].toMap());
}
QtRoonTransportApi::NowPlaying::NowPlaying(const NowPlaying& other) :
	seek_position(other.seek_position),
    length(other.length),
    image_key(other.image_key),
    one_line(other.one_line == nullptr ? nullptr : new Lines(*other.one_line)),
    two_line(other.two_line == nullptr ? nullptr : new Lines(*other.two_line)),
    three_line(other.three_line == nullptr ? nullptr : new Lines(*other.three_line))
{}
QtRoonTransportApi::NowPlaying& QtRoonTransportApi::NowPlaying::operator= (const QtRoonTransportApi::NowPlaying& other) {
    seek_position = other.seek_position;
    length = other.length;
    image_key = other.image_key;
    one_line = other.one_line == nullptr ? nullptr : new Lines(*other.one_line);
    two_line = other.two_line == nullptr ? nullptr : new Lines(*other.two_line);
    three_line= other.three_line == nullptr ? nullptr : new Lines(*other.three_line);
    return *this;
}

QtRoonTransportApi::NowPlaying::NowPlaying()
{ 
    one_line = two_line = three_line = nullptr;
}
QtRoonTransportApi::NowPlaying::~NowPlaying() {
    if (one_line != nullptr) delete one_line;
    if (two_line != nullptr) delete two_line;
    if (three_line != nullptr) delete three_line;
    one_line = two_line = three_line = nullptr;
}

QtRoonTransportApi::Zone::Zone(const QVariantMap& map)
{
    settings = nullptr; now_playing = nullptr;
	zone_id = map["zone_id"].toString();
	state = toEState(map["state"].toString());
	display_name = map["display_name"].toString();
	is_next_allowed = map["is_next_allowed"].toBool();
	is_pause_allowed = map["is_pause_allowed"].toBool();
	is_play_allowed = map["is_play_allowed"].toBool();
	is_previous_allowed = map["is_previous_allowed"].toBool();
	is_seek_allowed = map["is_seek_allowed"].toBool();
	seek_position = map["seek_position"].toInt();
    QVariantList list = map["outputs"].toList();
	for (int i = 0; i < list.count(); ++i)
	{
		outputs.append(Output(list[i].toMap()));
	}
	queue_items_remaining = map["queue_items_remaining"].toInt();
	queue_time_remaining = map["queue_time_remaining"].toInt();
	if (map.contains("settings"))
		settings = new ZoneSettings(map["settings"].toMap());
	if (map.contains("now_playing"))
		now_playing = new NowPlaying(map["now_playing"].toMap());
}
QtRoonTransportApi::Zone::Zone(const Zone& other) :
	zone_id(other.zone_id),
    state(other.state),
	display_name(other.display_name),
	is_next_allowed(other.is_next_allowed),
	is_pause_allowed(other.is_pause_allowed),
	is_play_allowed(other.is_play_allowed),
	is_previous_allowed(other.is_previous_allowed),
	is_seek_allowed(other.is_seek_allowed),
    seek_position(other.seek_position),
    outputs(other.outputs),
    queue_items_remaining(other.queue_items_remaining),
	queue_time_remaining(other.queue_items_remaining),
    settings(other.settings == nullptr ? nullptr : new ZoneSettings(*other.settings)),
    now_playing(other.now_playing == nullptr ? nullptr : new NowPlaying(*other.now_playing))
{}
QtRoonTransportApi::Zone& QtRoonTransportApi::Zone::operator= (const QtRoonTransportApi::Zone& other) {
    zone_id = other.zone_id;
    state  = other.state;
    display_name = other.display_name;
    is_next_allowed = other.is_next_allowed;
    is_pause_allowed = other.is_pause_allowed;
    is_play_allowed = other.is_play_allowed;
    is_previous_allowed  = other.is_previous_allowed;
    is_seek_allowed = other.is_seek_allowed;
    seek_position = other.seek_position;
    outputs = other.outputs;
    queue_items_remaining = other.queue_items_remaining;
    queue_time_remaining = other.queue_items_remaining;
    settings = other.settings == nullptr ? nullptr : new ZoneSettings(*other.settings);
    now_playing = other.now_playing == nullptr ? nullptr : new NowPlaying(*other.now_playing);    return *this;
}

QtRoonTransportApi::Zone::Zone() {
    settings = nullptr; now_playing = nullptr;
}
QtRoonTransportApi::Zone::~Zone() {
    if (settings != nullptr) delete settings;
    if (now_playing != nullptr) delete now_playing;
    settings = nullptr; now_playing = nullptr;
}

QtRoonTransportApi::ZoneChanged::ZoneChanged(const QVariantMap& map)
{
	zone_id = map["zone_id"].toString();
	seek_position = map["seek_position"].toInt();
	queue_time_remaining = map["queue_time_remaining"].toInt();
}
QtRoonTransportApi::ZoneChanged::ZoneChanged(const ZoneChanged& other) :
	zone_id(other.zone_id),
	seek_position(other.seek_position),
	queue_time_remaining(other.queue_time_remaining)
{}

QtRoonTransportApi::TransportMessage::TransportMessage(const QVariantMap& map)
{
    zones = nullptr; zones_removed = nullptr; zones_added = nullptr; zones_changed = nullptr;
    zones_seek_changed = nullptr;
	QVariantList list;
	if (map.contains("zones")) {
		zones = new QList<Zone>();
		list = map["zones"].toList();
		for (int i = 0; i < list.count(); i++)
			zones->append(Zone(list[i].toMap()));
	}
	if (map.contains("zones_removed")) {
		zones_removed = new QList<Zone>();
		list = map["zones_removed"].toList();
		for (int i = 0; i < list.count(); i++)
			zones_removed->append(Zone(list[i].toMap()));
	}
	if (map.contains("zones_added")) {
		zones_added = new QList<Zone>();
		list = map["zones_added"].toList();
		for (int i = 0; i < list.count(); i++)
			zones_added->append(Zone(list[i].toMap()));
	}
	if (map.contains("zones_changed")) {
		zones_changed = new QList<Zone>();
		list = map["zones_changed"].toList();
		for (int i = 0; i < list.count(); i++)
			zones_changed->append(Zone(list[i].toMap()));
	}
	if (map.contains("zones_seek_changed")) {
		zones_seek_changed = new QList<ZoneChanged>();
		list = map["zones_seek_changed"].toList();
		for (int i = 0; i < list.count(); i++)
			zones_seek_changed->append(ZoneChanged(list[i].toMap()));
	}
}
QtRoonTransportApi::TransportMessage::TransportMessage() {
    zones = zones_removed = zones_added = zones_changed = nullptr; zones_seek_changed = nullptr;
}
QtRoonTransportApi::TransportMessage::~TransportMessage() {
    if (zones != nullptr) delete zones;
    if (zones_removed != nullptr) delete zones_removed;
    if (zones_added != nullptr) delete zones_added;
    if (zones_changed != nullptr) delete zones_changed;
}

QtRoonTransportApi::QtRoonTransportApi(QtRoonApi& roonApi, TCallback callback, QObject* parent) :
	QObject(parent),
	_roonApi(roonApi),
	_permanentCallback(callback),
    _callback(nullptr)
{
}
void QtRoonTransportApi::OnReceived(const ReceivedContent& content)
{
    if (_roonApi.Log().isDebugEnabled())
        qCDebug(_roonApi.Log()) << "RoonTransportApi.OnReceived : " << content._messageType << " " << content._requestId << " " << content._service << content._command;
    QJsonDocument document = QJsonDocument::fromJson(content._body.toUtf8());
	QVariantMap map = document.toVariant().toMap();
	bool zChg = false;
	if (content._command == QtRoonApi::Subscribed) {
		TransportMessage msg(map);
        if (msg.zones != nullptr) {
			for (int i = 0; i < msg.zones->count(); i++) {
                const Zone& zone = msg.zones->at(i);
				_zones.insert(zone.zone_id, zone);
				zChg = true;
			}
		}
		if (zChg)
			emit zonesChanged();
	}
	else if (content._command == QtRoonApi::Changed) {
		TransportMessage msg(map);
        if (msg.zones_added != nullptr) {
			for (int i = 0; i < msg.zones_added->count(); i++) {
                const Zone& zone = msg.zones_added->at(i);
				_zones.insert(zone.zone_id, zone);
				zChg = true;
			}
		}
        if (msg.zones_removed != nullptr) {
			for (int i = 0; i < msg.zones_removed->count(); i++) {
				_zones.remove(msg.zones_removed->at(i).zone_id);
				zChg = true;
			}
		}
        if (msg.zones_changed != nullptr) {
			for (int i = 0; i < msg.zones_changed->count(); i++) {
                const Zone& zone = msg.zones_changed->at(i);
				_zones.insert(zone.zone_id, zone);
				zChg = true;
			}
		}
        if (msg.zones_seek_changed != nullptr) {
			for (int i = 0; i < msg.zones_seek_changed->count(); i++) {
				const ZoneChanged& changed = msg.zones_seek_changed->at(i);
                for (QMap<QString, Zone>::iterator x = _zones.begin(); x != _zones.end(); ++x) {
					if (x.key() == changed.zone_id) {
                        Zone& zone = x.value();
                        zone.seek_position = changed.seek_position;
                        zone.queue_time_remaining = changed.queue_time_remaining;
                        emit zoneSeekChanged(zone);
                        break;
					}
				}
                /*
                 * requires assignment operator, I think
				Zone& zone = _zones[changed.zone_id];
				zone.seek_position = changed.seek_position;
				zone.queue_time_remaining = changed.queue_time_remaining;
				emit zoneSeekChanged(zone);
                */
			}
		}
		if (zChg)
			emit zonesChanged();
	}
	else {
        if (_callback != nullptr) {
			_callback(content._requestId, content._command.isEmpty() ? "Error" : content._command);
            _callback = nullptr;
		}
        if (_permanentCallback != nullptr) {
			_permanentCallback(content._requestId, content._command.isEmpty() ? "Error" : content._command);
		}
	}
}

const QtRoonTransportApi::Zone& QtRoonTransportApi::getZone(const QString& zone_id)
{
	return _zones[zone_id];
}

QList<QtRoonTransportApi::Zone>* QtRoonTransportApi::getZones() {
	QList<Zone>* list = new QList<Zone>();
	for (auto e : _zones.keys())
	{
		const Zone& zone = _zones.value(e);
		list->append(zone);
	}
	return list;
}


void QtRoonTransportApi::subscribeZones()
{
    qCDebug(_roonApi.Log()) << "QtRoonTransportApi.subscribeZones";
	_roonApi.sendSubscription(QtRoonApi::ServiceTransport + "/subscribe_zones", this);
}

int QtRoonTransportApi::control(const QString& zone_or_output_id, EControl control, TCallback callback) {
	QVariantMap map;
	map["zone_or_output_id"] = zone_or_output_id;
	map["control"] = toString (control);
	return send (QtRoonApi::ServiceTransport + "/control", map, callback);
}
int	QtRoonTransportApi::changeSettings(const QString& zone_or_output_id, bool shuffle, bool autoRadio, ELoop loop, TCallback callback) {
	QVariantMap map;
	map["zone_or_output_id"] = zone_or_output_id;
	map["shuffle"] = shuffle;
	map["auto_radio"] = autoRadio;
	map["loop"] = toString(loop);
	return send(QtRoonApi::ServiceTransport + "/change_settings", map, callback);
}
int	QtRoonTransportApi::changeVolume(const QString& output_id, EValueMode mode, int value, TCallback callback)
{
	QVariantMap map;
	map["output_id"] = output_id;
	map["how"] = toString(mode);
	map["value"] = value;
	return send(QtRoonApi::ServiceTransport + "/change_volume", map, callback);
}
int	QtRoonTransportApi::mute(const QString& output_id, EMute mute, TCallback callback)
{
	QVariantMap map;
	map["output_id"] = output_id;
	map["how"] = toString(mute);
	return send(QtRoonApi::ServiceTransport + "/mute", map, callback);
}
int	QtRoonTransportApi::muteAll(EMute mute, TCallback callback)
{
	QVariantMap map;
	map["how"] = toString(mute);
	return send(QtRoonApi::ServiceTransport + "/mute_all", map, callback);
}
int	QtRoonTransportApi::pauseAll(TCallback callback)
{
	QVariantMap map;
	return send(QtRoonApi::ServiceTransport + "/pause_all", map, callback);
}
int	QtRoonTransportApi::seek(const QString& zone_or_output_id, EValueMode how, int second, TCallback callback)
{
	QVariantMap map;
	map["zone_or_output_id"] = zone_or_output_id;
	map["how"] = toString(how);
	map["second"] = second;
	return send(QtRoonApi::ServiceTransport + "/seek", map, callback);
}

void QtRoonTransportApi::OnPaired(const RoonCore& core)
{
    qCInfo(_roonApi.Log()) << "QtRoonTransportApi.OnPaired" << core.core_id;
	subscribeZones();
}
void QtRoonTransportApi::OnUnpaired(const RoonCore& core)
{
    qCInfo(_roonApi.Log()) << "QtRoonTransportApi.OnUnPaired" << core.core_id;
}
int	QtRoonTransportApi::send(const QString& path, const QVariantMap& map, TCallback callback)
{
	QJsonDocument doc = QJsonDocument::fromVariant(map);
	QString json = doc.toJson(QJsonDocument::JsonFormat::Compact);
	_callback = callback;
	return _roonApi.send(path, this, &json);
}

QString QtRoonTransportApi::toString(EControl state)
{
	QMetaEnum metaEnum = getMetaEnum("EControl");
	return metaEnum.valueToKey(static_cast<int>(state));
}
QString QtRoonTransportApi::toString(ELoop loop)
{
	QMetaEnum metaEnum = getMetaEnum("ELoop");
	return metaEnum.valueToKey(static_cast<int>(loop));
}
QString QtRoonTransportApi::toString(EValueMode mode)
{
	QMetaEnum metaEnum = getMetaEnum("EVolumeMode");
	return metaEnum.valueToKey(static_cast<int>(mode));
}
QString QtRoonTransportApi::toString(EMute mute)
{
	QMetaEnum metaEnum = getMetaEnum("EMute");
	return metaEnum.valueToKey(static_cast<int>(mute));
}
QtRoonTransportApi::EState QtRoonTransportApi::toEState(const QString& str)
{
	static QMetaEnum s_metaEnum;
	if (!s_metaEnum.isValid())
		s_metaEnum = getMetaEnum("EState");
    return static_cast<EState>(s_metaEnum.keyToValue(str.toUtf8()));
}
QtRoonTransportApi::ELoop QtRoonTransportApi::toELoop(const QString& str)
{
	static QMetaEnum s_metaEnum;
	if (!s_metaEnum.isValid())
		s_metaEnum = getMetaEnum("ELoop");
    return static_cast<ELoop>(s_metaEnum.keyToValue(str.toUtf8()));
}
QtRoonTransportApi::ESourceControlStatus QtRoonTransportApi::toESourceControlStatus(const QString& str)
{
	static QMetaEnum s_metaEnum;
	if (!s_metaEnum.isValid())
		s_metaEnum = getMetaEnum("ESourceControlStatus");
    return static_cast<ESourceControlStatus>(s_metaEnum.keyToValue(str.toUtf8()));
}

QMetaEnum QtRoonTransportApi::getMetaEnum(const char * enumName)
{
	const QMetaObject& mo = QtRoonTransportApi::staticMetaObject;
	int index = mo.indexOfEnumerator(enumName);
	return mo.enumerator(index);
}

