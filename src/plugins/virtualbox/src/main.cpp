// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
// Contributed to by 2016 Martin Buergmann
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

#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QFile>
#include <QStandardPaths>
#include <QDomDocument>
#include <QDomElement>
#include "main.h"
#include "query.h"
#include "xdgiconlookup.h"
#include "standarditem.h"
#include "standardaction.h"
using Core::Action;
using Core::StandardAction;
using Core::StandardItem;

/** ***************************************************************************/
VirtualBox::Extension::Extension()
    : Core::Extension("org.albert.extension.virtualbox"),
      Core::QueryHandler(Core::Extension::id) {
    QString iconPath = XdgIconLookup::instance()->themeIconPath("virtualbox");
    iconPath_ = iconPath.isNull() ? ":vbox" : iconPath;
    VMItem::iconPath_ = iconPath_;
}



/** ***************************************************************************/
QWidget *VirtualBox::Extension::widget(QWidget *parent) {
    return new QWidget(parent);
}



/** ***************************************************************************/
void VirtualBox::Extension::setupSession() {

    QFile vboxConfigFile(QStandardPaths::locate(QStandardPaths::ConfigLocation, "VirtualBox/VirtualBox.xml"));
    if (!vboxConfigFile.exists())
        return;
    if (!vboxConfigFile.open(QFile::ReadOnly)) {
        qCritical("Could not open VBox config file for read operation!");
        return;
    }

    QDomDocument vboxConfig;
    QString errMsg = "";
    int errLine = 0, errCol = 0;
    if (!vboxConfig.setContent(&vboxConfigFile, &errMsg, &errLine, &errCol)) {
        qWarning("Parsing of VBox config failed because %s in line %d col %d", errMsg.toStdString().c_str(), errLine, errCol);
        vboxConfigFile.close();
        return;
    }
    vboxConfigFile.close();

    QDomElement root = vboxConfig.documentElement();
    if (root.isNull()) {
        qCritical("root element is null");
        return;
    }

    QDomElement global = root.firstChildElement("Global");
    if (global.isNull()) {
        qCritical("global element is null");
        return;
    }

    QDomElement machines = global.firstChildElement("MachineRegistry");  // List of MachineEntry
    if (machines.isNull()) {
        qCritical("machine registry element is null");
        return;
    }

    // With this we iterate over the machine entries
    QDomElement machine = machines.firstChildElement();

    // This list holds the deteceted uuids
    QList<QString> uuids;

    // And we count how many entries we find for information reasons
    int found = 0;

    while (!machine.isNull()) {
        uuids.append(machine.attribute("uuid"));
        machine = machine.nextSiblingElement();
        found++;
    }
    qDebug("Found %d VMs", found);

    for (VM *item : vms_) { // Check all VM we already got.
        if (uuids.contains(item->uuid())) { // Every uuid we alredy have and was found in the config we keep
            uuids.removeAll(item->uuid());  // and delete it from the list
        } else {                            // Every uuid we already have but was not found in the config we delete
            vms_.removeAll(item);
            delete item;
        }
    }

    // Now the QList uuids contains only uuids which do not have a corresponding object in vms_
    // So we create VM objects for them
    machine = machines.firstChildElement();
    while (!machine.isNull()) {
        if (uuids.contains(machine.attribute("uuid")))
            vms_.append(new VM(machine.attribute("src")));
        machine = machine.nextSiblingElement();
    }

    /*
    QProcess *process = new QProcess;
    process->setReadChannel(QProcess::StandardOutput);
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [this, process](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus == QProcess::NormalExit && exitCode == 0){
            while (process->canReadLine()) {
               QString line = QString::fromLocal8Bit(process->readLine());
               vms_.append(new VM(line));
            }
        }
        process->deleteLater();
    });
    process->start("VBoxManage",  {"list", "vms"});
    */
}



/** ***************************************************************************/
void VirtualBox::Extension::handleQuery(Core::Query * query) {

/* Rebase-Conflict Artifact 
<<<<<<< HEAD:src/plugins/virtualbox/src/main.cpp
   for (uint i = 0; i < names_.size(); ++i){
       if (names_[i].startsWith(query->searchTerm(), Qt::CaseInsensitive)) {
=======
    //*
    for (uint i = 0; i < names_.size(); ++i){
        if (names_[i].startsWith(query.searchTerm(), Qt::CaseInsensitive)) {
>>>>>>> Improved VirtualBox extension:src/plugins/virtualbox/extension.cpp

            std::shared_ptr<StandardItem> item = std::make_shared<StandardItem>(uuids_[i]);
            item->setText(names_[i]);
            item->setSubtext(QString("'%1' aka '%2'").arg(names_[i], uuids_[i]));
            item->setIconPath(iconPath_);

<<<<<<< HEAD:src/plugins/virtualbox/src/main.cpp
           std::shared_ptr<StandardAction> action = std::make_shared<StandardAction>();
           action->setText("Start virtual machine");
           action->setAction([this, i](){
               QProcess::startDetached("VBoxManage", {"startvm", uuids_[i]});
           });
=======
            std::shared_ptr<StandardAction> action = std::make_shared<StandardAction>();
            action->setText("Start virtual machine");
            action->setAction([this, i](ExecutionFlags *){
                QProcess::startDetached("VBoxManage", {"startvm", uuids_[i]});
            });
>>>>>>> Improved VirtualBox extension:src/plugins/virtualbox/extension.cpp

            item->setActions({action});

<<<<<<< HEAD:src/plugins/virtualbox/src/main.cpp
           query->addMatch(item);
       }
   }
=======
            query.addMatch(item);
        }
    } * /
    //*
    for (uint i = 0; i < names_.size(); ++i){
        if (names_[i].startsWith(query->searchTerm(), Qt::CaseInsensitive)) {
            std::shared_ptr<StandardItem> item = std::make_shared<StandardItem>();
            item->setText(names_[i]);
            item->setSubtext(QString("Start '%1'").arg(names_[i]));
            item->setIcon(iconPath_);
            item->setAction([this, i](){
                QProcess::startDetached("VBoxManage", {"startvm", uuids_[i]});
            });
            query->addMatch(item);
        }
    }
    */
    for (VM* vm : vms_) {
        if (vm->startsWith(query.searchTerm()))
            query.addMatch(std::shared_ptr<AbstractItem>(vm->produceItem()));
    }
//>>>>>>> Improved VirtualBox extension:src/plugins/virtualbox/extension.cpp
}
