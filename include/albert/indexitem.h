// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QString>
#include <vector>
#include "item.h"
#include "export.h"

namespace Core {

/**
 * @brief The Indexable class
 * The interface items need to be indexable by the offline index
 */
class EXPORT_CORE IndexItem : public Item
{

public:

    struct IndexString {
        IndexString(const QString& str, uint32_t rel) : string(str), relevance(rel){}
        QString string;
        uint32_t relevance;
    };

    virtual std::vector<IndexString> indexStrings() const = 0;

};

typedef std::vector<Core::IndexItem::IndexString> IdxStrList;

}

