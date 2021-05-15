// Copyright (C) 2014-2021 Manuel Schneider

#include "globalshortcut/hotkeymanager.h"
#if defined __linux__ || defined __FreeBSD__
#include "hotkeymanager_x11.h"
#elif defined __APPLE__
#elif defined _WIN32
#include "hotkeymanager_win.h"
#endif
#include <QKeySequence>


GlobalShortcut::HotkeyManager::HotkeyManager(QObject *parent) :
    QObject(parent), d(new HotkeyManagerPrivate) {
    connect(d.get(), &HotkeyManagerPrivate::hotKeyPressed, this, &HotkeyManager::onHotkeyPressed);
    enabled_ = true;
}


GlobalShortcut::HotkeyManager::~HotkeyManager() {

}


bool GlobalShortcut::HotkeyManager::registerHotkey(const QString &hk) {
	return registerHotkey(QKeySequence(hk));
}


bool GlobalShortcut::HotkeyManager::registerHotkey(const QKeySequence &hk) {
	if (hk.count() != 1)
		return false;
	return registerHotkey(hk[0]);
}


bool GlobalShortcut::HotkeyManager::registerHotkey(const int hk) {
    if (hotkeys_.contains(hk))
        return true;
    if (d->registerNativeHotkey(hk)) {
        hotkeys_.insert(hk);
		return true;
	}
    return false;
}


bool GlobalShortcut::HotkeyManager::unregisterHotkey(const QString &hk) {
    return unregisterHotkey(QKeySequence(hk));
}


bool GlobalShortcut::HotkeyManager::unregisterHotkey(const QKeySequence &hk) {
    if (hk.count() != 1)
        return false;
    unregisterHotkey(hk[0]);
    return true;
}


void GlobalShortcut::HotkeyManager::unregisterHotkey(const int hk) {
    if (!hotkeys_.contains(hk))
        return;
    d->unregisterNativeHotkey(hk);
    hotkeys_.remove(hk);
}


void GlobalShortcut::HotkeyManager::onHotkeyPressed() {
    if (enabled_)
		emit hotKeyPressed();
}

