// albert - a simple application launcher for linux
// Copyright (C) 2014-2016 Manuel Schneider
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

    explicit FuzzySearch(unsigned int q = 3, double d = 2);
    explicit FuzzySearch(const PrefixSearch& rhs, unsigned int q = 3, double d = 2);
    ~FuzzySearch();

    void add(std::shared_ptr<Indexable> idxble) override;
    void clear() override;
    std::vector<std::shared_ptr<Indexable>> search(const QString &req) const override;
    inline double delta() const {return delta_;}
    inline void setDelta(double d){delta_=d;}

private:

    // Map of qGrams, containing their word references and #occurences
    typedef std::map<QString, std::map<QString, unsigned int>> QGramIndex;
    QGramIndex qGramIndex_;

    // Size of the slices
    unsigned int q_;

    // Maximum error
    double delta_;
};

}
