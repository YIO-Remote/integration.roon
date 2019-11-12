#pragma once
#include <QtCore/QObject>
#include "QtRoonApi.h"

class QtRoonBrowseApi : public QObject, public IRoonCallback
{
    Q_OBJECT
public:
    struct BrowseInput {
        BrowseInput();
        BrowseInput(const BrowseInput& other);
        BrowseInput(const QVariantMap& map);
        QString			prompt;
        QString			action;
        QString			value;
        bool			is_password;
    };
    struct BrowseItem {
        BrowseItem();
        ~BrowseItem();
        BrowseItem(const BrowseItem& other);
        BrowseItem(const QVariantMap& map);
        BrowseItem& operator=(const BrowseItem& other);
        QString			title;
        QString			subtitle;
        QString			item_key;
        QString			image_key;
        QString			hint;
        BrowseInput*            input_prompt;
    };
    struct BrowseOption {
        BrowseOption() {
            pop_levels = set_display_offset = -1; pop_all = refresh_list = false;
            hierarchy = "browse";
        }
        QString			hierarchy;
        QString			zone_or_output_id;
        QString			multi_session_key;
        QString			item_key;
        QString			input;
        int			pop_levels;
        bool			pop_all;
        bool			refresh_list;
        int			set_display_offset;
        void			toVariant(QVariantMap& map) const;
        void			toJson(QString& json) const;
    };
    struct BrowseList {
        BrowseList();
        BrowseList(const QVariantMap& map);
        int			count;
        int			level;
        int			display_offset;
        QString			title;
        QString			subtitle;
        QString			image_key;
        QString			hint;
    };
    struct BrowseResult {
        BrowseResult();
        BrowseResult(const QVariantMap& map);
        ~BrowseResult();
        QString			action;
        BrowseList*		list;
        BrowseItem*		item;
        bool			is_error;
        QString			message;
    };
    struct LoadOption {
        LoadOption() { level = offset = count = set_display_offset = -1; hierarchy = "browse"; }
        QString			hierarchy;
        int			level;
        int			offset;
        int			count;
        int			set_display_offset;
        QString			multi_session_key;
        void			toVariant(QVariantMap& map) const;
        void			toJson(QString& json) const;
    };
    struct LoadResult {
        LoadResult();
        LoadResult(const QVariantMap& map);
        ~LoadResult();
        QList<BrowseItem>	items;
        int			offset;
        BrowseList*		list;
    };

    explicit	QtRoonBrowseApi(QtRoonApi& roonApi, QObject* parent = nullptr);
    virtual	void OnReceived(const ReceivedContent& content) override;

    int browse	(const BrowseOption& option, void (*func) (int requestId, const QString& err, const BrowseResult& result));
    int load	(const LoadOption& option,	void (*func) (int requestId, const QString& err, const LoadResult& result));

private:
    QtRoonApi&				_roonApi;
    void (*_browseCallback) (int requestId, const QString& err, const BrowseResult& result);
    void (*_loadCallback)   (int requestId, const QString& err, const LoadResult& result);

};
