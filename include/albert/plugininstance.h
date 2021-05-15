// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QDir>
#include <QSettings>
#include <memory>
#include "extensionmanager.h"


class QWidget;
namespace Core {
struct PluginSpec;
class EXPORT_CORE PluginInstance : virtual public Extension {
public:
    virtual ~PluginInstance();

    PluginSpec &spec();

    /**
     * @brief id
     * This is the global unique identifier of the plugin
     * @return
     */
    QString id() const override;

    /**
     * @brief The settings widget factory
     * This has to return the widget that is accessible to the user from the
     * albert settings plugin tab. If the return value is a nullptr there will
     * be no settings widget available in the settings.
     * @return The settings widget
     */
    virtual QWidget* widget(QWidget *parent = nullptr);

    /**
     * @brief cacheLocation
     * @return The recommended cache location for the plugin.
     * @note If the dir does not exist it will be crated.
     */
    QDir cacheLocation() const;

    /**
     * @brief configLocation
     * @return The recommended config location for the plugin.
     * @note If the dir does not exist it will be crated.
     */
    QDir configLocation() const;

    /**
     * @brief dataLocation
     * @return The recommended data location for the plugin
     * @note If the dir does not exist it will be crated.
     */
    QDir dataLocation() const;

    /**
     * @brief settings
     * This is a convenience function returning the default settings object for this plugin. It is
     * located in the config location defined in this plugin and has the basename "config". Note
     * that QSettings is not thread-safe, so if you want to access the settings concurrently you
     * should create a new instance with the path of this object.
     * @return The settings object
     */
    std::unique_ptr<QSettings> settings();


protected:

    PluginInstance(PluginSpec &spec);

    struct Private;
    std::unique_ptr<Private> d;

};
}
