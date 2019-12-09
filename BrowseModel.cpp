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

#include "BrowseModel.h"

BrowseModel::BrowseModel(QObject* parent) :
    QAbstractListModel(parent)
{}
BrowseModel::~BrowseModel()
{
}

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

QHash<int, QByteArray> BrowseModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ItemKeyRole] = "item_key";
    roles[TitleRole] = "title";
    roles[SubTitleRole] = "sub_title";
    roles[ImageUrlRole] = "image_url";
    roles[InputPromptRole] = "input_prompt";
    return roles;
}

QString BrowseModel::getKey (int index) {
    if (index >= _items.length())
        return "";
    return _items[index].item_key();
}

QObject* BrowseModel::getItem (int index) {
    if (index < _items.length()) {
        // magically freed by the Qt Engine
        return new QModelItem(_items.value(index), parent());
    }
    return nullptr;
}

