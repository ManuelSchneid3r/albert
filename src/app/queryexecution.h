// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QAbstractListModel>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>
#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include "albert/query.h"
namespace Core {
class QueryHandler;
class FallbackProvider;
class Extension;
class Item;
}

struct QueryStatistics {
    QString input;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    std::map<QString, uint> runtimes;
    bool cancelled = false;
    QString activatedItem;
};


/**
 * @brief The QueryExecution class
 * Represents the execution of a query
 */
class QueryExecution : public QAbstractListModel {
    Q_OBJECT
public:

    enum class State { Idle, Running, Finished };

    QueryExecution(const QString &queryString, std::map<QString,uint> scores, bool fetchIncrementally);
    ~QueryExecution() override;

    const State &state() const;

    const Core::Query *query();

    void run();
    void cancel();

    // Model interface
    int rowCount(const QModelIndex &) const override;
    QHash<int,QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool canFetchMore(const QModelIndex &) const override;
    void fetchMore(const QModelIndex &) override;

    QueryStatistics stats;

private:

    void setState(State state);

    void runBatchHandlers();
    void onBatchHandlersFinished();
    void runRealtimeHandlers();
    void onRealtimeHandlersFinsished();
    void insertPendingResults();

    bool isValid_ = true;

    Core::Query query_;
    State state_;

    std::set<Core::QueryHandler*> batchHandlers_;
    std::set<Core::QueryHandler*> realtimeHandlers_;

    mutable std::vector<std::pair<std::shared_ptr<Core::Item>, uint>> results_;
    mutable std::vector<std::pair<std::shared_ptr<Core::Item>, uint>> fallbacks_;
    mutable int sortedItems_ = 0;
    bool fetchIncrementally_ = false;

    QTimer fiftyMsTimer_;

    QFuture<std::pair<Core::QueryHandler*,uint>> future_;
    QFutureWatcher<std::pair<Core::QueryHandler*,uint>> futureWatcher_;

signals:

    void resultsReady(QAbstractItemModel*);
    void stateChanged(State state);

};


