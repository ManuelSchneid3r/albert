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

#include <QDebug>
#include "extension.h"
#include "configwidget.h"
#include "abstractitem.h"
#include "query.h"

/** ***************************************************************************/
FirefoxBookmarks::Extension::Extension() : AbstractExtension("org.albert.extension.firefoxbookmarks") {
    // Do sth.
}



/** ***************************************************************************/
FirefoxBookmarks::Extension::~Extension() {
    // Do sth.
}



/** ***************************************************************************/
QWidget *FirefoxBookmarks::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);
    }
    return widget_;
}



/** ***************************************************************************/
void FirefoxBookmarks::Extension::setupSession() {

}



/** ***************************************************************************/
void FirefoxBookmarks::Extension::teardownSession() {

}



/** ***************************************************************************/
void FirefoxBookmarks::Extension::handleQuery(Query query) {
    // Avoid annoying warnings
    Q_UNUSED(query)
}

