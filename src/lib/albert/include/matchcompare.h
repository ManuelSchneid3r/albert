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
#include "core_globals.h"
#include "item.h"

namespace Core {

/**
 * @brief The MatchOrder class
 * The implements the order of the results
 */
class EXPORT_CORE MatchCompare
{
public:

    static void update();
    bool operator()(const std::pair<std::shared_ptr<Item>, short>& lhs,
                    const std::pair<std::shared_ptr<Item>, short>& rhs);

private:

    static std::map<QString, double> order;
};

}
