// Copyright (C) 2014-2021 Manuel Schneider

#include <QStandardPaths>
#include <QCoreApplication>
#include "albert/plugininstance.h"
#include "albert/pluginprovider.h"
#include "albert/pluginspec.h"
#include "albert/extensionmanager.h"
using namespace std;

struct Core::PluginInstance::Private {
    PluginSpec &spec;
};


Core::PluginInstance::PluginInstance(PluginSpec& spec) :
    d(new Private{spec})
{
}


Core::PluginInstance::~PluginInstance()
{
}


Core::PluginSpec &Core::PluginInstance::spec()
{
    return d->spec;
}


QString Core::PluginInstance::id() const
{
    return d->spec.id;
}


QWidget *Core::PluginInstance::widget(QWidget *)
{
    return nullptr;
}


QDir Core::PluginInstance::cacheLocation() const
{
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if ( !cacheDir.exists(d->spec.id) )
        cacheDir.mkdir(d->spec.id);
    cacheDir.cd(d->spec.id);
    return cacheDir;
}


QDir Core::PluginInstance::configLocation() const
{
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if ( !configDir.exists(d->spec.id) )
        configDir.mkdir(d->spec.id);
    configDir.cd(d->spec.id);
    return configDir;
}


QDir Core::PluginInstance::dataLocation() const
{
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if ( !dataDir.exists(d->spec.id) )
        dataDir.mkdir(d->spec.id);
    dataDir.cd(d->spec.id);
    return dataDir;
}


unique_ptr<QSettings> Core::PluginInstance::settings()
{
    auto conf = make_unique<QSettings>(qApp->applicationName());
    conf->beginGroup(spec().id);
    return conf;
}

