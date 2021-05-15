// Copyright (C) 2021 Manuel Schneider

#pragma once
#include <QObject>
#include "export.h"
#include "extensionmanager.h"

namespace Core {

struct PluginSpec;

class EXPORT_CORE PluginProvider : public QObject, virtual public Extension {
    Q_OBJECT
public:
    virtual ~PluginProvider() {}
    void setEnabled(const QString &id, bool enabled = true);
    bool isEnabled(const QString &id);

    virtual std::vector<Core::PluginSpec*> plugins() const  = 0;

protected:
    void reloadEnabledPlugins();

    virtual void load(const QString &id) = 0;
    virtual void unload(const QString &id) = 0;

signals:
    void pluginStateChanged(const PluginSpec&);
};

}
