// Copyright (C) 2014-2021 Manuel Schneider

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QProcess>
#include <QStringList>
#include <pwd.h>
#include <unistd.h>
#include "standardactions.h"

namespace  {

void runDetached(const QStringList &commandline, const QString &workingDirectory) {
    if (commandline.isEmpty()){
        qDebug() << "NOP: commandline is empty";
    } else {
        qint64 pid;
        QStringList mutableCmdLn(commandline);
        if (QProcess::startDetached(mutableCmdLn.takeFirst(), mutableCmdLn, workingDirectory, &pid))
            qInfo() << "Detached process started successfully. PID:" << pid << commandline;
        else
            qWarning() << "Starting detached process failed." << commandline;
    }
}

}

// Place it somewhere for now. TODO: move it to potential core plugin
EXPORT_CORE /* global */ QString terminalCommand;


/** **************************************************************************/
Core::StandardActionBase::StandardActionBase(const QString &text)
    : text_(text) { }


QString Core::StandardActionBase::text() const
{
    return text_;
}


/** **************************************************************************/
Core::FuncAction::FuncAction(const QString &text, std::function<void ()> action)
    : StandardActionBase(text), action_(action) { }


void Core::FuncAction::activate() const
{
    action_();
}


/** **************************************************************************/
Core::ClipAction::ClipAction(const QString &text,const QString &clipBoardText)
    : StandardActionBase(text), clipBoardText_(clipBoardText) { }


void Core::ClipAction::activate() const
{
    QGuiApplication::clipboard()->setText(clipBoardText_);
}


/** **************************************************************************/
Core::UrlAction::UrlAction(const QString &text,const QUrl &url)
    : StandardActionBase(text), url_(url) { }


void Core::UrlAction::activate() const
{
    QDesktopServices::openUrl(url_);
}


/** **************************************************************************/
Core::ProcAction::ProcAction(const QString &text, const QStringList &commandline, const QString &workingDirectory)
    : StandardActionBase(text), commandline_(commandline), workingDir_(workingDirectory) { }


void Core::ProcAction::activate() const
{
    runDetached(commandline_, workingDir_);
}

/** **************************************************************************/
Core::TermAction::TermAction(const QString &text, const QStringList &commandline, const QString &workingDirectory)
    : StandardActionBase(text), commandline_(commandline), workingDir_(workingDirectory) { }


Core::TermAction::TermAction(const QString &text, const QString &script, Core::TermAction::CloseBehavior closeBehavior, const QString &workingDirectory)
    : StandardActionBase(text), workingDir_(workingDirectory)
{
    // Get the user shell (passwd must not be freed)
    passwd *pwd = getpwuid(geteuid());
    if (pwd == nullptr)
        throw "Could not retrieve user shell";

    switch (closeBehavior) {
    case CloseBehavior::CloseOnExit:
        commandline_ << pwd->pw_shell << "-ic" << script;
        break;
    case CloseBehavior::CloseOnSuccess:
        commandline_ << pwd->pw_shell << "-ic" << QString("%1 || exec %2").arg(script, pwd->pw_shell);
        break;
    case CloseBehavior::DoNotClose:
        commandline_ << pwd->pw_shell << "-ic" << QString("%1; exec %2").arg(script, pwd->pw_shell);
    }
}


void Core::TermAction::activate() const
{
    runDetached(terminalCommand.split(QChar(QChar::Space), QString::SkipEmptyParts) << commandline_, workingDir_);
}
