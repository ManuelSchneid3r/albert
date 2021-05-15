// Copyright (C) 2014-2021 Manuel Schneider

#include "albert/util/offlineindex.h"
#include "albert/indexitem.h"
#include "prefixsearch.h"
#include "fuzzysearch.h"



Core::OfflineIndex::OfflineIndex(bool fuzzy)
    : impl_((fuzzy) ? new FuzzySearch() : new PrefixSearch()){
}



Core::OfflineIndex::OfflineIndex(OfflineIndex &&other) {
    impl_ = std::move(other.impl_);
}



Core::OfflineIndex &Core::OfflineIndex::operator=(Core::OfflineIndex &&other) {
    impl_ = std::move(other.impl_);
    return *this;
}



Core::OfflineIndex::~OfflineIndex() {

}



void Core::OfflineIndex::setFuzzy(bool fuzzy) {
    if (dynamic_cast<FuzzySearch*>(impl_.get())) {
        if (fuzzy) return;
        impl_.reset(new PrefixSearch(*dynamic_cast<FuzzySearch*>(impl_.get())));
    } else if (dynamic_cast<PrefixSearch*>(impl_.get())) {
        if (!fuzzy) return;
        impl_.reset(new FuzzySearch(*dynamic_cast<PrefixSearch*>(impl_.get())));
    } else {
        throw; //should not happen
    }
}



bool Core::OfflineIndex::fuzzy() {
    return dynamic_cast<FuzzySearch*>(impl_.get()) != nullptr;
}



void Core::OfflineIndex::setDelta(double d) {
    FuzzySearch* f = dynamic_cast<FuzzySearch*>(impl_.get());
    if (f)
        f->setDelta(d);
}



double Core::OfflineIndex::delta() {
    FuzzySearch* f = dynamic_cast<FuzzySearch*>(impl_.get());
    if (f)
        return f->delta();
    return 0;
}



void Core::OfflineIndex::add(const std::shared_ptr<Core::IndexItem> &idxble) {
    impl_->add(idxble);
}



void Core::OfflineIndex::clear() {
    impl_->clear();
}



std::vector<std::shared_ptr<Core::IndexItem> > Core::OfflineIndex::search(const QString &req) const {
    return impl_->search(req);
}
