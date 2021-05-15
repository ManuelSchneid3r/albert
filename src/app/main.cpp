// Copyright (C) 2014-2021 Manuel Schneider

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QTime>
#include <QUrl>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#if defined __linux__ || defined __freebsd__
#include <QX11Info>
#include "xcb/xproto.h"
#endif
#include <csignal>
#include <functional>
#include "albert/frontend.h"
#include "albert/queryhandler.h"
#include "albert/util/standardactions.h"
#include "albert/util/standarditem.h"
#include "globalshortcut/hotkeymanager.h"
#include "logging.h"
#include "nativepluginprovider.h"
#include "querymanager.h"
#include "settingswidget/settingswidget.h"
#include "trayicon.h"
#include "xdg/iconlookup.h"
Q_LOGGING_CATEGORY(clc, "core")
using namespace Core;
using namespace std;
using namespace GlobalShortcut;


struct CoreQueryHandler : public QueryHandler {
    CoreQueryHandler(const vector<shared_ptr<Item>>& items) : items(items){}
    QString id() const override { return "org.albert"; }
    void handleQuery(Query * query) const override;
    vector<shared_ptr<Item>> items;
};


struct GlobalNativeEventFilter : public QAbstractNativeEventFilter {
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *) override;
};



/* global */ QString terminalCommand;

unique_ptr<GlobalNativeEventFilter> gnev;
unique_ptr<QApplication> app;
unique_ptr<QCommandLineParser> parser;
unique_ptr<TrayIcon> trayIcon;
unique_ptr<QMenu> trayIconMenu;
unique_ptr<QueryManager> queryManager;
unique_ptr<HotkeyManager> hotkeyManager;
unique_ptr<NativePluginProvider> nativePluginProvider;
#if X_PROTOCOL // Allow running on hosts with multiple Xservers
const QString socketPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+ "/socket.X" + QString::number(QX11Info::appScreen());
#else
const QString socketPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/socket";
#endif
unique_ptr<QLocalServer> localServer;
unique_ptr<CoreQueryHandler> coreQueryHandler;  // Needs SW


void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
void printReport();
void createWritableLocations();
void setupUnixSignalHandlers();
QStringList defaultPluginPaths();

void initializeCoreDatabase();
void initializeCoreQueryHandler();
void initializeIPCServer();
void initializeTrayIcon();
void initializeHotkey();
void initializeGlobalTerminalCommand();
void setupFrontendConnections();

void openSettingsWidget();// Needs NPP
void restartApp();
void notifyFirstRunAndMajorVersionChange();  // Needs SW


//void initializeGlobalTerminalCommand()
//{
//    terminalCommand = QSettings(qApp->applicationName()).value(CFG_TERM, QString()).toString();
//    // Set the terminal command
//    if (terminalCommand.isNull()){
//        if (terms.empty()){
//            CRIT << "No terminal command set. Terminal actions wont work as expected!";
//            terminalCommand = "";
//        } else {
//            terminalCommand = terms[0].second;
//            WARN << "No terminal command set. Using" << terminalCommand;
//        }
//    }
//}





int main(int argc, char **argv) {

    // Parse commandline
    parser = make_unique<QCommandLineParser>();
    parser->setApplicationDescription("Albert is still in alpha. These options may change in future versions.");
    parser->addHelpOption();
    parser->addVersionOption();
    parser->addOption(QCommandLineOption({"k", "hotkey"}, "Overwrite the hotkey to use.", "hotkey"));
    parser->addOption(QCommandLineOption({"p", "plugin-dirs"}, "Set the plugin dirs to use. Comma separated.", "directory"));
    parser->addOption(QCommandLineOption({"r", "report"}, "Print issue report."));
    parser->addPositionalArgument("command", "Command to send to a running instance, if any. (show, hide, toggle, preferences, restart, quit)", "lbert -h");

    /*
     *  IPC/SINGLETON MECHANISM (Client)
     *  For performance purposes this has been optimized by using a QCoreApp
     */
    {
        unique_ptr<QCoreApplication> capp = make_unique<QCoreApplication>(argc, argv);
        capp->setApplicationName("albert");
        capp->setApplicationVersion(PROJECT_VERSION);
        parser->process(*capp);

        if (parser->isSet("report")){
            printReport();
            return EXIT_SUCCESS;
        }

        const QStringList args = parser->positionalArguments();
        QLocalSocket socket;
        socket.connectToServer(socketPath);
        if ( socket.waitForConnected(500) ) { // Should connect instantly
            // If there is a command send it
            if ( args.count() != 0 ){
                socket.write(args.join(' ').toUtf8());
                socket.flush();
                socket.waitForReadyRead(500);
                if (socket.bytesAvailable())
                    INFO << socket.readAll();
            }
            else
                INFO << "There is another instance of albert running.";
            socket.close();
            ::exit(EXIT_SUCCESS);
        } else if ( args.count() == 1 ) {
            INFO << "There is no other instance of albert running.";
            ::exit(EXIT_FAILURE);
        }
    }

    /*
     *  INITIALIZE APPLICATION
     */

    {
        qInstallMessageHandler(customMessageHandler);
        QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));

        QSettings::setPath(QSettings::defaultFormat(), QSettings::UserScope,
                           QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

        if (!qEnvironmentVariableIsSet("QT_DEVICE_PIXEL_RATIO")
                && !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
                && !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
                && !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS"))
            QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        INFO << "Initializing application";
        app = make_unique<QApplication>(argc, argv);
        app->setApplicationName("albert");
        app->setApplicationDisplayName("Albert");
        app->setApplicationVersion(PROJECT_VERSION);
        app->setQuitOnLastWindowClosed(false);
        QString icon = XDG::IconLookup::iconPath("albert");
        if ( icon.isEmpty() ) icon = ":app_icon";
        app->setWindowIcon(QIcon(icon));
        gnev = make_unique<GlobalNativeEventFilter>();
        app->installNativeEventFilter(gnev.get());

        setupUnixSignalHandlers();
        createWritableLocations();
        initializeCoreDatabase();
        initializeTrayIcon();
        initializeIPCServer();
        initializeHotkey();
        ExtensionManager::Global = new ExtensionManager();
        setupFrontendConnections();
        QStringList pluginDirs = parser->isSet("plugin-dirs") ? parser->value("plugin-dirs").split(',') : defaultPluginPaths();
        nativePluginProvider = make_unique<NativePluginProvider>(pluginDirs);
        queryManager = make_unique<QueryManager>();
        notifyFirstRunAndMajorVersionChange();
    }

    /*
     * ENTER EVENTLOOP
     */

    INFO << "Entering eventloop";
    int retval = app->exec();

    /*
     *  FINALIZE APPLICATION
     */
    INFO << "Shutting down IPC server";
    localServer->close();

    // Delete the running indicator file
    INFO << "Deleting running indicator file";
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/running");

    INFO << "Quit";
    return retval;
}


void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "%s \x1b[34;1m[debg:%s]\x1b[0m \x1b[3m%s\x1b[0m\n",
                QTime::currentTime().toString().toLocal8Bit().constData(),
                context.category,
                message.toLocal8Bit().constData());
        break;
    case QtInfoMsg:
        fprintf(stdout, "%s \x1b[32;1m[info:%s]\x1b[0m %s\n",
                QTime::currentTime().toString().toLocal8Bit().constData(),
                context.category,
                message.toLocal8Bit().constData());
        break;
    case QtWarningMsg:
        fprintf(stdout, "%s \x1b[33;1m[warn:%s]\x1b[0;1m %s\x1b[0m\n",
                QTime::currentTime().toString().toLocal8Bit().constData(),
                context.category,
                message.toLocal8Bit().constData());
        break;
    case QtCriticalMsg:
        fprintf(stdout, "%s \x1b[31;1m[crit:%s]\x1b[0;1m %s\x1b[0m\n",
                QTime::currentTime().toString().toLocal8Bit().constData(),
                context.category,
                message.toLocal8Bit().constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s \x1b[41;30;4m[fatal:%s]\x1b[0;1m %s  --  [%s]\x1b[0m\n",
                QTime::currentTime().toString().toLocal8Bit().constData(),
                context.category,
                message.toLocal8Bit().constData(),
                context.function);
        exit(1);
    }
    fflush(stdout);
}


void printReport()
{
    const uint8_t w = 22;
    INFO << QString("%1: %2").arg("Albert version", w).arg(qApp->applicationVersion());
    INFO << QString("%1: %2").arg("Build date", w).arg(__DATE__ " " __TIME__);

    INFO << QString("%1: %2").arg("Qt version", w).arg(qVersion());
    INFO << QString("%1: %2").arg("QT_QPA_PLATFORMTHEME", w).arg(QString::fromLocal8Bit(qgetenv("QT_QPA_PLATFORMTHEME")));

    INFO << QString("%1: %2").arg("Binary location", w).arg(qApp->applicationFilePath());

    INFO << QString("%1: %2").arg("PWD", w).arg(QString::fromLocal8Bit(qgetenv("PWD")));
    INFO << QString("%1: %2").arg("SHELL", w).arg(QString::fromLocal8Bit(qgetenv("SHELL")));
    INFO << QString("%1: %2").arg("LANG", w).arg(QString::fromLocal8Bit(qgetenv("LANG")));

    INFO << QString("%1: %2").arg("XDG_SESSION_TYPE", w).arg(QString::fromLocal8Bit(qgetenv("XDG_SESSION_TYPE")));
    INFO << QString("%1: %2").arg("XDG_CURRENT_DESKTOP", w).arg(QString::fromLocal8Bit(qgetenv("XDG_CURRENT_DESKTOP")));
    INFO << QString("%1: %2").arg("DESKTOP_SESSION", w).arg(QString::fromLocal8Bit(qgetenv("DESKTOP_SESSION")));
    INFO << QString("%1: %2").arg("XDG_SESSION_DESKTOP", w).arg(QString::fromLocal8Bit(qgetenv("XDG_SESSION_DESKTOP")));

    INFO << QString("%1: %2").arg("OS", w).arg(QSysInfo::prettyProductName());
    INFO << QString("%1: %2/%3").arg("OS (type/version)", w).arg(QSysInfo::productType(), QSysInfo::productVersion());

    INFO << QString("%1: %2").arg("Build ABI", w).arg(QSysInfo::buildAbi());
    INFO << QString("%1: %2/%3").arg("Arch (build/current)", w).arg(QSysInfo::buildCpuArchitecture(), QSysInfo::currentCpuArchitecture());

    INFO << QString("%1: %2/%3").arg("Kernel (type/version)", w).arg(QSysInfo::kernelType(), QSysInfo::kernelVersion());
}


bool GlobalNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *)
{
#if defined __linux__ || defined __FreeBSD__
    if (eventType == "xcb_generic_event_t")
    {
        // A triggered key grab on X11 steals the focus of the window for short
        // period of time. This may result in the following annoying behaviour:
        // When the hotkey is pressed and X11 steals the focus there arises a
        // race condition between the hotkey event and the focus out event.
        // When the app is visible and the focus out event is delivered the app
        // gets hidden. Finally when the hotkey is received the app gets shown
        // again although the user intended to hide the app with the hotkey.

        // Although X11 differs between the two focus out events, qt does not.
        // Use a native event filter to eat the malicious events. The behaviour
        // was expected when the app hides on:

        // (mode==XCB_NOTIFY_MODE_GRAB && detail==XCB_NOTIFY_DETAIL_NONLINEAR)||
        //  (mode==XCB_NOTIFY_MODE_NORMAL && detail==XCB_NOTIFY_DETAIL_NONLINEAR)
        // (Check Xlib Programming Manual)

        xcb_generic_event_t* event = static_cast<xcb_generic_event_t *>(message);
        switch (event->response_type & 127)
        {
        case XCB_FOCUS_OUT: {
            xcb_focus_out_event_t *fe = reinterpret_cast<xcb_focus_out_event_t*>(event);
            std::string msg = "XCB_FOCUS_OUT";

            switch (fe->mode) {
                case XCB_NOTIFY_MODE_NORMAL:        msg += "::XCB_NOTIFY_MODE_NORMAL";break;
                case XCB_NOTIFY_MODE_GRAB:          msg += "::XCB_NOTIFY_MODE_GRAB";break;
                case XCB_NOTIFY_MODE_UNGRAB:        msg += "::XCB_NOTIFY_MODE_UNGRAB";break;
                case XCB_NOTIFY_MODE_WHILE_GRABBED: msg += "::XCB_NOTIFY_MODE_WHILE_GRABBED";break;
            }
            switch (fe->detail) {
                case XCB_NOTIFY_DETAIL_ANCESTOR:          msg += "::ANCESTOR";break;
                case XCB_NOTIFY_DETAIL_INFERIOR:          msg += "::INFERIOR";break;
                case XCB_NOTIFY_DETAIL_NONE:              msg += "::NONE";break;
                case XCB_NOTIFY_DETAIL_NONLINEAR:         msg += "::NONLINEAR";break;
                case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL: msg += "::NONLINEAR_VIRTUAL";break;
                case XCB_NOTIFY_DETAIL_POINTER:           msg += "::POINTER";break;
                case XCB_NOTIFY_DETAIL_POINTER_ROOT:      msg += "::POINTER_ROOT";break;
                case XCB_NOTIFY_DETAIL_VIRTUAL:           msg += "::VIRTUAL";break;
            }
            if (fe->mode==XCB_NOTIFY_MODE_NORMAL && fe->detail==XCB_NOTIFY_DETAIL_NONLINEAR )
                return false;
            else
                return true;  // Stop propagation

        }
        }
    }
#endif
    return false;
}


void CoreQueryHandler::handleQuery(Query *query) const
{
    for (auto item : items)
        if (!query->string().isEmpty() && item->text().toLower().startsWith(query->string().toLower()))
            query->addMatch(item, static_cast<uint>((query->string().length() / item->text().length()) * numeric_limits<uint>::max()));
}


void createWritableLocations()
{
    INFO << "Create mandatory directories";
    for (const auto location : { QStandardPaths::DataLocation, QStandardPaths::CacheLocation, QStandardPaths::AppConfigLocation }) {
        QString path = QStandardPaths::writableLocation(location);
        if (!QDir(path).mkpath("."))
            qFatal("Could not create dir: %s",  qPrintable(path));
    }
}


void setupUnixSignalHandlers()
{
    INFO << "Setup signal handlers";
    // Quit gracefully on unix signals
    for ( int sig : { SIGINT, SIGTERM, SIGHUP, SIGPIPE } ) {
        signal(sig, [](int){
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        });
    }
}


void initializeCoreQueryHandler()
{

    coreQueryHandler = make_unique<CoreQueryHandler>(initializer_list<shared_ptr<Item>>{
        makeStdItem("open-preferences",
            ":app_icon", "Preferences", "Open the Albert preferences window.",
            initializer_list<shared_ptr<Action>>{
                makeFuncAction("Open preferences.", [](){ openSettingsWidget(); })
            }
        ),
        makeStdItem("restart-albert",
            ":app_icon", "Restart Albert", "Restart this application.",
            initializer_list<shared_ptr<Action>>{
                makeFuncAction("Restart Albert", [](){ restartApp(); })
            }
        ),
        makeStdItem("quit-albert",
            ":app_icon", "Quit Albert", "Quit this application.",
            initializer_list<shared_ptr<Action>>{
                makeFuncAction("Quit Albert", [](){ qApp->quit(); })
            }
        )
    });
    Core::ExtensionManager::Global->registerExtension(coreQueryHandler.get());
}


void initializeCoreDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    if ( !db.isValid() )
        qFatal("No sqlite available");
    if (!db.driver()->hasFeature(QSqlDriver::Transactions))
        qFatal("QSqlDriver::Transactions not available.");
    db.setDatabaseName(QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath("core.db"));
    if (!db.open())
        qFatal("Unable to establish a database connection.");

    db.transaction();

    QSqlQuery q(db);
    if (!q.exec("CREATE TABLE IF NOT EXISTS query_handler ( "
                "  id INTEGER PRIMARY KEY NOT NULL, "
                "  string_id TEXT UNIQUE NOT NULL "
                "); "))
        qFatal("Unable to create table 'query_handler': %s", q.lastError().text().toUtf8().constData());

    if (!q.exec("CREATE TABLE IF NOT EXISTS query ( "
                "    id INTEGER PRIMARY KEY, "
                "    input TEXT NOT NULL, "
                "    cancelled INTEGER NOT NULL, "
                "    runtime INTEGER NOT NULL, "
                "    timestamp INTEGER DEFAULT CURRENT_TIMESTAMP "
                "); "))
        qFatal("Unable to create table 'query': %s", q.lastError().text().toUtf8().constData());

    if (!q.exec("CREATE TABLE IF NOT EXISTS execution ( "
                "    query_id INTEGER NOT NULL REFERENCES query(id) ON UPDATE CASCADE, "
                "    handler_id INTEGER NOT NULL REFERENCES query_handler(id) ON UPDATE CASCADE, "
                "    runtime INTEGER NOT NULL, "
                "    PRIMARY KEY (query_id, handler_id) "
                ") WITHOUT ROWID; "))
        qFatal("Unable to create table 'execution': %s", q.lastError().text().toUtf8().constData());

    if (!q.exec("CREATE TABLE IF NOT EXISTS activation ( "
                "    query_id INTEGER PRIMARY KEY NOT NULL REFERENCES query(id) ON UPDATE CASCADE, "
                "    item_id TEXT NOT NULL "
                "); "))
        qFatal("Unable to create table 'activation': %s", q.lastError().text().toUtf8().constData());

    if (!q.exec("DELETE FROM query WHERE julianday('now')-julianday(timestamp, 'unixepoch')>30; "))
        WARN << "Unable to cleanup 'query' table.";

    if (!q.exec("CREATE TABLE IF NOT EXISTS conf(key TEXT UNIQUE, value TEXT); "))
        qFatal("Unable to create table 'conf': %s", q.lastError().text().toUtf8().constData());

    db.commit();
}

QStringList defaultPluginPaths()
{
    QStringList defaultPluginPaths;

#if defined __linux__ || defined __FreeBSD__
    QStringList dirs = {
#if defined MULTIARCH_TUPLE
        QFileInfo("/usr/lib/" MULTIARCH_TUPLE).canonicalFilePath(),
#endif
        QFileInfo("/usr/lib/").canonicalFilePath(),
        QFileInfo("/usr/lib64/").canonicalFilePath(),
        QFileInfo("/usr/local/lib/").canonicalFilePath(),
        QFileInfo("/usr/local/lib64/").canonicalFilePath(),
        QDir::home().filePath(".local/lib/"),
        QDir::home().filePath(".local/lib64/")
    };

    dirs.removeDuplicates();

    for ( const QString& dir : dirs ) {
        QFileInfo fileInfo = QFileInfo(QDir(dir).filePath("albert/plugins"));
        if ( fileInfo.isDir() )
            defaultPluginPaths.push_back(fileInfo.canonicalFilePath());
    }
#elif defined __APPLE__
    throw "Not implemented";
#elif defined _WIN32
    throw "Not implemented";
#endif
    return defaultPluginPaths;
}


void notifyFirstRunAndMajorVersionChange()
{
    INFO << "Checking last used version";
    QFile file(QString("%1/last_used_version").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)));
    if ( file.exists() ) {
        // Read last used version
        if ( file.open(QIODevice::ReadOnly|QIODevice::Text) ) {
            QString lastUsedVersion;
            QTextStream(&file) >> lastUsedVersion;
            file.close();

            // Show newsbox in case of major version change
            if ( app->applicationVersion().section('.', 1, 1) != lastUsedVersion.section('.', 1, 1) ){
                // Do whatever is neccessary on first run
                QMessageBox(QMessageBox::Information, "Major version changed",
                            QString("You are now using Albert %1. The major version changed. "
                                    "Probably some parts of the API changed. Check the "
                                    "<a href=\"https://albertlauncher.github.io/news/\">news</a>.")
                            .arg(app->applicationVersion())).exec();
            }
        }
        else
            CRIT << QString("Could not open file %1: %2")
                    .arg(file.fileName(), file.errorString());
    } else {
        // Do whatever is neccessary on first run
        QMessageBox(QMessageBox::Information, "First run",
                    "Seems like this is the first time you run Albert. Albert is "
                    "standalone, free and open source software. Note that Albert is not "
                    "related to or affiliated with any other projects or corporations.\n\n"
                    "You should set a hotkey and enable some extensions.").exec();
        openSettingsWidget();
    }

    // Write the current version into the file
    if ( file.open(QIODevice::WriteOnly|QIODevice::Text) ) {
        QTextStream out(&file);
        out << app->applicationVersion();
        file.close();
    } else
        CRIT << QString("Could not open file %1: %2").arg(file.fileName(), file.errorString());
}


void initializeIPCServer()
{
    INFO << "Creating IPC server:" << socketPath;

    // Remove pipes potentially leftover after crash
    if (QFile::exists(socketPath)) {
        WARN << "Found leftover local server socket. Probably albert was not terminated correctly.";
        QLocalServer::removeServer(socketPath);
    }

    // Create server and handle messages
    localServer = make_unique<QLocalServer>();
    if ( !localServer->listen(socketPath) )
        WARN << "Local server could not be created. IPC will not work! Reason:"
                   << localServer->errorString();

    // Handle incoming messages
    QObject::connect(localServer.get(), &QLocalServer::newConnection, [&](){
        QLocalSocket* socket = localServer->nextPendingConnection();
        socket->waitForReadyRead(500);
        if (socket->bytesAvailable()) {
            QString msg = QString::fromLocal8Bit(socket->readAll());
            if ( msg.startsWith("show")) {
                for (auto *f : Core::ExtensionManager::Global->extensionsOfType<Frontend>()) {
                    if (msg.size() > 5) {
                        QString input = msg.mid(5);
                        f->setInput(input);
                    }
                    f->setVisible(true);
                }
                socket->write("Application set visible.");
            }
            else if ( msg == "hide") {
                for (auto *f : Core::ExtensionManager::Global->extensionsOfType<Frontend>())
                    f->setVisible(false);
                socket->write("Application set invisible.");
            }
            else if ( msg == "toggle") {
                for (auto *f : Core::ExtensionManager::Global->extensionsOfType<Frontend>())
                    f->toggleVisibility();
                socket->write("Visibility toggled.");
            }
            else if ( msg == "preferences") {
                openSettingsWidget();
                socket->write("Preferences opened.");
            }
            else if ( msg == "restart") {
                restartApp();
                socket->write("Albert restart triggered.");
            }
            else if ( msg == "quit") {
                qApp->quit();
                socket->write("Albert shutdown triggered.");
            }
            else
                socket->write("Command not supported.");
        }
        socket->flush();
        socket->close();
        socket->deleteLater();

    });
}


void initializeTrayIcon()
{
    trayIcon = make_unique<TrayIcon>();

    auto *trayIconMenu  = new QMenu();

    QAction* settingsAction = new QAction("Settings", trayIconMenu);
    trayIconMenu->addAction(settingsAction);
    QObject::connect(settingsAction, &QAction::triggered,
                     [](){openSettingsWidget();});

    QAction* docsAction = new QAction("Open docs", trayIconMenu);
    trayIconMenu->addAction(docsAction);
    QObject::connect(docsAction, &QAction::triggered,
                     [](){ QDesktopServices::openUrl(QUrl("https://albertlauncher.github.io/")); });

    trayIconMenu->addSeparator();

    QAction* restartAction = new QAction("Restart", trayIconMenu);
    trayIconMenu->addAction(restartAction);
    QObject::connect(restartAction, &QAction::triggered, [](){ restartApp(); });

    QAction* quitAction = new QAction("Quit", trayIconMenu);
    trayIconMenu->addAction(quitAction);
    QObject::connect(quitAction, &QAction::triggered,
                     qApp, &QApplication::quit);

    trayIcon->setContextMenu(trayIconMenu);
}


void openSettingsWidget()
{
    static unique_ptr<SettingsWidget> settingsWidget;
    if (!settingsWidget){
        settingsWidget = make_unique<SettingsWidget>(queryManager.get(),
                                                     hotkeyManager.get(),
                                                     trayIcon.get());
        QObject::connect(settingsWidget.get(), &QWidget::destroyed,
                         [&](){ settingsWidget.release(); });
    }
    QDesktopWidget *dw = QApplication::desktop();
    settingsWidget->move(dw->availableGeometry(dw->screenNumber(QCursor::pos())).center()
                         -QPoint(settingsWidget->width()/2,settingsWidget->height()/2));
    settingsWidget->show();
    settingsWidget->raise();
    settingsWidget->activateWindow();
}


void restartApp()
{
    QStringList cmdline(qApp->arguments());
    if (QProcess::startDetached(cmdline.takeFirst(), cmdline)) {
        // TODO: Potential race conditions on slow systems
        INFO << "Restarting application:" << cmdline;
        qApp->quit();
    } else
        WARN << "Restarting application failed:" << cmdline;
}


void setupFrontendConnections()
{
    QObject::connect(Core::ExtensionManager::Global, &Core::ExtensionManager::extensionRegistered,
                     [](Extension *e){
        Frontend *frontend = dynamic_cast<Frontend*>(e);
        if (frontend){
            QObject::connect(hotkeyManager.get(), &HotkeyManager::hotKeyPressed,
                             frontend, &Frontend::toggleVisibility);

            QObject::connect(trayIcon.get(), &TrayIcon::activated,
                             frontend, &Frontend::toggleVisibility);

            QObject::connect(queryManager.get(), &QueryManager::resultsReady,
                             frontend, &Frontend::setModel);

            QObject::connect(frontend, &Frontend::settingsWidgetRequested,
                             [](){ openSettingsWidget(); });

            QObject::connect(frontend, &Frontend::inputChanged,
                             queryManager.get(), &QueryManager::startQuery);

            QObject::connect(frontend, &Frontend::widgetHidden,
                             queryManager.get(), &QueryManager::teardownSession);

            QObject::connect(frontend, &Frontend::widgetShown, [=](){
                queryManager->setupSession();
                queryManager->startQuery(frontend->input());
            });

        }
    });
}

void initializeHotkey()
{
    QSettings settings(qApp->applicationName());
    if (!QGuiApplication::platformName().contains("wayland")) {
        hotkeyManager = make_unique<HotkeyManager>();
        if (parser->isSet("hotkey") && !hotkeyManager->registerHotkey(parser->value("hotkey")))
            CRIT << "Failed to set commandline hotkey";
        else if ( settings.contains("hotkey")  && !hotkeyManager->registerHotkey(settings.value("hotkey").toString()))
            CRIT << "Failed to set config hotkey";
    }
}
