// Copyright (C) 2014-2021 Manuel Schneider

#include <QString>
#include <QVariant>
#include <QtConcurrent>
#include <algorithm>
#include <chrono>
#include <functional>
#include <utility>
#include "albert/extensionmanager.h"
#include "albert/action.h"
#include "albert/fallbackprovider.h"
#include "albert/item.h"
#include "albert/queryhandler.h"
#include "albert/util/itemroles.h"
#include "logging.h"
#include "queryexecution.h"
using namespace std;
using namespace chrono;
using namespace Core;

namespace {
    const int FETCH_SIZE = 20;
    struct MatchCompare {
        bool operator()(const pair<shared_ptr<Item>, uint> &lhs,
                        const pair<shared_ptr<Item>, uint> &rhs) {
            // Compare urgency then score
            if (lhs.first->urgency() != rhs.first->urgency())
                return lhs.first->urgency() < rhs.first->urgency();
            else if (lhs.second != rhs.second)
                return lhs.second > rhs.second; // Compare match score
            else
                return lhs.first->text().size() < rhs.first->text().size();
        }
    };
}


QueryExecution::QueryExecution(const QString &queryString, map<QString,uint> scores, bool fetchIncrementally)
{

    fetchIncrementally_ = fetchIncrementally;
    query_.rawString_ = queryString;
    query_.string_    = queryString;
    query_.scores_ = move(scores);
    stats.input = queryString;

    auto queryHandlers = Core::ExtensionManager::Global->extensionsOfType<QueryHandler>();

    // Get fallbacks
    if (!query_.string_.trimmed().isEmpty())
        for (auto *fallbackProvider : Core::ExtensionManager::Global->extensionsOfType<FallbackProvider>())
            for (shared_ptr<Item> & item : fallbackProvider->fallbacks(queryString))
                fallbacks_.emplace_back(move(item), 0);

    // Run with a single handler if the trigger matches
    for (auto *handler : queryHandlers) {
        for (const QString& trigger : handler->triggers()) {
            if (!trigger.isNull() && queryString.startsWith(trigger)) {
                query_.trigger_ = trigger;
                query_.string_ = queryString.mid(trigger.size());
                if (handler->executionType() == QueryHandler::ExecutionType::Batch)
                    batchHandlers_.insert(handler);
                else
                    realtimeHandlers_.insert(handler);
                return;
            }
        }
    }

    // Else run all batched handlers
    for (auto *handler : queryHandlers)
        if ( handler->executionType() == QueryHandler::ExecutionType::Batch)
            batchHandlers_.insert(handler);
}


QueryExecution::~QueryExecution()
{

}


const Query *QueryExecution::query()
{
    return &query_;
}


const QueryExecution::State &QueryExecution::state() const
{
    return state_;
}


void QueryExecution::setState(State state)
{
    state_ = state;
    if (state_ == State::Finished)
        stats.end = system_clock::now();
    emit stateChanged(state_);
}


void QueryExecution::run()
{
    setState(State::Running);

    stats.start = system_clock::now();

    if ( !batchHandlers_.empty() )
        return runBatchHandlers();

    emit resultsReady(this);

    if ( !realtimeHandlers_.empty() )
        return runRealtimeHandlers();

    setState(State::Finished);
}


void QueryExecution::cancel()
{
    futureWatcher_.disconnect();
    future_.cancel();
    query_.isValid_ = false;
    stats.cancelled = true;
}


void QueryExecution::runBatchHandlers()
{
    // Call onBatchHandlersFinished when all handlers finished
    connect(&futureWatcher_, &QFutureWatcher<pair<QueryHandler*,uint>>::finished,
            this, &QueryExecution::onBatchHandlersFinished);

    // Run the handlers concurrently and measure the runtimes
    function<pair<QueryHandler*,uint>(QueryHandler*)> func = [this](QueryHandler* queryHandler){
        system_clock::time_point start = system_clock::now();
        queryHandler->handleQuery(&query_);
        long duration = duration_cast<microseconds>(system_clock::now()-start).count();
        DEBG << qPrintable(QString("TIME: %1 µs MATCHES [%2]").arg(duration, 6).arg(queryHandler->id()));
        return make_pair(queryHandler, static_cast<int>(duration));
    };
    future_ = QtConcurrent::mapped(batchHandlers_.begin(), batchHandlers_.end(), func);
    futureWatcher_.setFuture(future_);
}


void QueryExecution::onBatchHandlersFinished()
{
    // Save the runtimes of the current future
    for ( auto it = future_.begin(); it != future_.end(); ++it )
        stats.runtimes.emplace(it->first->id(), it->second);

    // Move the items of the "pending results" into "results"
    query_.mutex_.lock();
    swap(query_.results_, results_);
    query_.mutex_.unlock();

    // Sort the results
    if (query_.trigger_.isNull() || query_.sort_){
        if ( fetchIncrementally_ ) {
            int sortUntil = min(sortedItems_ + FETCH_SIZE, static_cast<int>(results_.size()));
            partial_sort(results_.begin() + sortedItems_, results_.begin() + sortUntil,
                         results_.end(), MatchCompare());
            sortedItems_ = sortUntil;
        }
        else
            std::sort(results_.begin(), results_.end(), MatchCompare());
    }

    if ( realtimeHandlers_.empty() ){
        if( results_.empty() && !query_.isTriggered() && !query_.rawString_.isEmpty() ){
            results_ = fallbacks_;
            sortedItems_ = static_cast<int>(fallbacks_.size());
            fetchIncrementally_ = false;
        }
        setState(State::Finished);
    }
    else
        runRealtimeHandlers();

    emit resultsReady(this);
}


void QueryExecution::runRealtimeHandlers()
{
    // Call onRealtimeHandlersFinsished when all handlers finished
    disconnect(&futureWatcher_, &QFutureWatcher<pair<QueryHandler*,uint>>::finished,
               this, &QueryExecution::onBatchHandlersFinished);

    connect(&futureWatcher_, &QFutureWatcher<pair<QueryHandler*,uint>>::finished,
            this, &QueryExecution::onRealtimeHandlersFinsished);

    // Run the handlers concurrently and measure the runtimes
    function<pair<QueryHandler*,uint>(QueryHandler*)> func = [this](QueryHandler* queryHandler){
        system_clock::time_point start = system_clock::now();
        queryHandler->handleQuery(&query_);
        long duration = duration_cast<microseconds>(system_clock::now()-start).count();
        DEBG << qPrintable(QString("TIME: %1 µs MATCHES REALTIME [%2]").arg(duration, 6).arg(queryHandler->id()));
        return make_pair(queryHandler, static_cast<int>(duration));
    };
    future_ = QtConcurrent::mapped(realtimeHandlers_.begin(), realtimeHandlers_.end(), func);
    futureWatcher_.setFuture(future_);

    // Insert pending results every 50 milliseconds
    connect(&fiftyMsTimer_, &QTimer::timeout, this, &QueryExecution::insertPendingResults);
    fiftyMsTimer_.start(50);
}


void QueryExecution::onRealtimeHandlersFinsished()
{
    // Save the runtimes of the current future
    for ( auto it = future_.begin(); it != future_.end(); ++it )
        stats.runtimes.emplace(it->first->id(), it->second);

    // Finally done
    fiftyMsTimer_.stop();
    fiftyMsTimer_.disconnect();
    insertPendingResults();

    if( results_.empty() && !query_.isTriggered() && !query_.rawString_.isEmpty() ){
        beginInsertRows(QModelIndex(), 0, static_cast<int>(fallbacks_.size()-1));
        results_ = fallbacks_;
        endInsertRows();
        fetchIncrementally_ = false;
    }
    setState(State::Finished);
}


void QueryExecution::insertPendingResults()
{
    if(query_.results_.size()) {
        QMutexLocker lock(&query_.mutex_);

        // When fetching incrementally, only emit if this is in the fetched range
        if ( !fetchIncrementally_ || sortedItems_ == static_cast<int>(results_.size()) ){
            beginInsertRows(QModelIndex(),
                            static_cast<int>(results_.size()),
                            static_cast<int>(results_.size() + query_.results_.size() - 1));
            results_.reserve(results_.size() + query_.results_.size());
            move(query_.results_.begin(), query_.results_.end(), back_inserter(results_));
            endInsertRows();
        } else {
            results_.reserve(results_.size() + query_.results_.size());
            move(query_.results_.begin(), query_.results_.end(), back_inserter(results_));
        }
        query_.results_.clear();
    }
}


int QueryExecution::rowCount(const QModelIndex &) const
{
    return (query_.trigger_.isNull() || query_.sort_) && fetchIncrementally_ ? sortedItems_ : static_cast<int>(results_.size());
}


QHash<int,QByteArray> QueryExecution::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[static_cast<int>(ItemRoles::TextRole)]       = "itemTextRole";
    roles[static_cast<int>(ItemRoles::ToolTipRole)]    = "itemToolTipRole";
    roles[static_cast<int>(ItemRoles::DecorationRole)] = "itemDecorationRole";
    roles[static_cast<int>(ItemRoles::CompletionRole)] = "itemCompletionStringRole";
    roles[static_cast<int>(ItemRoles::ActionRole)]     = "itemActionRole"; // used to activate items as well
    roles[static_cast<int>(ItemRoles::AltActionRole)]  = "itemAltActionsRole";  // used to activate items as well
    roles[static_cast<int>(ItemRoles::FallbackRole)]   = "itemFallbackRole"; // used to activate items as well
    return roles;
}


QVariant QueryExecution::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const shared_ptr<Item> &item = results_[static_cast<size_t>(index.row())].first;

        switch ( role ) {
        case ItemRoles::TextRole:
            return item->text();
        case ItemRoles::ToolTipRole:
            return item->subtext();
        case ItemRoles::DecorationRole:
            return item->iconPath();
        case ItemRoles::CompletionRole:
            return item->completion();
        case ItemRoles::ActionRole:
            return (0 < static_cast<int>(item->actions().size()))
                    ? item->actions()[0]->text()
                    : item->subtext();
        case ItemRoles::AltActionRole: { // Actions list
            QStringList actionTexts;
            for (auto &action : item->actions())
                actionTexts.append(action->text());
            return actionTexts;
        }
        case ItemRoles::FallbackRole:
            return QString("Search '%1' using default fallback").arg(query_.rawString_);
        }
    }
    return QVariant();
}


bool QueryExecution::canFetchMore(const QModelIndex & /* index */) const
{
    if ((query_.trigger_.isNull() || query_.sort_) && fetchIncrementally_ && sortedItems_ < static_cast<int>(results_.size()))
        return true;
    else
        return false;
}


void QueryExecution::fetchMore(const QModelIndex & /* index */)
{
    int sortUntil = min(sortedItems_ + FETCH_SIZE, static_cast<int>(results_.size()));
    if (query_.trigger_.isNull() || query_.sort_)
        partial_sort(results_.begin() + sortedItems_,
                     results_.begin() + sortUntil,
                     results_.end(),
                     MatchCompare());
    beginInsertRows(QModelIndex(), sortedItems_, sortUntil-1);
    sortedItems_ = sortUntil;
    endInsertRows();
    sortedItems_ = sortUntil;
}


bool QueryExecution::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid()) {
        shared_ptr<Item> &item = results_[static_cast<size_t>(index.row())].first;
        switch ( role ) {
        case ItemRoles::ActionRole:{
            if (0U < item->actions().size()){
                item->actions()[0]->activate();
                stats.activatedItem = item->id();
            }
            break;
        }
        case ItemRoles::AltActionRole:{
            size_t actionValue = static_cast<size_t>(value.toInt());
            if (actionValue < item->actions().size()) {
                item->actions()[actionValue]->activate();
                stats.activatedItem = item->id();
            }
            break;
        }
        case ItemRoles::FallbackRole:{
            if (0U < fallbacks_.size() && 0U < fallbacks_[0].first->actions().size()) {
                fallbacks_[0].first->actions()[0]->activate();
                stats.activatedItem = fallbacks_[0].first->id();
            }
            break;
        }
        }
        return true;
    }
    return false;
}
