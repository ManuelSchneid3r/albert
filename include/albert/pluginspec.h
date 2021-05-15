// Copyright (C) 2021 Manuel Schneider

#pragma once
#include <QString>
#include <QStringList>
#include <memory>
#include "export.h"

namespace Core {

class PluginInstance;
class PluginProvider;

struct EXPORT_CORE PluginSpec {
    PluginProvider &provider;
    const QString path;
    const QString id;
    const QString version;
    const QString name;
    const QStringList authors;
    const QStringList maintainers;
    const QStringList plugin_dependencies;
    const QStringList library_dependencies;
    const QStringList executable_dependencies;
    enum class LoadType {
        User,
        Frontend,
        OnDemand
    } load_type;
    const bool enabled_by_default;
    enum class State {
        Invalid,
        Ready,
        Loading,
        Loaded,
        Error
    } state;
    QString reason;
    PluginInstance *instance = nullptr;
};

}
