// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QStringList>
#include <map>
#include "xdg_globals.h"

namespace XDG {

class EXPORT_XDG IconLookup
{
public:

    /**
     * @brief iconPath Does XDG icon lookup for the given icon name
     * @param iconName The icon name to lookup
     * @param themeName The theme to use, use current theme if empty
     * @return If an icon was found the path to the icon, else an empty string
     */
    static QString iconPath(QString iconName, QString themeName = QString());

    /**
     * @brief iconPath Does XDG icon lookup for the given icon names, stops on success
     * @param iconNames A list of icon names to lookup
     * @param themeName The theme to use, use current theme if empty
     * @return If one of the icons was found the path to the icon, else an empty string
     */
    static QString iconPath(QStringList iconNames, QString themeName = QString());

private:

    IconLookup();
    static IconLookup *instance();

    QString themeIconPath(QString iconName, QString themeName = QString());
    QString doRecursiveIconLookup(const QString &iconName, const QString &theme, QStringList *checked);
    QString doIconLookup(const QString &iconName, const QString &themeFile);
    QString lookupThemeFile(const QString &themeName);

    QStringList iconDirs_;
    std::map<QString, QString> iconCache_;
};

}
