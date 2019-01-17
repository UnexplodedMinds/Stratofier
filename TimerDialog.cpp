/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QKeyEvent>

#include "TimerDialog.h"


TimerDialog::TimerDialog( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );
}


TimerDialog::~TimerDialog()
{
}


