// Copyright (C) 2014-2018 Manuel Schneider

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopWidget>
#include <QFocusEvent>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QSqlQuery>
#include <QStandardPaths>
#include <vector>
#include <memory>
#include <utility>
#include "../extensionmanager.h"
#include "../frontendmanager.h"
#include "../pluginspec.h"
#include "../querymanager.h"
#include "../trayicon.h"
#include "albert/extension.h"
#include "albert/frontend.h"
#include "globalshortcut/hotkeymanager.h"
#include "grabkeybutton.h"
#include "loadermodel.h"
#include "logging.h"
#include "settingswidget.h"
#include "statswidget.h"
using namespace std;
using namespace Core;
using namespace GlobalShortcut;

namespace {
const char* CFG_TERM = "terminal";
}

extern QString terminalCommand;


/** ***************************************************************************/
Core::SettingsWidget::SettingsWidget(ExtensionManager *extensionManager,
                                     FrontendManager *frontendManager,
                                     QueryManager *queryManager,
                                     HotkeyManager *hotkeyManager,
                                     TrayIcon *systemTrayIcon,
                                     QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f),
      extensionManager_(extensionManager),
      frontendManager_(frontendManager),
      queryManager_(queryManager),
      hotkeyManager_(hotkeyManager),
      trayIcon_(systemTrayIcon) {

    ui.setupUi(this);


    /*
     * GENERAL
     */

    // HOTKEY
    if (hotkeyManager) {
        QSet<int> hks = hotkeyManager->hotkeys();
        if (hks.size() < 1)
            ui.grabKeyButton_hotkey->setText("Press to set hotkey");
        else
            ui.grabKeyButton_hotkey->setText(QKeySequence(*hks.begin()).toString()); // OMG
        connect(ui.grabKeyButton_hotkey, &GrabKeyButton::keyCombinationPressed,
                this, &SettingsWidget::changeHotkey);
    } else {
        ui.grabKeyButton_hotkey->setVisible(false);
        ui.label_hotkey->setVisible(false);
    }

    // TRAY
    ui.checkBox_showTray->setChecked(trayIcon_->isVisible());
    connect(ui.checkBox_showTray, &QCheckBox::toggled,
            trayIcon_, &TrayIcon::setVisible);

    // INCREMENTAL SORT
    ui.checkBox_incrementalSort->setChecked(queryManager_->incrementalSort());
    connect(ui.checkBox_incrementalSort, &QCheckBox::toggled,
            queryManager_, &QueryManager::setIncrementalSort);

    // AUTOSTART
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    QString desktopfile_path = QStandardPaths::locate(QStandardPaths::ApplicationsLocation,
                                                      "albert.desktop",
                                                      QStandardPaths::LocateFile);
    if (!desktopfile_path.isNull()) {
        QString autostart_path = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).filePath("autostart/albert.desktop");
        ui.checkBox_autostart->setChecked(QFile::exists(autostart_path));
        connect(ui.checkBox_autostart, &QCheckBox::toggled,
                this, [=](bool toggled){
            if (toggled)
                QFile::link(desktopfile_path, autostart_path);
            else
                QFile::remove(autostart_path);
        });
    }
    else
        CRIT << "Deskop entry not found! Autostart option is nonfuctional";
#else
    ui.autostartCheckBox->setEnabled(false);
    WARN << "Autostart not implemented on this platform!"
#endif

    // FRONTEND
    for ( const unique_ptr<PluginSpec> &plugin : frontendManager_->frontendSpecs() ){

        // Add item (text and id)
        ui.comboBox_frontend->addItem(plugin->name(), plugin->id());

        // Add tooltip
        ui.comboBox_frontend->setItemData(ui.comboBox_frontend->count()-1,
                                          QString("%1\nID: %2\nVersion: %3\nAuthor: %4\nDependencies: %5")
                                          .arg(plugin->name(),
                                               plugin->id(),
                                               plugin->version(),
                                               plugin->author(),
                                               plugin->dependencies().join(", ")),
                                          Qt::ToolTipRole);
        // Set to current if ids match
        if ( plugin->id() == frontendManager_->currentFrontend()->id() )
            ui.comboBox_frontend->setCurrentIndex(ui.comboBox_frontend->count()-1);
    }

    ui.tabGeneral->layout()->addWidget(frontendManager_->currentFrontend()->widget(ui.tabGeneral));

    connect(ui.comboBox_frontend, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this](int i){
        QString id = ui.comboBox_frontend->itemData(i, Qt::UserRole).toString();
        frontendManager_->setCurrentFrontend(id);

        QLayoutItem* item;
        for ( int i = ui.tabGeneral->layout()->count() - 1; i > 0; --i ) {
            item = ui.tabGeneral->layout()->takeAt(i);
            delete item->widget();
            delete item;
        }

        ui.tabGeneral->layout()->addWidget(frontendManager_->currentFrontend()->widget(ui.tabGeneral));

    });


    // TERM CMD (TOOOOOOOOOOODOOOOOOOOOO CENTRALIZE THIS)

    Q_ASSERT(terminalCommand.isEmpty());

    // Available terms
    std::vector<std::pair<QString, QString>> terms {
        // Distro terms
        {"Deepin Terminal", "deepin-terminal -x"},
        {"Elementary Terminal", "io.elementary.terminal -x"},
        {"Gnome Terminal", "gnome-terminal --"},
        {"Konsole", "konsole -e"},
        {"LXTerminal", "lxterminal -e"},
        {"Mate-Terminal", "mate-terminal -x"},
        {"XFCE-Terminal", "xfce4-terminal -x"},
        // Standalone terms
        {"Cool Retro Term", "cool-retro-term -e"},
        {"QTerminal", "qterminal -e"},
        {"RoxTerm", "roxterm -x"},
        {"Terminator", "terminator -x"},
        {"Termite", "termite -e"},
        {"Tilix", "tilix -e"},
        {"UXTerm", "uxterm -e"},
        {"Urxvt", "urxvt -e"},
        {"XTerm", "xterm -e"}
    };

    // Filter available terms by availability
    for (auto it = terms.cbegin(); it != terms.cend();)
        if (QStandardPaths::findExecutable(it->second.split(' ').first()).isEmpty())
            it = terms.erase(it);
        else
            ++it;

    if (terms.empty())
        WARN << "No terminals found.";

    // Set the terminal command
    terminalCommand = QSettings(qApp->applicationName()).value(CFG_TERM, QString()).toString();
    if (terminalCommand.isNull()){
        if (terms.empty()){
            CRIT << "No terminal command set. Terminal actions wont work as expected!";
            terminalCommand = "";
        } else {
            terminalCommand = terms[0].second;
            WARN << "No terminal command set. Using" << terminalCommand;
        }
    }

    // Fill checkbox
    for (const auto & t : terms)
        ui.comboBox_term->addItem(t.first, t.second);
    ui.comboBox_term->insertSeparator(ui.comboBox_term->count());
    ui.comboBox_term->addItem(tr("Custom"));

    // Set current item
    ui.comboBox_term->setCurrentIndex(-1);
    for (size_t i = 0; i < terms.size(); ++i)
        if (terms[i].second == terminalCommand)
            ui.comboBox_term->setCurrentIndex(static_cast<int>(i));
    if (ui.comboBox_term->currentIndex() == -1)
        ui.comboBox_term->setCurrentIndex(ui.comboBox_term->count()-1); // Is never -1 since Custom is always there

    // Put command in lineedit
    ui.lineEdit_term->setText(terminalCommand);
    ui.lineEdit_term->setEnabled(ui.comboBox_term->currentIndex() == ui.comboBox_term->count()-1);

    // Set behavior on index change
    connect(ui.comboBox_term, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this](int index){
        if ( index != ui.comboBox_term->count()-1) {
            terminalCommand = ui.comboBox_term->currentData(Qt::UserRole).toString();
            ui.lineEdit_term->setText(terminalCommand);
            QSettings(qApp->applicationName()).setValue(CFG_TERM, terminalCommand);
        }
        ui.lineEdit_term->setEnabled(index == ui.comboBox_term->count()-1);
    });

    // Set behavior for textEdited signal of custom-term-lineedit
    connect(ui.lineEdit_term, &QLineEdit::textEdited, [](QString str){
        terminalCommand = str;
        QSettings(qApp->applicationName()).setValue(CFG_TERM, terminalCommand);
    });

    // Cache
    connect(ui.pushButton_clearHistory, &QPushButton::clicked,
            []{ QSqlQuery("DELETE FROM activation;"); });

    /*
     * PLUGINS
     */

    // Show the plugins. This* widget takes ownership of the model
    ui.listView_plugins->setModel(new LoaderModel(extensionManager_, ui.listView_plugins));

    // Update infos when item is changed
    connect(ui.listView_plugins->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &SettingsWidget::updatePluginInformations);

    connect(ui.listView_plugins->model(), &QAbstractListModel::dataChanged,
            this, &SettingsWidget::onPluginDataChanged);

    // Initially hide the title label
    ui.label_pluginTitle->hide();


    /*
     * STATS
     */

// TODO: Remove Apr 2020
#ifdef BUILD_WITH_QTCHARTS
    StatsWidget *statsWidget = new StatsWidget(this);
    ui.tabs->insertTab(2, statsWidget, "Stats");
#endif


    /*
     * ABOUT
     */

    QString about = ui.about_text->text();
    about.replace("___versionstring___", qApp->applicationVersion());
    about.replace("___buildinfo___", QString("Built %1 %2").arg(__DATE__, __TIME__));
    ui.about_text->setText(about);

    QDesktopWidget *dw = QApplication::desktop();
    move(dw->availableGeometry(dw->screenNumber(QCursor::pos())).center()
                -QPoint(width()/2,height()/2));
    raise();
    activateWindow();
}



/** ***************************************************************************/
void SettingsWidget::updatePluginInformations(const QModelIndex & current) {
    // Hidde the placehodler text
    QLayoutItem *i = ui.widget_pluginInfos->layout()->takeAt(1);
    delete i->widget();
    delete i;

    PluginSpec *spec = extensionManager_->extensionSpecs()[static_cast<size_t>(current.row())].get();
    if (spec->state() == PluginSpec::State::Loaded) {

        Extension *extension = dynamic_cast<Extension*>(spec->instance());
        if (!extension)
            qFatal("Cannot cast an object of extension spec to an extension!");

        QWidget *pw = extension->widget();
        ui.widget_pluginInfos->layout()->addWidget(pw);// Takes ownership
        ui.label_pluginTitle->setText(QString("<html><head/><body><p>"
                                              "<span style=\"font-size:12pt;\">%1 </span>"
                                              "<span style=\"font-size:8pt; font-style:italic; color:#a0a0a0;\">%3 %2</span>"
                                              "</p></body></html>").arg(extension->name(), spec->version(), spec->id()));
        ui.label_pluginTitle->show();
    }
    else{
        QString msg("Plugin not loaded.\n%1");
        QLabel *lbl = new QLabel(msg.arg(extensionManager_->extensionSpecs()[static_cast<size_t>(current.row())]->lastError()));
        lbl->setEnabled(false);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setWordWrap(true);
        ui.widget_pluginInfos->layout()->addWidget(lbl);
        ui.label_pluginTitle->hide();
    }
}



/** ***************************************************************************/
void SettingsWidget::changeHotkey(int newhk) {
    Q_ASSERT(hotkeyManager_);
    int oldhk = *hotkeyManager_->hotkeys().begin(); //TODO Make cool sharesdpointer design

    // Try to set the hotkey
    if (hotkeyManager_->registerHotkey(newhk)) {
        QString hkText(QKeySequence((newhk&~Qt::GroupSwitchModifier)).toString());//QTBUG-45568
        ui.grabKeyButton_hotkey->setText(hkText);
        QSettings(qApp->applicationName()).setValue("hotkey", hkText);
        hotkeyManager_->unregisterHotkey(oldhk);
    } else {
        ui.grabKeyButton_hotkey->setText(QKeySequence(oldhk).toString());
        QMessageBox(QMessageBox::Critical, "Error",
                    QKeySequence(newhk).toString() + " could not be registered.",
                    QMessageBox::NoButton,
                    this).exec();
    }
}



/** ***************************************************************************/
void SettingsWidget::onPluginDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
    Q_UNUSED(bottomRight)
    if (topLeft == ui.listView_plugins->currentIndex())
        for (int role : roles)
            if (role == Qt::CheckStateRole)
                updatePluginInformations(topLeft);
}



/** ***************************************************************************/
void SettingsWidget::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Escape ) {
        close();
    }
}



/** ***************************************************************************/
void SettingsWidget::closeEvent(QCloseEvent *event) {
    if (hotkeyManager_ && hotkeyManager_->hotkeys().empty()) {
        QMessageBox msgBox(QMessageBox::Warning, "Hotkey Missing",
                           "Hotkey is invalid, please set it. Press OK to go "\
                           "back to the settings.",
                           QMessageBox::Ok|QMessageBox::Ignore,
                           this);
        msgBox.exec();
        if ( msgBox.result() == QMessageBox::Ok ) {
            ui.tabs->setCurrentIndex(0);
            show();
            event->ignore();
            return;
        }
    }
    event->accept();
}
