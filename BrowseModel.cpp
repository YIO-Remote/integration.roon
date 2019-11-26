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
