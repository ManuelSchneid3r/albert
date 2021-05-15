// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QString>
#include <vector>
#include <memory>
#include "../export.h"

namespace Core {

class SearchBase;
class IndexItem;

class EXPORT_CORE OfflineIndex final {

public:

    /**
     * @brief Contstructs a search
     * @param fuzzy Sets the type of the search. Defaults to false.
     */
    OfflineIndex(bool fuzzy = false);
    OfflineIndex(const OfflineIndex &other) = delete;
    OfflineIndex(OfflineIndex &&other);
    OfflineIndex &operator=(const OfflineIndex & other) = delete;
    OfflineIndex &operator=(OfflineIndex && other);

    /**
     * @brief Destructs the search
     * @param
     */
    ~OfflineIndex();

    /**
     * @brief Sets the type of the search to fuzzy
     * @param fuzzy The type to set. Defaults to true.
     */
    void setFuzzy(bool fuzzy = true);

    /**
     * @brief Type of the search
     * @return True if the search is fuzzy else false.
     */
    bool fuzzy();

    /**
     * @brief Set the error tolerance of the fuzzy search
     *
     * If the value d is >1, the search tolerates d errors. If the value d is <1,
     * the search tolerates wordlength * d errors. The "amount of tolerance" is
     * measures in maximal prefix edit distance. If the search is not set to fuzzy
     * setDelta has no effect.
     *
     * @param t The amount of error tolerance
     */
    void setDelta(double d);

    /**
     * @brief The error tolerance of the fuzzy search
     * @return The amount of error tolerance if search is fuzzy 0 else.
     */
    double delta();

    /**
     * @brief Build the search index
     * @param The items to index
     */
    void add(const std::shared_ptr<Core::IndexItem> &idxble);

    /**
     * @brief Clear the search index
     */
    void clear();

    /**
     * @brief Perform a search on the index
     * @param req The query string
     */
    std::vector<std::shared_ptr<Core::IndexItem>> search(const QString &req) const;

private:

    std::unique_ptr<SearchBase> impl_;

};

}



