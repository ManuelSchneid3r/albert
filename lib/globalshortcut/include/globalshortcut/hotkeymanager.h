// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QObject>
#include <QSet>
#include <memory>
#include "globalshortcut_globals.h"

namespace GlobalShortcut {

class HotkeyManagerPrivate;

class EXPORT_GLOBALSHORTCUT HotkeyManager final : public QObject
{
    Q_OBJECT

public:
    HotkeyManager(QObject *parent = 0);
    ~HotkeyManager();

    bool registerHotkey(const QString&);
    bool registerHotkey(const QKeySequence&);
    bool registerHotkey(const int);
    bool unregisterHotkey(const QString&);
    bool unregisterHotkey(const QKeySequence&);
    void unregisterHotkey(const int);
    QSet<int> hotkeys() {return hotkeys_;}
    void enable(){ setEnabled(true); }
    void disable(){ setEnabled(false); }
    void setEnabled(bool enabled = true){enabled_ = enabled;}
    bool isEnabled() const {return enabled_;}

private:
    void onHotkeyPressed();

    bool enabled_;
    QSet<int> hotkeys_;

    std::unique_ptr<HotkeyManagerPrivate> d;

signals:
	void hotKeyPressed();
};

}
