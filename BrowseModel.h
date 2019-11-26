#ifndef BROWSEMODEL_H
#define BROWSEMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include "QtRoonBrowseApi.h"

class ModelItem
{
public:
    ModelItem ()
    {}
    ModelItem (const QtRoonBrowseApi::BrowseItem& item, const QString& url) :
        _item_key(item.item_key),
        _title(item.title),
        _sub_title(item.subtitle),
        _image_url(url),
        _input_prompt(item.input_prompt == nullptr ? "" : item.input_prompt->prompt)
    {
    }
    /*
    ModelItem (const ModelItem& item) :
        _item_key(item.item_key()),
        _title(item.title()),
        _sub_title(item.sub_title()),
        _image_url(item.image_url()),
        _input_prompt(item.input_prompt())
    {
    }
    */
    QString item_key()      const { return _item_key; }
    QString title()         const { return _title; }
    QString sub_title()     const { return _sub_title; }
    QString image_url()     const { return _image_url; }
    QString input_prompt()  const { return _input_prompt; }
private:
    QString _item_key;
    QString _title;
    QString _sub_title;
    QString _image_url;
    QString _input_prompt;
};

class BrowseModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum BrowseRoles {
        ItemKeyRole = Qt::UserRole + 1,
        TitleRole, SubTitleRole, ImageUrlRole    };

    explicit BrowseModel(QObject* parent = nullptr);
    ~BrowseModel();

    Q_INVOKABLE QString getKey (int index);
    Q_INVOKABLE QVariantMap getItem (int index);

    void begin () {
        beginInsertRows(QModelIndex(), 0, rowCount() - 1);
    }
    void end () {
        endInsertRows();
    }
    void addItem (const QtRoonBrowseApi::BrowseItem& item, const QString& url) {
        //beginInsertRows(QModelIndex(), rowCount(), rowCount());
        _items.append(ModelItem(item, url));
        //endInsertRows();
    }
    int rowCount(const QModelIndex & parent = QModelIndex()) const
    {
        Q_UNUSED(parent)
        return _items.count();
    }
    void clear();
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

protected:
    QHash<int, QByteArray> roleNames() const;
private:
    QList<ModelItem> _items;
};


#endif // BROWSEMODEL_H
