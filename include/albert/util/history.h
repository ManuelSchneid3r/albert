// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QObject>
#include <QStringList>
#include "../export.h"

namespace Core {

/**
 * @brief The History class. This is the common input line history backend for frontends.
 */
class EXPORT_CORE History final : public QObject
{
    Q_OBJECT

public:

    History(QObject *parent = nullptr);

    Q_INVOKABLE void add(QString str);
    Q_INVOKABLE QString next(const QString &substring = QString{});
    Q_INVOKABLE QString prev(const QString &substring = QString{});
    Q_INVOKABLE void resetIterator();

private:

    QStringList lines_;
    int currentLine_;

};

}
