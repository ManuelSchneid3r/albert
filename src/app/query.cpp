// Copyright (C) 2014-2021 Manuel Schneider

#include "albert/item.h"
#include "albert/query.h"
using namespace Core;
using namespace std;



const QString &Query::string() const {
    return string_;
}



const QString &Query::rawString() const {
    return rawString_;
}



bool Query::isTriggered() const {
    return !trigger_.isNull();
}


void Query::disableSort()
{
    sort_ = false;
}



const QString &Query::trigger() const {
    return trigger_;
}



bool Query::isValid() const {
    return isValid_;
}



void Query::addMatchWithoutLock(const shared_ptr<Item> &item, uint score) {
    auto it = scores_.find(item->id());
    if ( it == scores_.end() )
        results_.emplace_back(item, 0 /*score/2*/);
    else
        results_.emplace_back(item, it->second/*(static_cast<ulong>(score)+it->second)/2*/);
}



void Query::addMatchWithoutLock(shared_ptr<Item> &&item, uint score) {
    auto it = scores_.find(item->id());
    if ( it == scores_.end() )
        results_.emplace_back(move(item), 0/*score/2*/);
    else
        results_.emplace_back(move(item), it->second/*(static_cast<ulong>(score)+it->second)/2*/);
}
