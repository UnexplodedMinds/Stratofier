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

    connect( m_pExitButton, SIGNAL( clicked() ), this, SLOT( exitRoscoPi() ) );
    connect( m_pExitRoscoButton, SIGNAL( clicked() ), this, SLOT( exitRoscoPiApp() ) );
    connect( m_pResetLevelButton, SIGNAL( clicked() ), this, SIGNAL( resetLevel() ) );
    connect( m_pResetGMeterButton, SIGNAL( clicked() ), this, SIGNAL( resetGMeter() ) );
}


MenuDialog::~MenuDialog()
{
}


// Sync the config and close the dialog
void MenuDialog::exitRoscoPi()
{
    reject();
}


void MenuDialog::exitRoscoPiApp()
{
    qApp->closeAllWindows();
}


