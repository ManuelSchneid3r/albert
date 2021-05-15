// Copyright (C) 2014-2021 Manuel Schneider

#include "standarditem.h"

Core::StandardItem::StandardItem(QString id,
                                 QString iconPath,
                                 QString text,
                                 QString subtext,
                                 std::vector<std::shared_ptr<Core::Action> > actions,
                                 QString completion, Core::Item::Urgency urgency)
    : id_(std::move(id)),
      iconPath_(std::move(iconPath)),
      text_(std::move(text)),
      subtext_(std::move(subtext)),
      actions_(std::move(actions)),
      completion_(std::move(completion)),
      urgency_(urgency)
{ }

QString Core::StandardItem::id() const { return id_; }

void Core::StandardItem::setId(QString id) { id_ = std::move(id); }

QString Core::StandardItem::iconPath() const { return iconPath_; }

void Core::StandardItem::setIconPath(QString iconPath) { iconPath_ = std::move(iconPath); }

QString Core::StandardItem::text() const { return text_; }

void Core::StandardItem::setText(QString text) { text_ = std::move(text); }

QString Core::StandardItem::subtext() const { return subtext_; }

void Core::StandardItem::setSubtext(QString subtext) { subtext_ = std::move(subtext); }

QString Core::StandardItem::completion() const { return completion_; }

void Core::StandardItem::setCompletion(QString completion) { completion_ = std::move(completion); }

Core::Item::Urgency Core::StandardItem::urgency() const { return urgency_; }

void Core::StandardItem::setUrgency(Core::Item::Urgency urgency) { urgency_ = urgency; }

std::vector<std::shared_ptr<Core::Action> > Core::StandardItem::actions() { return actions_; }

void Core::StandardItem::setActions(std::vector<std::shared_ptr<Core::Action> > &&actions) { actions_ = std::move(actions); }

void Core::StandardItem::setActions(const std::vector<std::shared_ptr<Core::Action> > &actions) { actions_ = actions; }

void Core::StandardItem::addAction(std::shared_ptr<Core::Action> &&action) { actions_.push_back(std::move(action)); }

void Core::StandardItem::addAction(const std::shared_ptr<Core::Action> &action) { actions_.push_back(action); }
