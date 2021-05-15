// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QObject>
#include <QAbstractItemModel>
#include <memory>
#include <list>

class QueryExecution;

class QueryManager final : public QObject {
    Q_OBJECT
public:
    QueryManager(QObject *parent = 0);
    ~QueryManager();

    void setupSession();
    void teardownSession();
    void startQuery(const QString &searchTerm);

    bool incrementalSort();
    void setIncrementalSort(bool value);

private:
    void updateScores();

    std::list<QueryExecution*> pastQueries_;
    bool incrementalSort_;
    std::map<QString,uint> scores_;
    std::map<QString, unsigned long long> handlerIds_;
    unsigned long long lastQueryId_;

signals:

    void resultsReady(QAbstractItemModel*);
};
