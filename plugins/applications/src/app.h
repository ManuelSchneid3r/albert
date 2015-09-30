// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
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

#include <QStringList>
#include <QString>
#include <QIcon>
#include "utils/search/iindexable.h"

namespace Applications {

struct App final : public IIndexable
{
    QStringList aliases() const override {
        return QStringList() << name << altName << exec;
    }

    QString        path;
    QString        name;
    QString        altName;
    QString        exec;
    bool           terminal;
    QIcon          icon;
    mutable ushort usage;
};
}
