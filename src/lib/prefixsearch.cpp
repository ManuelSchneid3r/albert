// Copyright (C) 2014-2021 Manuel Schneider

#include <QRegularExpression>
#include <algorithm>
#include "indexitem.h"
#include "prefixsearch.h"
using std::map;
using std::set;
using std::shared_ptr;
using std::vector;




Core::PrefixSearch::PrefixSearch(){

}



Core::PrefixSearch::PrefixSearch(const Core::PrefixSearch &rhs) {
    index_ = rhs.index_;
    invertedIndex_ = rhs.invertedIndex_;
}



Core::PrefixSearch::~PrefixSearch(){}



void Core::PrefixSearch::add(const std::shared_ptr<IndexItem> &indexable) {

    // Add indexable to the index
    index_.push_back(indexable);
    uint id = static_cast<uint>(index_.size()-1);

    vector<IndexItem::IndexString> indexStrings = indexable->indexStrings();
    for (const auto &idxStr : indexStrings) {
        // Build an inverted index
        QStringList words = idxStr.string.split(QRegularExpression(SEPARATOR_REGEX), QString::SkipEmptyParts);
        for (const QString &w : words) {
            invertedIndex_[w.toLower()].insert(id);
        }
    }
}



void Core::PrefixSearch::clear() {
    invertedIndex_.clear();
    index_.clear();
}



vector<shared_ptr<Core::IndexItem> > Core::PrefixSearch::search(const QString &query) const {

    // Make words unique, lower and prefixfree
    set<QString> words = splitString(query);

    // Skip if there arent any // CONSTRAINT (2): |W| > 0
    if (words.empty())
        return vector<shared_ptr<IndexItem>>();

    set<uint> resultsSet;
    set<QString>::iterator wordIt = words.begin();

    // Get a word mapping once before going to handle intersections
    for (std::map<QString,std::set<uint>>::const_iterator lb = invertedIndex_.lower_bound(*wordIt);
         lb != invertedIndex_.cend() && lb->first.startsWith(*wordIt); ++lb)
        resultsSet.insert(lb->second.begin(), lb->second.end());
    wordIt++;

    for (;wordIt != words.end(); ++wordIt) {

        // Unite the sets that are mapped by words that begin with word
        // w ∈ W. This set is called U_w
        set<uint> wordMappingsUnion;
        for (std::map<QString,std::set<uint>>::const_iterator lb = invertedIndex_.lower_bound(*wordIt);
             lb != invertedIndex_.cend() && lb->first.startsWith(*wordIt); ++lb)
            wordMappingsUnion.insert(lb->second.begin(), lb->second.end());

        // Intersect all sets U_w with the results
        set<uint> intersection;
        std::set_intersection(resultsSet.begin(), resultsSet.end(),
                              wordMappingsUnion.begin(), wordMappingsUnion.end(),
                              std::inserter(intersection, intersection.begin()));

        // Break if intersection is empty
        if (intersection.empty())
            return vector<shared_ptr<IndexItem>>();

        resultsSet = std::move(intersection);
    }

    // Convert to a std::vector
    vector<shared_ptr<IndexItem>> resultsVector;
    for (uint id : resultsSet)
        resultsVector.emplace_back(index_.at(id));
    return resultsVector;
}
