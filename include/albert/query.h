// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QMutex>
#include <QString>
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include "export.h"

class QueryExecution;

namespace Core {

class QueryPrivate;
class Item;

/**
 * @brief The Query class
 * Represents a query to be handled by the query handlers
 */
class EXPORT_CORE Query final
{
public:

    /**
     * @brief The query string.
     * This is the processed query string relevant for most of the handlers. If the raw query string
     * is prefixed by a trigger string() returns the query string without the the trigger
     * prefix. If there is no trigger string() is equivalent to rawString().
     */
    const QString &string() const;

    /**
     * @brief The raw query string.
     * This is the raw query string as the users entered it into the input line.
     */
    const QString &rawString() const;

    /**
     * @brief Indicates a triggered query.
     */
    bool isTriggered() const;

    /**
     * @brief The sort hint of the query handler.
     * Call this if you want the query manager to omit the final sorting of items.
     * @note This is only effective for triggered batch handlers.
     */
    void disableSort();

    /**
     * @brief The trigger of this query
     * Note that if the trigger is set, string() differs from rawString().
     */
    const QString &trigger() const;

    /**
     * @brief The validity of the query
     * If the core cancelled the query for some reason the query gets invalid. Stop handling the
     * query if it is not valid anymore to save resources for other queries.
     * @return True if valid, else false.
     */
    bool isValid() const;

    /**
     * @brief addMatch
     * Use the addMatches if you have a lot of items to add.
     * @param item The to add to the results
     * @param score The relevance factor (UINT_MAX -> 1)
     * @see addMatches
     */
    template<typename T>
    void addMatch(T&& item, uint score = 0) {
        if ( isValid_ ) {
            mutex_.lock();
            addMatchWithoutLock(std::forward<T>(item), score);
            mutex_.unlock();
        }
    }

    /**
     * @brief addMatches
     * Cumulative addMatch function avoiding excessive mutex locking the results
     * @param begin
     * @param end
     */
    template<typename Iterator>
    void addMatches(Iterator begin, Iterator end) {
        if ( isValid() ) {
            mutex_.lock();
            for (; begin != end; ++begin)
                // Must not use operator->() !!! dereferencing a pointer returns an lvalue
                addMatchWithoutLock((*begin).first, (*begin).second);
            mutex_.unlock();
        }
    }

private:

    void addMatchWithoutLock(const std::shared_ptr<Core::Item> &item, uint score);
    void addMatchWithoutLock(std::shared_ptr<Core::Item> &&item, uint score);

    Query() = default;
    ~Query() = default;

    std::vector<std::pair<std::shared_ptr<Item>, uint>> results_;
    QMutex mutex_;
    QString trigger_;
    QString string_;
    QString rawString_;
    std::map<QString, uint> scores_;
    bool sort_ = true;
    bool isValid_ = true;

    friend class ::QueryExecution;
};

}


