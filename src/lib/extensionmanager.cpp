// Copyright (C) 2014-2021 Manuel Schneider

#include "extensionmanager.h"
using namespace Core;

ExtensionManager *ExtensionManager::Global = nullptr;

struct ExtensionManager::Private {
    std::set<Extension*> sExtensions;
    std::mutex mutex;
};

Extension::~Extension()
{
}

ExtensionManager::ExtensionManager() : d(new Private)
{
}

ExtensionManager::~ExtensionManager()
{
}


const std::set<Extension *> &ExtensionManager::extensions()
{
    std::lock_guard<std::mutex> guard(d->mutex);
    return d->sExtensions;
}

void ExtensionManager::registerExtension(Extension *e)
{
    d->mutex.lock();
    d->sExtensions.emplace(e);
    d->mutex.unlock();
    emit extensionRegistered(e);
}

void ExtensionManager::unregisterExtension(Extension *e)
{
    d->mutex.lock();
    d->sExtensions.erase(e);
    d->mutex.unlock();
    emit extensionUnregistered(e);
}
