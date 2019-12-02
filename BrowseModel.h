#ifndef BROWSEMODEL_H
#define BROWSEMODEL_H

#include <QObject>
#include <QAbstractListModel>

class ModelItem
{
public:
    ModelItem ()
    {}
    ModelItem (const QString& itemKey, const QString& title, const QString& subTitle, const QString& url, const QString& inputPrompt) :
        _item_key(itemKey),
        _title(title),
        _sub_title(subTitle),
        _image_url(url),
        _input_prompt(inputPrompt)
    {
    }
    ModelItem (const ModelItem& item) :
        _item_key(item._item_key),
        _title(item._title),
        _sub_title(item._sub_title),
        _image_url(item._image_url),
        _input_prompt(item._input_prompt)
    {
    }
    QString item_key()          const { return _item_key; }
    QString title()             const { return _title; }
    QString sub_title()         const { return _sub_title; }
    QString image_url()         const { return _image_url; }
    QString input_prompt()      const { return _input_prompt; }

private:
    QString _item_key;
    QString _title;
    QString _sub_title;
    QString _image_url;
    QString _input_prompt;
};

// for use in QML
class QModelItem : public QObject, public ModelItem
{
    Q_OBJECT
public:
    explicit QModelItem (const ModelItem& item, QObject* parent = nullptr) :
        QObject(parent),
        ModelItem(item)
    {
    }
    explicit QModelItem (QObject* parent = nullptr) :
        QObject(parent)
    {
    }
    Q_PROPERTY(QString  item_key        READ item_key       CONSTANT)
    Q_PROPERTY(QString  title           READ title          CONSTANT)
    Q_PROPERTY(QString  sub_title       READ sub_title      CONSTANT)
    Q_PROPERTY(QString  image_url       READ image_url      CONSTANT)
    Q_PROPERTY(QString  input_prompt    READ input_prompt   CONSTANT)
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
private:
    QString _type;
    QString _title;
    int     _level;
};

// for use in QML
class QModelHeader : public QObject, public ModelHeader
{
    Q_OBJECT
public:
    explicit QModelHeader (QObject* parent = nullptr) : QObject(parent)
    {}
    Q_PROPERTY(QString  type  READ type  CONSTANT)
    Q_PROPERTY(QString  title READ title CONSTANT)
    Q_PROPERTY(int      level READ level CONSTANT)
};

class BrowseModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum BrowseRoles {
        ItemKeyRole = Qt::UserRole + 1,
        TitleRole, SubTitleRole, ImageUrlRole, InputPromptRole  };

    explicit BrowseModel(QObject* parent = nullptr);
    ~BrowseModel();

    Q_PROPERTY  (QModelHeader*  header          READ header         NOTIFY headerChanged)
    Q_PROPERTY  (QStringList    playCommands    READ playCommands   NOTIFY playCommandsChanged)
    Q_INVOKABLE QString         getKey (int index);
    Q_INVOKABLE QObject*        getItem (int index);

    QModelHeader* header ()
    {
        return &_header;
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
    void addItem        (const ModelItem& item)
    {
        //beginInsertRows(QModelIndex(), rowCount(), rowCount());
        _items.append(item);
        //endInsertRows();
    }
    void setHeader      (const QString& type, const QString& title, int level);
    void setPlayCommands(const QStringList& playCommands);

    int rowCount        (const QModelIndex& parent = QModelIndex()) const
    {
        Q_UNUSED(parent)
        return _items.count();
    }
    void clear();
    QVariant data       (const QModelIndex& index, int role = Qt::DisplayRole) const;

signals:
    void headerChanged();
    void playCommandsChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    QList<ModelItem>    _items;
    //QModelItem          _selectedItem;      // alternative dynamic allocation ? who will free it
    QModelHeader        _header;
    QStringList         _playCommands;
};
#endif // BROWSEMODEL_H
