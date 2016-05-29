// albert extension mpris - a mpris interface plugin for albert
// Copyright (C) 2016 Martin Buergmann
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
#include <QVariant>
#include <vector>
using std::vector;
#include "abstractobjects.hpp"
#include "player.h"
#include <QDBusMessage>

namespace MPRIS {
class Item final : public AlbertItem
{
public:
    Item(Player& p, QString& subtext, QString& iconPath, QDBusMessage& msg, bool hideAfter);
    ~Item();

    QString text() const override;
    QString subtext() const override;
    QString iconPath() const override;
    void activate(ExecutionFlags *) override;

private:
    Player& player_;
    QString text_;
    QString subtext_;
    QString iconPath_;
    QDBusMessage message_;
    bool hideAfter_;
};
}
