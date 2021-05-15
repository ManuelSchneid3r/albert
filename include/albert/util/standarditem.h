// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QString>
#include <vector>
#include <memory>
#include "albert/item.h"

namespace Core {

class Action;

/**
* @brief A standard item
* If you dont need the flexibility subclassing the abstract classes provided,
* you can simply use this container, fill it with data.
*/
class EXPORT_CORE StandardItem : public Item
{
public:

    explicit StandardItem(
        QString id = QString(),
        QString iconPath = QString(),
        QString text = QString(),
        QString subtext = QString(),
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

    std::vector<std::shared_ptr<Action>> actions() override;
    void setActions(std::vector<std::shared_ptr<Action>> &&actions);
    void setActions(const std::vector<std::shared_ptr<Action>> &actions);
    void addAction(std::shared_ptr<Action> &&action);
    void addAction(const std::shared_ptr<Action> &action);

    QString completion() const override;
    void setCompletion(QString completion);

    Item::Urgency urgency() const override;
    void setUrgency(Item::Urgency urgency);

protected:

    QString id_;
    QString iconPath_;
    QString text_;
    QString subtext_;
    std::vector<std::shared_ptr<Action>> actions_;
    QString completion_;
    Item::Urgency urgency_;

};
#define makeStdItem std::make_shared<Core::StandardItem>

}
