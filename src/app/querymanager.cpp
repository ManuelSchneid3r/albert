// Copyright (C) 2014-2021 Manuel Schneider

#include <QApplication>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <chrono>
#include <vector>
#include "albert/extensionmanager.h"
#include "albert/fallbackprovider.h"
#include "albert/item.h"
#include "albert/queryhandler.h"
#include "logging.h"
#include "queryexecution.h"
#include "querymanager.h"
using namespace Core;
using namespace std;
using namespace std::chrono;

namespace {
const char* CFG_INCREMENTAL_SORT = "incrementalSort";
const bool  DEF_INCREMENTAL_SORT = false;
}

QueryManager::QueryManager(QObject *parent) : QObject(parent)
{
    QSqlQuery q;

    // Get last query id
    lastQueryId_ = 0;
    q.prepare("SELECT MAX(id) FROM query;");
    if (!q.exec())
        qFatal("SQL ERROR: %s %s", qPrintable(q.executedQuery()), qPrintable(q.lastError().text()));
    if (q.next())
        lastQueryId_ = q.value(0).toULongLong();

    // Get the handlers Ids
    q.exec("SELECT string_id, id FROM query_handler;");
    if (!q.exec())
        qFatal("SQL ERROR: %s %s", qPrintable(q.executedQuery()), qPrintable(q.lastError().text()));
    while(q.next())
        handlerIds_.emplace(q.value(0).toString(), q.value(1).toULongLong());

    // Initialize the order
    updateScores();

    QSettings s(qApp->applicationName());
    incrementalSort_ = s.value(CFG_INCREMENTAL_SORT, DEF_INCREMENTAL_SORT).toBool();
}


QueryManager::~QueryManager()
{

}


void QueryManager::setupSession()
{

    DEBG << "========== SESSION SETUP STARTED ==========";

    system_clock::time_point start = system_clock::now();

    // Call all setup routines
    for (QueryHandler *handler : Core::ExtensionManager::Global->extensionsOfType<QueryHandler>()) {
        system_clock::time_point start = system_clock::now();
        handler->setupSession();
        long duration = duration_cast<microseconds>(system_clock::now()-start).count();
        DEBG << "SESSION SETUP [µs]" << duration << handler->id();
    }

    long duration = duration_cast<microseconds>(system_clock::now()-start).count();
    DEBG << "SESSION SETUP TOTAL [µs]" << duration ;
}


void QueryManager::teardownSession()
{

    DEBG << "========== SESSION TEARDOWN STARTED ==========";

    system_clock::time_point start = system_clock::now();

    // Call all teardown routines
    for (QueryHandler *handler : Core::ExtensionManager::Global->extensionsOfType<QueryHandler>()) {
        system_clock::time_point start = system_clock::now();
        handler->teardownSession();
        long duration = duration_cast<microseconds>(system_clock::now()-start).count();
        DEBG << "SESSION TEARDOWN [µs]" << duration << handler->id();
    }

    // Clear views
    emit resultsReady(nullptr);

    // Store statistics
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery query(db);
    for ( QueryExecution *queryExecution : pastQueries_ ){

        ++lastQueryId_;
        const QueryStatistics &stats = queryExecution->stats;

        // Create a query record
        query.prepare("INSERT INTO query (id, input, cancelled, runtime, timestamp) "
                      "VALUES (:id, :input, :cancelled, :runtime, :timestamp);");
        query.bindValue(":id", lastQueryId_);
        query.bindValue(":input", stats.input);
        query.bindValue(":cancelled", stats.cancelled);
        query.bindValue(":runtime", static_cast<qulonglong>(duration_cast<microseconds>(stats.end-stats.start).count()));
        query.bindValue(":timestamp", static_cast<qulonglong>(duration_cast<seconds>(stats.start.time_since_epoch()).count()));
        if (!query.exec())
            qFatal("SQL ERROR: %s", qPrintable(query.lastError().text()));

        // Make sure all handlers exits in database
        query.prepare("INSERT INTO query_handler (string_id) VALUES (:id);");
        for ( auto & runtime : stats.runtimes ) {
            auto it = handlerIds_.find(runtime.first);
            if ( it == handlerIds_.end()){
                query.bindValue(":id", runtime.first);
                if (!query.exec())
                    qFatal("SQL ERROR: %s %s", qPrintable(query.executedQuery()), qPrintable(query.lastError().text()));
                handlerIds_.emplace(runtime.first, query.lastInsertId().toULongLong());
            }
        }

        // Create execution records
        query.prepare("INSERT INTO execution (query_id, handler_id, runtime) "
                      "VALUES (:query_id, :handler_id, :runtime);");
        for ( auto & runtime : stats.runtimes ) {
            query.bindValue(":query_id", lastQueryId_);
            query.bindValue(":handler_id", handlerIds_[runtime.first]);
            query.bindValue(":runtime", runtime.second);
            if (!query.exec())
                qFatal("SQL ERROR: %s %s", qPrintable(query.executedQuery()), qPrintable(query.lastError().text()));
        }

        // Create activation record
        if (!stats.activatedItem.isNull()) {
            query.prepare("INSERT INTO activation (query_id, item_id) VALUES (:query_id, :item_id);");
            query.bindValue(":query_id", lastQueryId_);
            query.bindValue(":item_id", stats.activatedItem);
            if (!query.exec())
                qFatal("SQL ERROR: %s %s", qPrintable(query.executedQuery()), qPrintable(query.lastError().text()));
        }
    }
    db.commit();

    // Delete queries
    for ( QueryExecution *query : pastQueries_ )
        if ( query->state() == QueryExecution::State::Running )
            connect(query, &QueryExecution::stateChanged,
                    query, [query](){ query->deleteLater(); });
        else
            delete query;
    pastQueries_.clear();

    // Compute new match rankings
    updateScores();

    long duration = duration_cast<microseconds>(system_clock::now()-start).count();
    DEBG << "SESSION TEARDOWN TOTAL [µs]" << duration ;
}


void QueryManager::startQuery(const QString &searchTerm)
{

    DEBG << "========== QUERY:" << searchTerm << " ==========";

    if ( pastQueries_.size() ) {
        // Stop last query
        QueryExecution *last = pastQueries_.back();
        disconnect(last, &QueryExecution::resultsReady, this, &QueryManager::resultsReady);
        if (last->state() != QueryExecution::State::Finished)
            last->cancel();
    }

    system_clock::time_point start = system_clock::now();

    // Start query
    QueryExecution *currentQuery = new QueryExecution(searchTerm, scores_, incrementalSort_);
    connect(currentQuery, &QueryExecution::resultsReady, this, &QueryManager::resultsReady);
    currentQuery->run();

    connect(currentQuery, &QueryExecution::stateChanged, [start](QueryExecution::State state){
        if ( state == QueryExecution::State::Finished ) {
            long duration = duration_cast<microseconds>(system_clock::now()-start).count();
            DEBG << QString("TIME: %1 µs QUERY OVERALL").arg(duration, 6);
        }
    });

    pastQueries_.emplace_back(currentQuery);

    long duration = duration_cast<microseconds>(system_clock::now()-start).count();
    DEBG << QString("TIME: %1 µs SESSION TEARDOWN OVERALL").arg(duration, 6);
}


bool QueryManager::incrementalSort()
{
    return incrementalSort_;
}


void QueryManager::setIncrementalSort(bool value)
{
    QSettings(qApp->applicationName()).setValue(CFG_INCREMENTAL_SORT, value);
    incrementalSort_ = value;
}


/**
 * @brief Core::MatchCompare::update
 * Update the usage score:
 * Score of a single usage is 1/(<age_in_days>+1).
 * Accumulate all scores groupes by itemId.
 * Normalize the scores to the range of UINT_MAX.
 */
void QueryManager::updateScores()
{
    scores_.clear();
    QSqlQuery query("SELECT a.item_id AS id, SUM(1/(julianday('now')-julianday(timestamp, 'unixepoch')+1)) AS score "
                    "FROM activation a JOIN  query q ON a.query_id = q.id "
                    "WHERE a.item_id<>'' "
                    "GROUP BY a.item_id "
                    "ORDER BY score DESC");
    if ( query.next() ){
        double max = query.value(1).toDouble();
        do {
            scores_.emplace(query.value(0).toString(), static_cast<uint>(query.value(1).toDouble()*UINT_MAX/max));
        } while (query.next());
    }
}
