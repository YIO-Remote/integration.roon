#include <QJsonDocument>
#include "QtRoonBrowseApi.h" 

QtRoonBrowseApi::ICallback::~ICallback()
{}

QtRoonBrowseApi::BrowseInput::BrowseInput() :
	is_password(false)
{}
QtRoonBrowseApi::BrowseInput::BrowseInput(const BrowseInput& other) :
    prompt(other.prompt),
    action(other.action),
    value(other.value),
    is_password(other.is_password)
{}
QtRoonBrowseApi::BrowseInput::BrowseInput(const QVariantMap& map) {
    prompt = map["prompt"].toString();
    action = map["action"].toString();
    value = map["value"].toString();
    is_password = map["is_password"].toBool();
}
QtRoonBrowseApi::BrowseItem::BrowseItem() :
    input_prompt(nullptr)
{}
QtRoonBrowseApi::BrowseItem::~BrowseItem() {
    if (input_prompt != nullptr) delete input_prompt;
    input_prompt = nullptr;
}
QtRoonBrowseApi::BrowseItem::BrowseItem(const BrowseItem& other) :
    title(other.title),
    subtitle(other.subtitle),
    item_key(other.item_key),
    image_key(other.image_key),
    hint(other.hint),
    input_prompt(other.input_prompt == nullptr ? nullptr : new BrowseInput(*other.input_prompt))
{}
QtRoonBrowseApi::BrowseItem::BrowseItem(const QVariantMap& map) {
    title = map["title"].toString();
    subtitle = map["subtitle"].toString();
    item_key = map["item_key"].toString();
    image_key = map["image_key"].toString();
    hint = map["hint"].toString();
    if (map.contains("input_prompt"))
        input_prompt = new BrowseInput(map["input_prompt"].toMap());
    else
        input_prompt = nullptr;
}
QtRoonBrowseApi::BrowseItem& QtRoonBrowseApi::BrowseItem::operator= (const QtRoonBrowseApi::BrowseItem& other) {
    title = other.title;
    subtitle = other.subtitle;
    item_key = other.item_key;
    image_key = other.image_key;
    hint = other.hint;
    input_prompt = other.input_prompt == nullptr ? nullptr : new BrowseInput(*other.input_prompt);
    return *this;
}

QtRoonBrowseApi::BrowseList::BrowseList() :
    count(-1),
    level(-1),
    display_offset(-1)
{}
QtRoonBrowseApi::BrowseList::BrowseList(const QVariantMap& map) {
    title = map["title"].toString();
    subtitle = map["subtitle"].toString();
    image_key = map["image_key"].toString();
    hint = map["hint"].toString();
    count = map["count"].toInt();
    level = map["level"].toInt();
    display_offset = map["display_offset"].toInt();
}
QtRoonBrowseApi::BrowseResult::BrowseResult() :
    list(nullptr),
    item(nullptr),
    is_error(false)
{}
QtRoonBrowseApi::BrowseResult::BrowseResult(const QVariantMap& map) {
    action = map["action"].toString();
    message = map["message"].toString();
    is_error = map["is_error"].toBool();
    if (map.contains("list"))
        list = new BrowseList(map["list"].toMap());
    else
        list = nullptr;
    if (map.contains("item"))
        item = new BrowseItem(map["item"].toMap());
    else
        item = nullptr;
}
QtRoonBrowseApi::BrowseResult::~BrowseResult() {
    if (list != nullptr) delete list;
    if (item != nullptr) delete item;
}

QtRoonBrowseApi::LoadResult::LoadResult() :
    offset(-1),
    list(nullptr)
{}
QtRoonBrowseApi::LoadResult::LoadResult(const QVariantMap& map) {
    offset = map["offset"].toInt();
    if (map.contains("list"))
        list = new BrowseList(map["list"].toMap());
    else
        list = nullptr;
    QVariantList itms = map["items"].toList();
    for (int i = 0; i < itms.count(); i++)
        items.append(BrowseItem(itms[i].toMap()));
}
QtRoonBrowseApi::LoadResult::~LoadResult() {
    if (list != nullptr) delete list;
}

void QtRoonBrowseApi::LoadOption::toVariant(QVariantMap& map) const
{
    map["hierarchy"] = hierarchy;
    if (!multi_session_key.isEmpty())
        map["multi_session_key"] = multi_session_key;
    if (level >= 0)
        map["level"] = level;
    if (set_display_offset >= 0)
        map["set_display_offset"] = set_display_offset;
    if (offset >= 0)
        map["offset"] = offset;
    if (count >= 0)
        map["count"] = count;

}
void QtRoonBrowseApi::LoadOption::toJson(QString& json) const
{
    QVariantMap map;
    toVariant(map);
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    json = doc.toJson(QJsonDocument::JsonFormat::Compact);
}
void QtRoonBrowseApi::BrowseOption::toVariant(QVariantMap& map) const
{
    map["hierarchy"] = hierarchy;
    if (!input.isEmpty())
        map["input"] = input;
    if (!multi_session_key.isEmpty())
        map["multi_session_key"] = multi_session_key;
    if (!zone_or_output_id.isEmpty())
        map["zone_or_output_id"] = zone_or_output_id;
    if (set_display_offset >= 0)
        map["set_display_offset"] = set_display_offset;
    if (!item_key.isEmpty())
        map["item_key"] = item_key;
    else if (pop_all)
        map["pop_all"] = pop_all;
    else if (refresh_list)
        map["refresh_list"] = refresh_list;
    else if (pop_levels >= 0)
        map["pop_levels"] = pop_levels;
}
void QtRoonBrowseApi::BrowseOption::toJson(QString& json) const
{
    QVariantMap map;
    toVariant(map);
    QJsonDocument doc = QJsonDocument::fromVariant(map);
    json = doc.toJson(QJsonDocument::JsonFormat::Compact);
}


QtRoonBrowseApi::QtRoonBrowseApi(QtRoonApi& roonApi, ICallback* callback, QObject* parent) :
    QObject(parent),
    _roonApi(roonApi),
    _callback(callback),
    _browseCallback(nullptr),
    _loadCallback(nullptr)
{
    memset (_context, 0, sizeof (_context));
}
void QtRoonBrowseApi::OnReceived(const ReceivedContent& content)
{
    if (_roonApi.Log().isDebugEnabled())
        qCDebug(_roonApi.Log()) << "RoonBrowseApi.OnReceived : " << content._messageType << " " << content._requestId << " " << content._service << content._command;

    QJsonDocument document = QJsonDocument::fromJson(content._body.toUtf8());
    QVariantMap map = document.toVariant().toMap();
    QString err;
    if (content._command != "Success")
        err = "Error: " + content._command;
    if (_callback != nullptr) {
        Context* context = _context[content._requestId % NUMCONTEXTS];
        if (context == nullptr || context->requestId != content._requestId) {
            qCWarning(_roonApi.Log()) << "RoonBrowseApi.OnReceived context not found : "  << content._requestId;
            return;
        }
        if (context->isLoad) {
            LoadResult result(map);
            _callback->OnLoad(err, *context, result);
        }
        else {
            BrowseResult result(map);
            if (result.is_error)
                err = result.message;
            _callback->OnBrowse(err, *context, result);
        }
        _context[content._requestId % NUMCONTEXTS] = nullptr;
    }
    else if (_browseCallback != nullptr) {
        BrowseResult result(map);
        if (result.is_error)
            err = result.message;
        _browseCallback(content._requestId, err, result);
        _browseCallback = nullptr;
    }
    else if (_loadCallback != nullptr) {
        LoadResult result(map);
        _loadCallback(content._requestId, err, result);
        _loadCallback = nullptr;
    }
}

int QtRoonBrowseApi::browse(const BrowseOption& option, void (*func) (int requestId, const QString& err, const BrowseResult& result))
{
    _browseCallback = func;
    QString	json;
    option.toJson(json);
    return _roonApi.send(QtRoonApi::ServiceBrowse + "/browse", this, &json);
}
int QtRoonBrowseApi::load(const LoadOption& option, void (*func) (int requestId, const QString& err, const LoadResult& result))
{
    _loadCallback = func;
    QString json;
    option.toJson(json);
    return _roonApi.send(QtRoonApi::ServiceBrowse + "/load", this, &json);
}

int QtRoonBrowseApi::browse (const BrowseOption& option, Context& context)
{
    QString	json;
    option.toJson(json);
    int requestId = _roonApi.send(QtRoonApi::ServiceBrowse + "/browse", this, &json);
    context.requestId = requestId;
    context.isLoad = false;
    _context[requestId % NUMCONTEXTS] = &context;
    return requestId;
}
int QtRoonBrowseApi::load (const LoadOption& option, Context& context) {
    QString	json;
    option.toJson(json);
    int requestId = _roonApi.send(QtRoonApi::ServiceBrowse + "/load", this, &json);
    context.requestId = requestId;
    context.isLoad = true;
    _context[requestId % NUMCONTEXTS] = &context;
    return requestId;
}


