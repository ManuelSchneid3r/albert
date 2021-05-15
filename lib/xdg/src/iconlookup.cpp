// Copyright (C) 2014-2021 Manuel Schneider

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QSettings>
#include <QString>
#include "themefileparser.h"
#include "iconlookup.h"
using namespace std;

namespace  {
    QStringList icon_extensions = {"png", "svg", "xpm"};
}



QString XDG::IconLookup::iconPath(QStringList iconNames, QString themeName) {
    QString result;
    for ( const QString &iconName : iconNames )
        if ( !(result = instance()->themeIconPath(iconName, themeName)).isNull() )
            return result;
    return QString();
}



QString XDG::IconLookup::iconPath(QString iconName, QString themeName){
    return instance()->themeIconPath(iconName, themeName);
}



XDG::IconLookup::IconLookup()
{
    /*
     * Icons and themes are looked for in a set of directories. By default,
     * apps should look in $HOME/.icons (for backwards compatibility), in
     * $XDG_DATA_DIRS/icons and in /usr/share/pixmaps (in that order).
     */

    QString path = QDir::home().filePath(".icons");
    if (QFile::exists(path))
        iconDirs_.append(path);

    for (const QString &basedir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
        if (QFile::exists(path = QDir(basedir).filePath("icons")))
            iconDirs_.append(path);

    path = "/usr/share/pixmaps";
    if (QFile::exists(path))
        iconDirs_.append(path);
}




XDG::IconLookup *XDG::IconLookup::instance()
{
    static IconLookup *instance_ = nullptr;
    if (!instance_){
        qInfo() << "Systems icon theme is:" << QIcon::themeName();
        instance_ = new IconLookup();
    }
    return instance_;
}




QString XDG::IconLookup::themeIconPath(QString iconName, QString themeName){

    if (themeName.isEmpty())
        themeName = QIcon::themeName();

    // if we have an absolute path, just return it
    if ( iconName[0]=='/' ){
        if ( QFile::exists(iconName) )
            return iconName;
        else
            return QString();
    }

    // check if it has an extension and strip it
    for (const QString &ext : icon_extensions)
        if (iconName.endsWith(QString(".").append(ext)))
            iconName.chop(4);

    // Check cache
    try {
        return iconCache_.at(iconName);
    }  catch (out_of_range) { }

    QStringList checkedThemes;
    QString iconPath;

    // Lookup themefile
    if (!(iconPath = doRecursiveIconLookup(iconName, themeName, &checkedThemes)).isNull())
        return iconCache_.emplace(iconName, iconPath).first->second;

    // Lookup in hicolor
    if (!checkedThemes.contains("hicolor"))
        if (!(iconPath = doRecursiveIconLookup(iconName, "hicolor", &checkedThemes)).isNull())
            return iconCache_.emplace(iconName, iconPath).first->second;

    // Now search unsorted
    for (const QString &iconDir : iconDirs_)
        for (const QString &ext : icon_extensions)
            if (QFile(iconPath = QString("%1/%2.%3").arg(iconDir, iconName, ext)).exists())
                return iconCache_.emplace(iconName, iconPath).first->second;

    // Nothing found, save though to avoid repeated expensive lookups
    return iconCache_.emplace(iconName, QString()).first->second;
}




QString XDG::IconLookup::doRecursiveIconLookup(const QString &iconName, const QString &themeName, QStringList *checked){

    // Exlude multiple scans
    if (checked->contains(themeName))
        return QString();
    checked->append(themeName);

    // Check if theme exists
    QString themeFile = lookupThemeFile(themeName);
    if (themeFile.isNull())
        return QString();

    // Check if icon exists
    QString iconPath;
    iconPath = doIconLookup(iconName, themeFile);
    if (!iconPath.isNull())
        return iconPath;

    // Check its parents too
    for (const QString &parent : ThemeFileParser(themeFile).inherits()){
        iconPath = doRecursiveIconLookup(iconName, parent, checked);
        if (!iconPath.isNull())
            return iconPath;
    }

    return QString();
}




QString XDG::IconLookup::doIconLookup(const QString &iconName, const QString &themeFile) {

    ThemeFileParser themeFileParser(themeFile);
    QDir themeDir = QFileInfo(themeFile).dir();
    QString themeName = themeDir.dirName();

    // Get the sizes of the dirs
    vector<pair<QString, int>> dirsAndSizes;
    for (const QString &subdir : themeFileParser.directories())
        dirsAndSizes.push_back(make_pair(subdir, themeFileParser.size(subdir)));

    // Sort them by size
    sort(dirsAndSizes.begin(), dirsAndSizes.end(),
              [](pair<QString, int>  a, pair<QString, int> b) {
                  return a.second > b.second;
              });

    // Well now search for a file beginning with the greatest
    QString filename;
    QFile file;
    for (const auto &dirAndSize : dirsAndSizes){
        for (const QString &iconDir : iconDirs_){
            for (const QString &ext : icon_extensions){
                filename = QString("%1/%2/%3/%4.%5").arg(iconDir, themeName, dirAndSize.first, iconName, ext);
                if (file.exists(filename)){
                    return filename;
                }
            }
        }
    }

    return QString();
}



QString XDG::IconLookup::lookupThemeFile(const QString &themeName)
{
    // Lookup themefile
    for (const QString &iconDir : iconDirs_){
        QString indexFile = QString("%1/%2/index.theme").arg(iconDir, themeName);
        if (QFile(indexFile).exists())
            return indexFile;
    }
    return QString();
}
