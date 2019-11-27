#include "BrowseModel.h"

QVariantMap ModelItem::toVariantMap() const
{
    QVariantMap map;
    map["item_key"] = _item_key;
    map["title"] = _title;
    map["sub_title"] = _sub_title;
    map["image_url"] = _image_url;
    map["input_prompt"] = _input_prompt;
    return map;
}
QVariantMap ModelHeader::toVariantMap() const
{
    QVariantMap map;
    map["type"] = _type;
    map["title"] = _title;
    map["level"] = _level;
    return map;
}

BrowseModel::BrowseModel(QObject* parent) :
    QAbstractListModel(parent)
{}
BrowseModel::~BrowseModel()
{}

void BrowseModel::clear() {
    if (_items.count() > 0) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        _items.clear();
        endRemoveRows();
    }
}
void BrowseModel::setHeader (const QString& type, const QString& title, int level)
{
    _header.setType(type); _header.setTitle(title); _header.setLevel(level);
    emit headerChanged();
    setHeaderData(0, Qt::Orientation::Vertical, type, TypeRole);
    setHeaderData(0, Qt::Orientation::Vertical, title, TitleRole);
    setHeaderData(0, Qt::Orientation::Vertical, level, LevelRole);
    emit headerDataChanged(Qt::Orientation::Vertical, 0, 0);
}
void BrowseModel::setPlayCommands(const QStringList& playCommands)
{
    _playCommands.clear();
    _playCommands.append(playCommands);
    emit playCommandsChanged();
}

QVariant BrowseModel::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() >= _items.count())
        return QVariant();
    const ModelItem &item = _items[index.row()];
    switch (role) {
        case ItemKeyRole:     return item.item_key();
        case TitleRole:       return item.title();
        case SubTitleRole:    return item.sub_title();
        case ImageUrlRole:    return item.image_url();
        case InputPromptRole: return item.input_prompt();
    }
    return QVariant();
}
QVariant BrowseModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    switch (role) {
        case TypeRole:     return _header.type();
        case TitleRole:    return _header.title();
        case LevelRole:    return _header.level();
    }
    return QVariant();
}

QHash<int, QByteArray> BrowseModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemKeyRole] = "item_key";
    roles[TitleRole] = "title";
    roles[SubTitleRole] = "sub_title";
    roles[ImageUrlRole] = "image_url";
    roles[InputPromptRole] = "input_prompt";
    roles[TypeRole] = "type";
    return roles;
}

QString BrowseModel::getKey (int index) {
    if (index >= _items.length())
        return "";
    return _items[index].item_key();
}
QVariantMap BrowseModel::getItem (int index) {
    if (index < _items.length()) {
        return _items.value(index).toVariantMap();
    }
    return QVariantMap();
}

