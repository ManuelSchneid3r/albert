// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <vector>
#include "standarditem.h"
#include "albert/indexitem.h"

namespace Core {

/**
* @brief A standard index item for use with the offline index
* If you dont need the flexibility subclassing the abstract classes provided,
* you can simply use this container, fill it with data.
*/
class EXPORT_CORE StandardIndexItem final : public IndexItem
{
public:

    explicit StandardIndexItem(
        QString id = QString(),
        QString iconPath = QString(),
        QString text = QString(),
        QString subtext = QString(),
        std::vector<Core::IndexItem::IndexString> indexStrings = std::vector<Core::IndexItem::IndexString>(),
        std::vector<std::shared_ptr<Action>> actions = std::vector<std::shared_ptr<Action>>(),
        QString completion = QString(),
        Urgency urgency = Item::Urgency::Normal
    );

    QString id() const override;
    void setId(QString id);

    QString iconPath() const override;
    void setIconPath(QString iconPath);

    QString text() const override;
    void setText(QString text);

    QString subtext() const override;
    void setSubtext(QString subtext);

    virtual std::vector<Core::IndexItem::IndexString> indexStrings() const override;
    void setIndexKeywords(std::vector<Core::IndexItem::IndexString> indexStrings);

    std::vector<std::shared_ptr<Action>> actions() override;
    void setActions(std::vector<std::shared_ptr<Action>> actions);
    void addAction(std::shared_ptr<Action> action);

    QString completion() const override;
    void setCompletion(QString completion);

    Item::Urgency urgency() const override;
    void setUrgency(Item::Urgency urgency);


protected:

    QString id_;
    QString iconPath_;
    QString text_;
    QString subtext_;
    std::vector<IndexItem::IndexString> indexStrings_;
    std::vector<std::shared_ptr<Action>> actions_;
    QString completion_;
    Item::Urgency urgency_;

};
#define makeStdIdxItem std::make_shared<Core::StandardIndexItem>

}
