// Copyright (C) 2014-2021 Manuel Schneider

#include <QApplication>
#include <QDirIterator>
#include <QRegularExpression>
#include <chrono>
#include <exception>
#include <algorithm>
#include "albert/native.h"
#include "logging.h"
#include "nativepluginprovider.h"
using namespace Core;
using namespace std::chrono;
using namespace std;

namespace {
const char* CFG_FRONTEND_ID = "frontendId";
}

NativePluginProvider::NativePluginProvider(const QStringList &paths)
{
    for (const QString &pluginDir : paths) {
        QDirIterator dirIterator(pluginDir, QDir::Files);
        while (dirIterator.hasNext()) {
            auto path = dirIterator.next();

            // Check the sanity of the shared library and its interface

            QPluginLoader loader(path);
            if (!loader.metaData().contains("IID")) {
                DEBG << "No Qt plugin:" << path;
                continue;
            }

            auto iid_match = QRegularExpression(R"R(org.albert.plugininterface/(\d+).(\d+))R")
                    .match(loader.metaData()["IID"].toString());

            if (!iid_match.hasMatch()){
                WARN << QString("Invalid IID pattern: '%1'. Expected '%2'. (%3)")
                        .arg(iid_match.captured(), iid_match.regularExpression().pattern(), path);
                continue;
            }

            // From here on subject is at least an albert plugin, i.e. show

            QJsonObject &&metadata = loader.metaData()["MetaData"].toObject();

            PluginSpec::LoadType load_type = PluginSpec::LoadType::User;
            if (auto json_load_type = metadata.value("load_type"); !json_load_type.isUndefined()) {
                if (auto string_load_type = json_load_type.toString(); string_load_type == "on_demand")
                    load_type = PluginSpec::LoadType::OnDemand;
                else if (string_load_type == "frontend")
                    load_type = PluginSpec::LoadType::Frontend;
                else if (string_load_type == "user")
                    load_type = PluginSpec::LoadType::User;
                else
                    WARN << "Invalid metadata load_type. Defaulting to 'user'.";
            }

            unique_ptr<PluginSpec> spec{new PluginSpec{
                *this,
                path,
                metadata.value("id").toString(),
                metadata.value("version").toString(),
                metadata.value("name").toString(),
                metadata.value("authors").toVariant().toStringList(),
                metadata.value("maintainers").toVariant().toStringList(),
                metadata.value("plugin_dependencies").toVariant().toStringList(),
                metadata.value("library_dependencies").toVariant().toStringList(),
                metadata.value("executable_dependencies").toVariant().toStringList(),
                load_type,
                metadata.value("enabled_by_default").toBool(false),
                PluginSpec::State::Invalid,
                QString(),
                nullptr
            }};

            auto plugin_iid_major = iid_match.captured(1).toUInt();
            if (plugin_iid_major != ALBERT_MAJOR_VERSION)
                spec->reason = QString("Incompatible major version: %1. Expected: %2.")
                        .arg(plugin_iid_major).arg(ALBERT_MAJOR_VERSION);

            auto plugin_iid_minor = iid_match.captured(2).toUInt();
            if (plugin_iid_minor > ALBERT_MINOR_VERSION)
                spec->reason = QString("Incompatible minor version: %1. Supported up to: %2.")
                        .arg(plugin_iid_minor).arg(ALBERT_MINOR_VERSION);

            if (!QRegularExpression("[a-z0-9_]").match(spec->id).hasMatch())
                spec->reason = "Invalid id. Use [a-z0-9_].";

            else if (!QRegularExpression("^\\d+\\.\\d+$").match(spec->version).hasMatch())
                spec->reason = "Invalid version scheme. Use '<version>.<patch>'.";

            else if (spec->name.isNull())
                spec->reason = "Pretty printed name must not be empty.";

            else
                spec->state = PluginSpec::State::Ready;

            if (!spec->reason.isNull())
                WARN << spec->reason << path;

            if (pluginIndex_.count(spec->id))
                WARN << "Extension IDs already exists. Skipping:" << spec->path;
            else {
                DEBG << "Found plugin" << spec->id << spec->version << spec->path;

                pluginIndex_.emplace(spec->id, *spec);
                if (spec->load_type == PluginSpec::LoadType::Frontend)
                    frontendIndex_.emplace(spec->id, *spec);
                plugins_.emplace_back(move(spec));
            }
        }
    }

    INFO << "Loading plugins…";
    reloadEnabledPlugins();

    if (frontendIndex_.empty())
        qFatal("No frontends available");

    // Find the configured frontend, fallback to default if not configured
    QString id = QSettings(qApp->applicationName()).value(CFG_FRONTEND_ID).toString();
    if (!id.isNull()){
        try {
            auto &spec = frontendIndex_.at(id);
            if (NativePluginProvider::load(id); spec.state == PluginSpec::State::Loaded)
                return;
            CRIT << "Configured frontend"<<id<<"failed loading.";
            NativePluginProvider::load(id);
        }  catch (std::out_of_range) {
            CRIT << "Configured frontend"<<id<<"not found.";
        }
    } else {
        CRIT << "No frontend configured.";
    }
    CRIT << "Try until the first one which loads.";
    for (const auto &[id, spec] : frontendIndex_)
        if (NativePluginProvider::load(id); spec.state == PluginSpec::State::Loaded){
            QSettings(qApp->applicationName()).setValue(CFG_FRONTEND_ID, id);
            INFO << "Now setup to use"<<id;
            return;
        } else
            CRIT << "Falling back to"<<id<<"failed.";

    qFatal("Frontend initialization failed.");
}


NativePluginProvider::~NativePluginProvider()
{
    for (auto &spec : plugins_)
        NativePluginProvider::unload(spec->id);
}


vector<PluginSpec*> NativePluginProvider::plugins() const
{
    vector<PluginSpec*> result;
    transform(plugins_.cbegin(), plugins_.cend(), back_inserter(result),
              [](const auto &spec){ return spec.get(); });
    return result;
}


void NativePluginProvider::load(const QString &id)
{
    auto &spec = pluginIndex_.at(id);
    switch (lock_guard lock(loader_mutex); spec.state) {
    case PluginSpec::State::Error:
        [[fallthrough]];
    case PluginSpec::State::Ready:
    {
        DEBG << "Loading plugin" << spec.id;
        spec.reason = QString();
        spec.state = PluginSpec::State::Loading;
        emit pluginStateChanged(spec);

        // This code may run in parallel so dont touch the spec while loading
        QPluginLoader loader(spec.path);

        // Some python libs do not link against python. Export the python symbols to the main app.
        // loader_.setLoadHints(QLibrary::ExportExternalSymbolsHint); TODO: WTF?!?!?!

        QString reason;
        try {
            // Load the factory, hack the default constructability
            const std::lock_guard<std::mutex> lock(Core::Native::DefaultConstructiblePlugin::ctor_spec_mutex);
            Core::Native::DefaultConstructiblePlugin::ctor_spec = &spec;
            auto start = system_clock::now();
            QObject *instance = loader.instance();
            if (instance)
                if (spec.instance = dynamic_cast<Core::PluginInstance*>(instance); spec.instance) {
                    auto dur_ms = duration_cast<milliseconds>(system_clock::now()-start).count();
                    INFO << "Successfully loaded plugin" << spec.id << "in" << dur_ms << "ms";
                    ExtensionManager::Global->registerExtension(this);
                } else {
                    reason = "Plugin instance is not a Core::Native::Plugin.";
                    loader.unload();
                }
            else
                reason = loader.errorString();
        } catch (const exception& ex) {
            reason = ex.what();
        } catch (const string& s) {
            reason = QString::fromStdString(s);
        } catch (const QString& s) {
            reason = s;
        } catch (const char *s) {
            reason = s;
        } catch (...) {
            reason = "Unkown exception.";
        }

        if (reason.isNull())
            spec.state = PluginSpec::State::Loaded;
        else {
            spec.reason = reason;
            WARN << "Failed loading plugin:" << spec.reason << spec.path;
            spec.state = PluginSpec::State::Error;
        }
        emit pluginStateChanged(spec);
        return;
    }
    case PluginSpec::State::Loading:
        throw runtime_error("Tried to load a loading plugin.");  // Should never happen
    case PluginSpec::State::Loaded:
        throw runtime_error("Tried to load a loaded plugin.");  // Should never happen
    case PluginSpec::State::Invalid:
        throw runtime_error("Tried to load an invalid plugin.");  // Should never happen
    }
}


void NativePluginProvider::unload(const QString &id)
{
    // Never _really_ unload a plugin, since otherwise all objects instanciated
    // by this extension (items, widgets, etc) and spread all over the app
    // would have to be deleted. This is a lot of work and nobody cares about
    // that little amount of extra KBs in RAM until next restart.
    auto &spec = pluginIndex_.at(id);
    switch (lock_guard lock(loader_mutex); spec.state) {
    case PluginSpec::State::Error:
        throw runtime_error("Tried to load an unloaded (state error) plugin.");  // Should never happen
    case PluginSpec::State::Ready:
        throw runtime_error("Tried to load an unloaded plugin.");  // Should never happen
    case PluginSpec::State::Loading:
        throw runtime_error("Tried to load a loading plugin.");  // Should never happen
    case PluginSpec::State::Loaded:
        {
            ExtensionManager::Global->unregisterExtension(this);

            system_clock::time_point start = system_clock::now();
            delete QPluginLoader(spec.path).instance();
            DEBG << QString("%1 unloaded in %2 milliseconds").arg(spec.id)
                            .arg(duration_cast<milliseconds>(system_clock::now()-start).count());

            spec.instance = nullptr;
            spec.reason.clear();
            spec.state = PluginSpec::State::Ready;
            emit pluginStateChanged(spec);
            return;
        }
    case PluginSpec::State::Invalid:
        throw runtime_error("Tried to unload an invalid plugin.");  // Should never happen
    }
}
