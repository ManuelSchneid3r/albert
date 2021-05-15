// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QAbstractListModel>
#include <vector>
#include "albert/pluginprovider.h"

class PluginListModel : public QAbstractListModel {
public:
    PluginListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;

    std::vector<Core::PluginSpec*> plugin_specs;

private:
    void updatePlugins();

};
