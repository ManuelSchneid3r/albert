// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include "ui_settingswidget.h"

namespace GlobalShortcut { class HotkeyManager; }
namespace Core { class ExtensionManager; }
class FrontendManager;
class QueryManager;
class MainWindow;
class TrayIcon;

class SettingsWidget final : public QWidget {
public:
    SettingsWidget(QueryManager *, GlobalShortcut::HotkeyManager *, TrayIcon *);

private:

    void keyPressEvent(QKeyEvent * event) override;
    void closeEvent(QCloseEvent * event) override;
    void onPluginDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void changeHotkey(int);
    void updatePluginInformations(const QModelIndex & curr);

    QueryManager *queryManager_;
    GlobalShortcut::HotkeyManager *hotkeyManager_;
    TrayIcon *trayIcon_;
    Ui::SettingsWidget ui;

};
