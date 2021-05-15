// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <memory>
#include <QStackedLayout>
#include <QVBoxLayout>

class PluginListModel;

class PluginWidget : public QWidget {
public:
    PluginWidget(QWidget *parent = nullptr);
    void updatePluginWidget();
    void onPluginDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    QLabel *buildPlaceholder(const QString &msg);

    std::unique_ptr<PluginListModel> model;
    QHBoxLayout *hlayout;
    QListView *list_view;
    QVBoxLayout *vlayout;
    QLabel *label_plugin_title;
};
