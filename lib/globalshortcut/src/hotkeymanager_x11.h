// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QDebug>
#include <set>
#include <QAbstractNativeEventFilter>

struct _XDisplay;
typedef _XDisplay Display;

namespace GlobalShortcut {

class HotkeyManagerPrivate final : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:

    HotkeyManagerPrivate(QObject* parent = 0);
    ~HotkeyManagerPrivate();

    bool registerNativeHotkey(quint32 hk);
    void unregisterNativeHotkey(quint32 hk);

private:

    bool nativeEventFilter(const QByteArray&, void*, long*) override;
    std::set<uint> nativeKeycodes(uint QtKey);
    uint nativeModifiers(uint QtKbdMods);

    std::set<std::pair<uint,uint>> grabbedKeys;
    std::set<uint> offendingMasks;

    struct Masks {
        unsigned int alt;
        unsigned int meta;
        unsigned int numlock;
        unsigned int super;
        unsigned int hyper;
        unsigned int iso_level3_shift;
    } masks;

signals:

     void hotKeyPressed();
};

}
