// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QStringList>
#include <QSettings>

namespace XDG {

class ThemeFileParser
{
public:

    ThemeFileParser(const QString &iniFile);

    QString path();
    QString name();
    QString comment();
    QStringList inherits();
    QStringList directories();
    bool hidden();
    int size(const QString& directory);
    QString context(const QString& directory);
    QString type(const QString& directory);
    int maxSize(const QString& directory);
    int minSize(const QString& directory);
    int threshold(const QString& directory);

private:

    QSettings iniFile_;

};

}
