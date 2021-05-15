// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QObject>
#include <QMenu>
#include <QSystemTrayIcon>

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:

    TrayIcon();

    void setVisible(bool = true);

signals:

    void stateChanged(bool);
};
