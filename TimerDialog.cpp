/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QKeyEvent>

#include "TimerDialog.h"


TimerDialog::TimerDialog( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );
    connect( m_pRestartButton, SIGNAL( clicked() ), this, SLOT( restart() ) );
    connect( m_pChangeButton, SIGNAL( clicked() ), this, SLOT( change() ) );
}


TimerDialog::~TimerDialog()
{
}


void TimerDialog::restart()
{
    done( Restart );
}


void TimerDialog::change()
{
    done( Change );
}

