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

#include "command.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QDebug>
#include "item.h"

/** ***************************************************************************/
MPRIS::Command::Command(const QString &label, const QString &title, const QString &subtext, const QString &method, QString iconpath)
    : label_(label), title_(title), subtext_(subtext), method_(method), iconpath_(iconpath) {
}



/** ***************************************************************************/
QString& MPRIS::Command::getIconPath() {
    return iconpath_;
}



/** ***************************************************************************/
MPRIS::Command &MPRIS::Command::applicableWhen(const char* path, const char *property, const QVariant expectedValue, bool positivity) {
    path_ = path;
    property_ = property;
    expectedValue_ = expectedValue;
    positivity_ = positivity;
    applicableCheck_ = true;
    return *this;
}



/** ***************************************************************************/
SharedItem MPRIS::Command::produceAlbertItem(Player &player) const {
    QDBusMessage msg = QDBusMessage::createMethodCall(player.getBusId(), "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method_);
    SharedItem ptr(new MPRIS::Item(player, title_, subtext_, iconpath_, msg));
    return ptr;
}



/** ***************************************************************************/
bool MPRIS::Command::isApplicable(Player &p) const {
    // Check the applicable-option if given
    if (!applicableCheck_)
        return true;

    // split DBus interface and property into seperate strings
    int splitAt = property_.lastIndexOf('.');
    QString ifaceName = property_.left(splitAt);
    QString propertyName = property_.right(property_.length() - splitAt -1);

    // Compose Get-Property-Message
    QDBusMessage mesg = QDBusMessage::createMethodCall(
                p.getBusId(), //"org.mpris.MediaPlayer2.rhythmbox",
                path_, //"/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties",
                "Get");
    QList<QVariant> args;
    // Specify DBus interface to get the property from and the property-name
    args.append(ifaceName); //"org.mpris.MediaPlayer2.Player");
    args.append(propertyName); //"CanGoNext");
    mesg.setArguments(args);

    // Query the property
    QDBusMessage reply = QDBusConnection::sessionBus().call(mesg);

    // Check if the result is as expected
    QVariant result = reply.arguments().at(0).value<QDBusVariant>().variant();
    return (result == expectedValue_) == positivity_;
}



/** ***************************************************************************/
QString& MPRIS::Command::getLabel() {
    return label_;
}



/** ***************************************************************************/
QString& MPRIS::Command::getMethod() {
    return method_;
}



/** ***************************************************************************/
QString& MPRIS::Command::getTitle() {
    return title_;
}

