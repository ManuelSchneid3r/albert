// Copyright (C) 2014-2021 Manuel Schneider

#include "pluginlistmodel.h"
#include "pluginwidget.h"
#include "albert/plugininstance.h"
#include "albert/pluginspec.h"
using namespace Core;


PluginWidget::PluginWidget(QWidget *parent) :  QWidget(parent)
{
    model = std::make_unique<PluginListModel>();

    // Layout structure
    hlayout = new QHBoxLayout(this);

    list_view = new QListView(this);
    hlayout->addWidget(list_view);

    vlayout = new QVBoxLayout;
    hlayout->addLayout(vlayout);

    label_plugin_title = new QLabel(this);
    vlayout->addWidget(label_plugin_title);

    vlayout->addWidget(buildPlaceholder("Choose a plugin"));

    // Layout congfig
    vlayout->setStretch(1,1);
    list_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    list_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    label_plugin_title->setStyleSheet("font-size: 12pt;padding-bottom: 5px;");
    label_plugin_title->hide();

    // Show the plugins. This* widget takes ownership of the model
    list_view->setModel(new PluginListModel(list_view));

    // Update infos when item is changed
    connect(list_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PluginWidget::updatePluginWidget);

    // update tthe
    connect(model.get(), &QAbstractListModel::dataChanged,
            this, &PluginWidget::onPluginDataChanged);
}


void PluginWidget::updatePluginWidget()
{
    // Delete old widget
    QLayoutItem *i = vlayout->takeAt(1);
    delete i->widget();
    delete i;

    QWidget *w;
    try {
        switch (const auto *spec = model->plugin_specs.at(list_view->currentIndex().row()); spec->state) {
        case PluginSpec::State::Error:
            w = buildPlaceholder(QString("Error loading plugin:\n%1").arg(spec->reason));
            label_plugin_title->hide();
            break;
        case PluginSpec::State::Invalid:
            w = buildPlaceholder(QString("Invalid plugin metadata:\n%1").arg(spec->reason));
            label_plugin_title->hide();
            break;
        case PluginSpec::State::Loaded:
            if (w = spec->instance->widget(this); w){
                label_plugin_title->setText(QString("<html><head/><body><p>"
                                                      "<span style=\"font-size:12pt;\">%1 </span>"
                                                      "<span style=\"font-size:8pt; font-style:italic; color:#a0a0a0;\">%3 %2</span>"
                                                      "</p></body></html>").arg(spec->name, spec->version, spec->id));
            } else {
                w = buildPlaceholder("void.");
            }
            label_plugin_title->show();
            break;
        case PluginSpec::State::Loading:
            w = buildPlaceholder("Plugin initializing…");
            label_plugin_title->hide();
            break;
        case PluginSpec::State::Ready:
            w = buildPlaceholder(QString("Click the checkbox to load %1 v%2.").arg(spec->name, spec->version));
            label_plugin_title->hide();
            break;
        }
    }  catch (std::out_of_range) {
        w = buildPlaceholder("Choose a plugin");
        label_plugin_title->hide();
    }
    vlayout->addWidget(w);
}


void PluginWidget::onPluginDataChanged(const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &roles)
{
    if (topLeft == list_view->currentIndex())
        for (int role : roles)
            if (role == Qt::CheckStateRole)
                updatePluginWidget();
}


QLabel *PluginWidget::buildPlaceholder(const QString &msg)
{
    auto *placeholder = new QLabel(msg, this);
    placeholder->setEnabled(false);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    return placeholder;
}
