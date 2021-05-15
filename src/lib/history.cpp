// Copyright (C) 2014-2021 Manuel Schneider

#include <QStringList>
#include <QVariant>
#include <QSqlQuery>
#include "history.h"


Core::History::History(QObject *parent) : QObject(parent) {
    QSqlQuery query("SELECT input "
                    "FROM activation a JOIN  query q ON a.query_id = q.id "
                    "GROUP BY input  "
                    "ORDER BY max(timestamp) DESC;");
    while (query.next())
        lines_.append(query.value(0).toString());

    currentLine_ = -1; // This means historymode is not active
}



void Core::History::add(QString str) {
    if (!str.isEmpty()){
        if (lines_.contains(str))
            lines_.removeAll(str); // Remove dups
        lines_.prepend(str);
    }
    resetIterator();
}



QString Core::History::next(const QString &substring) {
    int newCurrentLine = currentLine_;
    while (++newCurrentLine < static_cast<int>(lines_.size())){
        const QString &line = lines_[newCurrentLine];
        if (line.contains(substring, Qt::CaseInsensitive)){
            currentLine_ = newCurrentLine;
            return line;
        }
    }
    return QString{};
}



QString Core::History::prev(const QString &substring) {
    int newCurrentLine = currentLine_;
    while (-1 < --newCurrentLine){
        const QString &line = lines_[newCurrentLine];
        if (line.contains(substring, Qt::CaseInsensitive)){
            currentLine_ = newCurrentLine;
            return line;
        }
    }
    return QString{};
}



void Core::History::resetIterator() {
    currentLine_ = -1;
}
