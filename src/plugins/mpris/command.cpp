#include "command.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QDebug>
#include "albertapp.h"
#include "item.h"

namespace MPRIS {

Command::Command(QString &label, QString &title, QString &method, QString &iconpath)
    : label_(label), title_(title), method_(method), iconpath_(iconpath), closeOnEnter_(false)
{
//    fireCallback([](){});
}

Command::Command(const char *label, const char *title, const char *method, QString iconpath)
    : label_(label), title_(title), method_(method), iconpath_(iconpath), closeOnEnter_(false)
{
//    fireCallback([](){});
}

QString& Command::getIconPath() {
    return iconpath_;
}

Command &Command::applicableWhen(const char* path, const char *property, QVariant expectedValue, bool positivity)
{
    path_ = path;
    property_ = property;
    expectedValue_ = expectedValue;
    positivity_ = positivity;
    applicableCheck_ = true;
    return *this;
}

Command &Command::closeWhenHit()
{
    closeOnEnter_ = true;
    return *this;
}

/*
Command &Command::fireCallback(function<void ()> clbk)
{
    fireCallback_ = std::move(clbk);
    return *this;
}
*/

bool Command::closesWhenHit()
{
    return closeOnEnter_;
}

std::shared_ptr<AlbertItem> Command::produceAlbertItem(Player &player)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(player.getBusId(), "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method_);
    //std::shared_ptr<StandardItem> ptr = std::make_shared<Template::Item>(player, title_, iconpath_, msg, closeOnEnter_);
    std::shared_ptr<AlbertItem> ptr(new Template::Item(player, title_, iconpath_, msg, closeOnEnter_));
    return ptr;

/*
    // Create a StandardItem instance for this command on the given player
    std::shared_ptr<StandardItem> ptr = std::make_shared<StandardItem>();
    ptr->setIcon(iconpath_);
    ptr->setSubtext(player.getName());
    ptr->setText(title_);
    QDBusMessage msg = QDBusMessage::createMethodCall(player.getBusId(), "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method_);
    ptr->setAction([msg, this](){
        QDBusConnection::sessionBus().send(msg);
        if (closeOnEnter_)
            qApp->hideWidget();
        //fireCallback_();
    });
    return ptr;*/
}

bool Command::isApplicable(Player &p)
{
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

QString& Command::getLabel() {
    return label_;
}

QString& Command::getMethod() {
    return method_;
}

QString& Command::getTitle() {
    return title_;
}

} // namespace MPRIS
