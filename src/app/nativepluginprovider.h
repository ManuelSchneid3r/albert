// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QStringList>
#include <QJsonObject>
#include <vector>
#include <memory>
#include <mutex>
#include "albert/native.h"
#include "albert/pluginspec.h"


class NativePluginProvider : public Core::PluginProvider {
public:
    NativePluginProvider(const QStringList &paths);
    ~NativePluginProvider();


private:
    // PluginProvider interface
    QString id() const override { return "albert_plugins"; };
    std::vector<Core::PluginSpec*> plugins() const override;
    void load(const QString &id) override;
    void unload(const QString &id) override;

    std::vector<std::unique_ptr<Core::PluginSpec>> plugins_;
    std::map<QString, Core::PluginSpec&> pluginIndex_;
    std::map<QString, Core::PluginSpec&> frontendIndex_;

    std::mutex loader_mutex;
};

