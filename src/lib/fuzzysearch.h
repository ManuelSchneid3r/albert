// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QString>
#include <map>
#include <memory>
#include <vector>
#include "prefixsearch.h"

namespace Core {

class FuzzySearch final : public PrefixSearch
{
public:

    explicit FuzzySearch(uint q = 3, double d = 1.0/3);
    explicit FuzzySearch(const PrefixSearch& rhs, uint q = 3, double d = 1.0/3);
    ~FuzzySearch();

    void add(const std::shared_ptr<IndexItem> &idxble) override;
    void clear() override;
    std::vector<std::shared_ptr<IndexItem>> search(const QString &req) const override;
    inline double delta() const {return delta_;}
    inline void setDelta(double d){delta_=d;}

private:

    // Map of qGrams, containing their word references and #occurences
    std::map<QString,std::map<QString,uint>> qGramIndex_;

    // Size of the slices
    uint q_;

    // Maximum error
    double delta_;
};

}
