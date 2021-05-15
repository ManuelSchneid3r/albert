// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QString>
#include <vector>
#include <set>
#include <memory>

namespace Core {

class IndexItem;

class SearchBase
{
public:

    virtual ~SearchBase();
    virtual void add(const std::shared_ptr<IndexItem> &idxble) = 0;
    virtual void clear() = 0;
    virtual std::vector<std::shared_ptr<IndexItem>> search(const QString &req) const = 0;

protected:

    std::set<QString> splitString(const QString &) const;

    static constexpr const char* SEPARATOR_REGEX  = "[!?<>\"'=+*.:,;\\\\\\/ _\\-]+";

};

}
