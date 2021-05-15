// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QObject>
#include <mutex>
#include <memory>
#include <set>
#include "export.h"

int main(int argc, char **argv);

namespace Core {

class EXPORT_CORE Extension {
public:
    virtual ~Extension();
    virtual QString id() const = 0;
};


class EXPORT_CORE ExtensionManager final : public QObject {
    Q_OBJECT
public:
    void registerExtension(Extension *e);
    void unregisterExtension(Extension *e);
    const std::set<Extension*> &extensions();

    template<typename T>
    std::set<T*> extensionsOfType()
    {
        std::set<T*> results;
        T* t;
        for (decltype(auto) e : extensions()){
            t = dynamic_cast<T*>(e);
            if (t)
                results.insert(t);
        }
        return results;
    }

    static ExtensionManager *Global;

private:
    ExtensionManager();
    ~ExtensionManager();

    struct Private;
    std::unique_ptr<Private> d;

    friend int ::main(int argc, char **argv);

signals:
    void extensionRegistered(Extension*);
    void extensionUnregistered(Extension*);
};

}

