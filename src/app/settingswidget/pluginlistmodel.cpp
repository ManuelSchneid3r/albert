// Copyright (C) 2014-2021 Manuel Schneider

#include <QIcon>
#include <algorithm>
#include "albert/pluginspec.h"
#include "albert/pluginprovider.h"
#include "albert/extensionmanager.h"
#include "pluginlistmodel.h"
using namespace std;
using namespace Core;


PluginListModel::PluginListModel(QObject *parent) : QAbstractListModel(parent)
{
    // On new PPs: update plugins and reset view on pluginStateChanged
    connect(ExtensionManager::Global, &Core::ExtensionManager::extensionRegistered,
            this,[this](Extension *e){
        if (auto * pp = dynamic_cast<PluginProvider*>(e); pp) {
            connect(pp, &PluginProvider::pluginStateChanged,
                    this, [this](){endResetModel();});
            updatePlugins();
        }
    });

    // On removed PPs: update plugins and reset view on pluginStateChanged
    connect(ExtensionManager::Global, &Core::ExtensionManager::extensionRegistered,
            this,[this](Extension *e){
        if (auto * pp = dynamic_cast<PluginProvider*>(e); pp) {
            disconnect(pp, &PluginProvider::pluginStateChanged,
                       this, nullptr);
            updatePlugins();
        }
    });

    updatePlugins();
}


void PluginListModel::updatePlugins()
{
    plugin_specs.clear();
    for (auto *pp : ExtensionManager::Global->extensionsOfType<PluginProvider>())
        for (auto *p : pp->plugins())
            plugin_specs.emplace_back(p);

    std::sort(plugin_specs.begin(), plugin_specs.end(),
              [](auto *lhs, auto *rhs) { return lhs->name < rhs->name; });
}


int PluginListModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(plugin_specs.size());
}


QVariant PluginListModel::data(const QModelIndex &index, int role) const {
    try {
        const auto *p = plugin_specs.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return p->name;
        case Qt::ToolTipRole:{
            QString toolTip;
    //        toolTip = QString("ID: %1\nVersion: %2\nAuthor: %3\n").arg(p->id(), p->version(), p->authors().join(", "));
    //        if (!p->reason.isEmpty())
    //            toolTip.append(QString("Error: %1\n").arg(p->reason()));
    //        if (!p->plugin_dependencies.empty())
    //            toolTip.append(QString("Dependencies: %1\n").arg(p->dependencies().join(", ")));
    //        toolTip.append(QString("Path: %1").arg(p->path()));
            return toolTip;
        }
        case Qt::DecorationRole:
            switch (p->state) {
            case PluginSpec::State::Invalid:
                return QIcon();
            case PluginSpec::State::Loaded:
                return QIcon(":plugin_loaded");
            case PluginSpec::State::Loading:
                return QIcon(":plugin_loading");
            case PluginSpec::State::Ready:
                return QIcon(":plugin_notloaded");
            case PluginSpec::State::Error:
                return QIcon(":plugin_error");
            }
        case Qt::CheckStateRole:
            return p->provider.isEnabled(p->id) ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }

    }  catch (std::out_of_range) {
        return QVariant();
    }
}


bool PluginListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    try {
        const auto *p = plugin_specs.at(index.row());
        switch (role) {
        case Qt::CheckStateRole:
            p->provider.setEnabled(p->id, value == Qt::Checked);
            dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        default:
            return false;
        }
    }  catch (std::out_of_range) {
        return false;
    }
}


Qt::ItemFlags PluginListModel::flags(const QModelIndex &index) const
{
    try {
        const auto *p = plugin_specs.at(index.row());

        if (p->load_type != PluginSpec::LoadType::User)
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;

        switch (p->state) {
        case PluginSpec::State::Invalid:
            return Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
        case PluginSpec::State::Error:
            [[fallthrough]];
        case PluginSpec::State::Loaded:
            [[fallthrough]];
        case PluginSpec::State::Loading:
            [[fallthrough]];
        case PluginSpec::State::Ready:
            return Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
        }
    }  catch (std::out_of_range) {
        return 0;
    }
}
