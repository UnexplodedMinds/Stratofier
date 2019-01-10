/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include "MenuDialog.h"

#include "ui_MenuDialog.h"


MenuDialog::MenuDialog( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );

    connect( m_pExitButton, SIGNAL( clicked() ), this, SIGNAL( shutdownStratux() ) );
    connect( m_pExitRoscoButton, SIGNAL( clicked() ), this, SIGNAL( shutdownRoscoPi() ) );
    connect( m_pResetLevelButton, SIGNAL( clicked() ), this, SIGNAL( resetLevel() ) );
    connect( m_pResetGMeterButton, SIGNAL( clicked() ), this, SIGNAL( resetGMeter() ) );
    connect( m_pUpgradeButton, SIGNAL( clicked() ), this, SIGNAL( upgradeRosco() ) );
    connect( m_pTrafficFilterButton, SIGNAL( toggled( bool ) ), this, SIGNAL( trafficToggled( bool ) ) );
    connect( m_pTrafficFilterButton, SIGNAL( toggled( bool ) ), this, SLOT( traffic( bool ) ) );
}


MenuDialog::~MenuDialog()
{
}


void MenuDialog::traffic( bool bAll )
{
    if( bAll )
        m_pTrafficFilterButton->setText( "ALL TRAFFIC  " );
    else
        m_pTrafficFilterButton->setText( "CLS TRAFFIC  " );
}