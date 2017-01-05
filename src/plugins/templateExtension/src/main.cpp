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
#include <stdexcept>
#include "main.h"
#include "configwidget.h"
#include "item.h"
#include "query.h"

/** ***************************************************************************/
Template::Extension::Extension()
    : Core::Extension("org.albert.extension.template"),
      Core::QueryHandler(Core::Extension::id) {

    // You can throw in the constructor if something fatal happened
    throw std::runtime_error( "Description of error." );
    throw std::string( "Description of error." );
    throw QString( "Description of error." );
    throw "Description of error.";
    throw; // Whatever prints "unknown error"

}



/** ***************************************************************************/
Template::Extension::~Extension() {
    // Do sth.
}



/** ***************************************************************************/
QWidget *Template::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);
    }
    return widget_;
}



/** ***************************************************************************/
void Template::Extension::setupSession() {

}



/** ***************************************************************************/
void Template::Extension::teardownSession() {

}



/** ***************************************************************************/
void Template::Extension::handleQuery(Core::Query * query) {
    // Avoid annoying warnings
    Q_UNUSED(query)
}

