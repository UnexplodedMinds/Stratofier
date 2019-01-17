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
    explicit TimerDialog( QWidget *pParent );
    ~TimerDialog();
};

#endif // __TIMERDIALOG_H__
