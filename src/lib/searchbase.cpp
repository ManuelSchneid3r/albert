// Copyright (C) 2014-2021 Manuel Schneider

#include <QRegularExpression>
#include <QStringList>
#include <set>
#include "searchbase.h"
using std::set;

Core::SearchBase::~SearchBase() {

}

std::set<QString> Core::SearchBase::splitString(const QString &str) const {

    // Split the query into words W
    QStringList wordlist = str.toLower().split(QRegularExpression(SEPARATOR_REGEX), QString::SkipEmptyParts);

    // Make words unique
    set<QString> words(wordlist.begin(), wordlist.end());

    // Drop all words that are prefixes of others (since this an ordered set the next word)
    for (auto it = words.begin(); it != words.end();) {
        auto next = std::next(it);
        if ( next != words.end() && next->startsWith(*it) )
            it = words.erase(it);
        else
            ++it;
    }

    return words;
}
