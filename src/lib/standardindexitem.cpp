// Copyright (C) 2018-2021 Manuel Schneider

#include "standardindexitem.h"

Core::StandardIndexItem::StandardIndexItem(QString id,
                                           QString iconPath,
                                           QString text,
                                           QString subtext,
                                           std::vector<Core::IndexItem::IndexString> indexStrings,
                                           std::vector<std::shared_ptr<Core::Action> > actions,
                                           QString completion,
                                           Core::Item::Urgency urgency)
    : id_(std::move(id)),
      iconPath_(std::move(iconPath)),
      text_(std::move(text)),
      subtext_(std::move(subtext)),
      indexStrings_(std::move(indexStrings)),
      actions_(std::move(actions)),
      completion_(std::move(completion)),
      urgency_(urgency)
{ }

QString Core::StandardIndexItem::id() const { return id_; }

void Core::StandardIndexItem::setId(QString id) { id_ = std::move(id); }

QString Core::StandardIndexItem::iconPath() const { return iconPath_; }

void Core::StandardIndexItem::setIconPath(QString iconPath) { iconPath_ = std::move(iconPath); }

QString Core::StandardIndexItem::text() const { return text_; }

void Core::StandardIndexItem::setText(QString text) { text_ = std::move(text); }

QString Core::StandardIndexItem::subtext() const { return subtext_; }

void Core::StandardIndexItem::setSubtext(QString subtext) { subtext_ = std::move(subtext); }

QString Core::StandardIndexItem::completion() const { return completion_; }

void Core::StandardIndexItem::setCompletion(QString completion) { completion_ = std::move(completion); }

Core::Item::Urgency Core::StandardIndexItem::urgency() const { return urgency_; }

void Core::StandardIndexItem::setUrgency(Core::Item::Urgency urgency) { urgency_ = urgency; }

std::vector<std::shared_ptr<Core::Action> > Core::StandardIndexItem::actions() { return actions_; }

void Core::StandardIndexItem::setActions(std::vector<std::shared_ptr<Core::Action> > actions) { actions_ = std::move(actions); }

void Core::StandardIndexItem::addAction(std::shared_ptr<Core::Action> action) { actions_.push_back(std::move(action)); }

std::vector<Core::IndexItem::IndexString> Core::StandardIndexItem::indexStrings() const { return indexStrings_; }

void Core::StandardIndexItem::setIndexKeywords(std::vector<Core::IndexItem::IndexString> indexStrings) { indexStrings_ = std::move(indexStrings); }
