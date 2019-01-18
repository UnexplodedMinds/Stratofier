/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __TIMERDIALOG_H__
#define __TIMERDIALOG_H__

#include <QDialog>

#include "ui_TimerDialog.h"


class TimerDialog : public QDialog, public Ui::TimerDialog
{
    Q_OBJECT

public:
    enum TimerDialogSel
    {
        Restart = 10,
        Change = 20
    };

    explicit TimerDialog( QWidget *pParent );
    ~TimerDialog();

private slots:
    void restart();
    void change();
};

#endif // __TIMERDIALOG_H__
