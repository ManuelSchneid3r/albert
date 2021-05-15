// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QPushButton>
#include <QKeyEvent>
#include <QKeySequence>

class GrabKeyButton final : public QPushButton
{
    Q_OBJECT

public:
    GrabKeyButton(QWidget * parent = 0);
    ~GrabKeyButton();

private:
    bool waitingForHotkey_;
    QString oldText_;

    void grabAll();
    void releaseAll();
    void onClick();
    void keyPressEvent (QKeyEvent *) override;
    void keyReleaseEvent ( QKeyEvent* ) override;

signals:
    void keyCombinationPressed(int);
};
