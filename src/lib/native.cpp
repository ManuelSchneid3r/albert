// Copyright (C) 2014-2021 Manuel Schneider
#include "native.h"

Core::PluginSpec *Core::Native::DefaultConstructiblePlugin::ctor_spec = nullptr;
std::mutex Core::Native::DefaultConstructiblePlugin::ctor_spec_mutex;


Core::Native::DefaultConstructiblePlugin::DefaultConstructiblePlugin()
    : Core::PluginInstance(*ctor_spec)
{
    ctor_spec = nullptr;
}
