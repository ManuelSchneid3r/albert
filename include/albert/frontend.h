// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QObject>
#include "export.h"
#include "plugininstance.h"
class QAbstractItemModel;

namespace Core {
class EXPORT_CORE Frontend : public QObject, virtual public Extension {
    Q_OBJECT
public:
    virtual bool isVisible() = 0;
    virtual void setVisible(bool visible = true) = 0;
    virtual QString input() = 0;
    virtual void setInput(const QString&) = 0;
    virtual void setModel(QAbstractItemModel *) = 0;
    virtual QWidget *widget(QWidget *parent) = 0;
    void toggleVisibility() { setVisible(!isVisible()); }

signals:
    void widgetShown();
    void widgetHidden();
    void inputChanged(QString qry);
    void settingsWidgetRequested();
};
}
