// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QUrl>
#include <functional>
#include "albert/action.h"
#include "../export.h"

namespace Core {


struct EXPORT_CORE StandardActionBase : public Action
{
    /**
     * @brief Base class for standard actions implementing the text getter
     * @param text The description of the action
     */
    explicit StandardActionBase(const QString &text);
    QString text() const override;
private:
    QString text_;
};


struct EXPORT_CORE FuncAction : public StandardActionBase
{
    /**
     * @brief Runs a function
     * @param text The description of the action
     * @param action The fuction to execute
     */
    explicit FuncAction(const QString &text, std::function<void()> action);
    void activate() const override;
private:
    std::function<void()> action_;
};
#define makeFuncAction std::make_shared<Core::FuncAction>


struct EXPORT_CORE ClipAction : public StandardActionBase
{
    /**
     * @brief Copies text into the clipboard
     * @param text The description of the action
     * @param clipBoardText The text to put in the clipboard
     */
    explicit ClipAction(const QString &text, const QString &clipBoardText);
    void activate() const override;
private:
    QString clipBoardText_;
};
#define makeClipAction std::make_shared<Core::ClipAction>

struct EXPORT_CORE UrlAction : public StandardActionBase
{
    /**
     * @brief Opens an URL using the system scheme/mime hanlders
     * @param text The description of the action
     * @param commandline The URL to open with the correspondig handler
     */
    explicit UrlAction(const QString &text, const QUrl &url);
    void activate() const override;
private:
    QUrl url_;
};
#define makeUrlAction std::make_shared<Core::UrlAction>


struct EXPORT_CORE ProcAction : public StandardActionBase
{
    /**
     * @brief Starts a detached process
     * @param text The description of the action
     * @param commandline The program with arguments to execute
     * @param workingDirectory The working directory
     */
    explicit ProcAction(const QString &text, const QStringList &commandline, const QString &workingDirectory = QString());
    void activate() const override;
protected:
    QStringList commandline_;
    QString workingDir_;
};
#define makeProcAction std::make_shared<Core::ProcAction>


struct EXPORT_CORE TermAction : public StandardActionBase
{
    enum CloseBehavior {
        CloseOnSuccess,
        CloseOnExit,
        DoNotClose
    };

    /**
     * @brief Runs a commandline in a detached user definded terminal
     * @param text The description of the action
     * @param commandline The commandline to run
     * @param workingDirectory The working directory
     */
    explicit TermAction(const QString &text, const QStringList &commandline, const QString &workingDirectory = QString());

    /**
     * @brief Executes a script wrapped in the user shell in a detached user definded terminal
     * @param text The description of the action
     * @param script The shell script to execute
     * @param closeBehavior What happens when the script finished
     * @param workingDirectory The working directory
     */
    explicit TermAction(const QString &text, const QString &script, CloseBehavior closeBehavior = CloseOnSuccess, const QString &workingDirectory = QString());
    void activate() const override;
private:
    QStringList commandline_;
    QString workingDir_;
};
#define makeTermAction std::make_shared<Core::TermAction>

}
