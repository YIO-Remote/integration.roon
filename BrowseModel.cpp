#include "BrowseModel.h"

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
    }
    return QVariant();
}

QHash<int, QByteArray> BrowseModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemKeyRole] = "item_key";
    roles[TitleRole] = "title";
    roles[SubTitleRole] = "sub_title";
    roles[ImageUrlRole] = "image_url";
    return roles;
}

QString BrowseModel::getKey (int index) {
    if (index >= _items.length())
        return "";
    return _items[index].item_key();
}
QVariantMap BrowseModel::getItem (int index) {
    QVariantMap map;
    if (index < _items.length()) {
        ModelItem item = _items.value(index);
        map["item_key"] = item.item_key();
        map["title"] = item.title();
        map["sub_title"] = item.sub_title();
        map["image_url"] = item.image_url();
        map["input_prompt"] = item.input_prompt();
    }
    return map;
}

