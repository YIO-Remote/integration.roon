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
    QString item_key()          const { return _item_key; }
    QString title()             const { return _title; }
    QString sub_title()         const { return _sub_title; }
    QString image_url()         const { return _image_url; }
    QString input_prompt()      const { return _input_prompt; }
    QVariantMap toVariantMap()  const;
private:
    QString _item_key;
    QString _title;
    QString _sub_title;
    QString _image_url;
    QString _input_prompt;
};
class ModelHeader
{
public:
    ModelHeader ()
    {}
    QString type()                          const { return _type; }
    QString title()                         const { return _title; }
    int     level()                         const { return _level; }
    void    setType (const QString& type)   { _type = type; }
    void    setTitle(const QString& title)  { _title = title; }
    void    setLevel(int level)             { _level = level; }
    QVariantMap toVariantMap()  const;
private:
    QString _type;
    QString _title;
    int     _level;
};

class BrowseModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum BrowseRoles {
        ItemKeyRole = Qt::UserRole + 1,
        TitleRole, SubTitleRole, ImageUrlRole, InputPromptRole, TypeRole, LevelRole    };

    explicit BrowseModel(QObject* parent = nullptr);
    ~BrowseModel();

    Q_PROPERTY  (QVariantMap    header          READ header         NOTIFY headerChanged)
    Q_PROPERTY  (QStringList    playCommands    READ playCommands   NOTIFY playCommandsChanged)
    Q_INVOKABLE QString         getKey (int index);
    Q_INVOKABLE QVariantMap     getItem (int index);

    QVariantMap header ()
    {
        return _header.toVariantMap();
    }
    QStringList playCommands ()
    {
        return _playCommands;
    }

    void begin ()
    {
        beginInsertRows (QModelIndex(), 0, rowCount() - 1);
    }
    void end ()
    {
        endInsertRows();
    }
    void addItem        (const QtRoonBrowseApi::BrowseItem& item, const QString& url)
    {
        //beginInsertRows(QModelIndex(), rowCount(), rowCount());
        _items.append(ModelItem(item, url));
        //endInsertRows();
    }
    void setHeader      (const QString& type, const QString& title, int level);
    void setPlayCommands(const QStringList& playCommands);

    int rowCount        (const QModelIndex & parent = QModelIndex()) const
    {
        Q_UNUSED(parent)
        return _items.count();
    }
    void clear();
    QVariant data       (const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

signals:
    void headerChanged();
    void playCommandsChanged();

protected:
    QHash<int, QByteArray> roleNames() const;
private:
    QList<ModelItem>    _items;
    ModelHeader         _header;
    QStringList         _playCommands;
};


#endif // BROWSEMODEL_H
