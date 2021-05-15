// Copyright (C) 2014-2021 Manuel Schneider
#pragma once
#include "config.h"
#include "plugininstance.h"
#include "frontend.h"
#include "pluginprovider.h"

class NativePluginProvider;

namespace Core {
namespace Native {

class EXPORT_CORE DefaultConstructiblePlugin : public Core::PluginInstance {
protected:
    DefaultConstructiblePlugin();
private:
    static PluginSpec *ctor_spec;
    static std::mutex ctor_spec_mutex;
    friend class ::NativePluginProvider;
};

class EXPORT_CORE PluginInstance : public QObject, public DefaultConstructiblePlugin {};

class EXPORT_CORE PluginProvider : public Core::PluginProvider, public DefaultConstructiblePlugin {};

class EXPORT_CORE Frontend : public Core::Frontend, public DefaultConstructiblePlugin {};

}
}

#define ALBERT_PLUGIN(metadata) Q_PLUGIN_METADATA(IID ALBERT_IID FILE metadata)
