// Copyright (C) 2021 Manuel Schneider

#include <QCoreApplication>
#include <QSettings>
#include "pluginprovider.h"
#include "pluginspec.h"


void Core::PluginProvider::setEnabled(const QString &id, bool enabled)
{
    enabled ? load(id) : unload(id);
    QSettings(qApp->applicationName()).setValue(QString("%1/enabled").arg(id), enabled);
}


bool Core::PluginProvider::isEnabled(const QString &id)
{
    return QSettings(qApp->applicationName()).value(QString("%1/enabled").arg(id), false).toBool();
}


void Core::PluginProvider::reloadEnabledPlugins()
{
    for (auto &spec : plugins()){
        if (spec->load_type == PluginSpec::LoadType::User){
            if (spec->state == PluginSpec::State::Loaded)
                unload(spec->id);
            if (isEnabled(spec->id))
                load(spec->id);
        }
    }
}
