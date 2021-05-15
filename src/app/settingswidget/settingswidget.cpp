// Copyright (C) 2014-2021 Manuel Schneider

#include "../querymanager.h"
#include "../trayicon.h"
#include "albert/extensionmanager.h"
#include "albert/frontend.h"
#include "globalshortcut/hotkeymanager.h"
#include "grabkeybutton.h"
#include "logging.h"
#include "pluginlistmodel.h"
#include "pluginwidget.h"
#include "settingswidget.h"
#include "statswidget.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFocusEvent>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QSqlQuery>
#include <QStandardPaths>
#include <memory>
#include <utility>
#include <vector>
using namespace std;
using namespace Core;
using namespace GlobalShortcut;

namespace {
const char* CFG_TERM = "terminal";

const std::vector<std::pair<const QString, const QString>> potential_terminals {
    {"Cool Retro Term", "cool-retro-term -e"},
    {"Deepin Terminal", "deepin-terminal -x"},
    {"Elementary Terminal", "io.elementary.terminal -x"},
    {"Gnome Terminal", "gnome-terminal --"},
    {"Konsole", "konsole -e"},
    {"LXTerminal", "lxterminal -e"},
    {"Mate-Terminal", "mate-terminal -x"},
    {"QTerminal", "qterminal -e"},
    {"RoxTerm", "roxterm -x"},
    {"Terminator", "terminator -x"},
    {"Termite", "termite -e"},
    {"Tilix", "tilix -e"},
    {"UXTerm", "uxterm -e"},
    {"Urxvt", "urxvt -e"},
    {"XFCE-Terminal", "xfce4-terminal -x"},
    {"XTerm", "xterm -e"}
};


}

extern QString terminalCommand;

SettingsWidget::SettingsWidget(QueryManager *qm, HotkeyManager *hm, TrayIcon *ti)
    : queryManager_(qm), hotkeyManager_(hm), trayIcon_(ti)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    /*
     * GENERAL
     */

    // HOTKEY
    if (hotkeyManager_) {
        QSet<int> hks = hotkeyManager_->hotkeys();
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

//    // FRONTEND
//    for (decltype(auto) plugin : frontendManager_->frontends()){

//        // Add item (text and id)
//        ui.comboBox_frontend->addItem(plugin->name(), plugin->id());

//        // Add tooltip
//        ui.comboBox_frontend->setItemData(ui.comboBox_frontend->count()-1,
//                                          QString("%1\nID: %2\nVersion: %3\nAuthors: %4\nDependencies: %5")
//                                          .arg(plugin->name(),
//                                               plugin->id(),
//                                               plugin->version(),
//                                               plugin->authors().join(", "),
//                                               plugin->dependencies().join(", ")),
//                                          Qt::ToolTipRole);
//        // Set to current if ids match
//        if ( plugin->id() == frontendManager_->id() )
//            ui.comboBox_frontend->setCurrentIndex(ui.comboBox_frontend->count()-1);
//    }

//    ui.tabGeneral->layout()->addWidget(frontendManager_->widget(ui.tabGeneral));

//    connect(ui.comboBox_frontend, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//            [this](int i){
//        QString id = ui.comboBox_frontend->itemData(i, Qt::UserRole).toString();
//        frontendManager_->setCurrentFrontend(id);

//        QLayoutItem* item;
//        for ( int i = ui.tabGeneral->layout()->count() - 1; i > 0; --i ) {
//            item = ui.tabGeneral->layout()->takeAt(i);
//            delete item->widget();
//            delete item;
//        }

//        ui.tabGeneral->layout()->addWidget(frontendManager_->widget(ui.tabGeneral));

//    });




    for (const auto &[name, cmd] : potential_terminals)
        if (!QStandardPaths::findExecutable(cmd.section(' ', 0)).isNull())
            if (ui.comboBox_term->addItem(name, cmd); cmd == terminalCommand)
                ui.comboBox_term->setCurrentIndex(ui.comboBox_term->count()-1);  // last

     if (ui.comboBox_term->count() == 0)
         WARN << "No terminals found.";

    ui.comboBox_term->insertSeparator(ui.comboBox_term->count());
    ui.comboBox_term->addItem(tr("Custom"));
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


    //  PLUGINS TAB

//    PluginWidget *pluginsWidget = new PluginWidget(this);
//    ui.tabs->insertTab(2, pluginsWidget, "Plugins");


    // STATS TAB

    StatsWidget *statsWidget = new StatsWidget(this);
    ui.tabs->insertTab(2, statsWidget, "Stats");


    //  ABOUT TAB

    QString about = ui.about_text->text();
    about.replace("___versionstring___", qApp->applicationVersion());
    about.replace("___buildinfo___", QString("Built %1 %2").arg(__DATE__, __TIME__));
    ui.about_text->setText(about);
}


void SettingsWidget::changeHotkey(int newhk)
{
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


void SettingsWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Escape ) {
        close();
    }
}


void SettingsWidget::closeEvent(QCloseEvent *event)
{
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
