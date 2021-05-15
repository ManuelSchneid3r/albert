// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QString>
#include <vector>
#include <memory>
#include "export.h"

namespace Core {

class Action;
typedef std::vector<std::shared_ptr<Action>> ActionList;


/**
 * @brief The item interface
 * Subclass this class to make your object displayable in the results list.
 */
class EXPORT_CORE Item
{

public:

    /**
     * An enumeration of urgency levels
     * Notifications are placed on top. Alert too but additionally get an visual
     * emphasis. Normal items are not handled in a special way.
     */
    enum class Urgency { Normal, Notification, Alert };

    virtual ~Item() {}

    /** An persistant, extensionwide unique identifier, "" if item is dynamic */
    virtual QString id() const = 0;

    /** The icon for the item */
    virtual QString iconPath() const = 0;

    /** The title for the item */
    virtual QString text() const = 0;

    /** The declarative subtext for the item */
    virtual QString subtext() const = 0;

    /** The alternative actions of the item*/
    virtual ActionList actions() = 0;

    /** The string to use for completion */
    virtual QString completion() const { return QString(); }  // Null semantic

    /** Urgency level of the item, defautls to "Normal" */
    virtual Urgency urgency() const { return Urgency::Normal; }

};

}
