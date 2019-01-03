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
    connect( m_pResetLevelButton, SIGNAL( clicked() ), this, SLOT( resetLevel() ) );
    connect( m_pExitRoscoButton, SIGNAL( clicked() ), this, SLOT( exitRoscoPiApp() ) );
}


MenuDialog::~MenuDialog()
{
}


// Bring up the settings dialog that just has an embedded QtWebEngineView
void MenuDialog::resetLevel()
{
    accept();
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


