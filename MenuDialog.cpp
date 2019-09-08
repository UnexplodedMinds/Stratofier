/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QSettings>
#include <QPushButton>

#include "MenuDialog.h"
#include "AHRSMainWin.h"
#include "AHRSCanvas.h"
#include "Canvas.h"
#include "FuelTanksDialog.h"
#include "SettingsDialog.h"
#include "StreamReader.h"

#include "ui_MenuDialog.h"


extern QSettings *g_pSet;
extern bool       g_bUnitsKnots;


MenuDialog::MenuDialog( QWidget *pParent, bool bPortrait )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_bPortrait( bPortrait )
{
    CanvasConstants     c = static_cast<AHRSMainWin *>( pParent )->disp()->canvas()->constants();
    int                 iIconSize = static_cast<int>( c.dH * (bPortrait ? 0.05 : 0.07) );

    setupUi( this );

    QList<QPushButton*> kids = findChildren<QPushButton *>();
    QPushButton        *pKid;

    foreach( pKid, kids )
        pKid->setIconSize( QSize( iIconSize, iIconSize ) );

    m_pUnitsKnotsButton->setText( g_bUnitsKnots ? "KNOTS" : "MPH" );

    connect( m_pExitButton, SIGNAL( clicked() ), this, SIGNAL( shutdownStratux() ) );
    connect( m_pExitStratofierButton, SIGNAL( clicked() ), this, SIGNAL( shutdownStratofier() ) );
    connect( m_pResetLevelButton, SIGNAL( clicked() ), this, SIGNAL( resetLevel() ) );
    connect( m_pResetGMeterButton, SIGNAL( clicked() ), this, SIGNAL( resetGMeter() ) );
#if defined( Q_OS_ANDROID )
    m_pUpgradeButton->hide();
    m_pDayModeButton->hide();
#else
    connect( m_pUpgradeButton, SIGNAL( clicked() ), this, SIGNAL( upgradeRosco() ) );
    connect( m_pDayModeButton, SIGNAL( clicked() ), this, SIGNAL( dayMode() ) );
#endif
    connect( m_pTimerButton, SIGNAL( clicked() ), this, SIGNAL( timer() ) );
    connect( m_pFuelButton, SIGNAL( clicked() ), this, SLOT( fuel() ) );
    connect( m_pUnitsKnotsButton, SIGNAL( clicked() ), this, SIGNAL( unitsKnots() ) );

    connect( m_pSettingsButton, SIGNAL( clicked() ), this, SLOT( settings() ) );
}


MenuDialog::~MenuDialog()
{
}


void MenuDialog::fuel()
{
    FuelTanksDialog dlg( this, static_cast<AHRSMainWin *>( parent() )->disp()->canvas() );
    QWidget        *pMainWin = parentWidget();

    if( m_bPortrait )
        dlg.setGeometry( 0, 0, pMainWin->width(), pMainWin->width() );
    else
        dlg.setGeometry( 0, 0, pMainWin->width() / 2, pMainWin->width() / 2 );

    if( dlg.exec() == QDialog::Accepted )
        emit fuelTanks( dlg.settings() );
    else
        emit stopFuelFlow();
}


void MenuDialog::settings()
{
    CanvasConstants c = static_cast<AHRSMainWin *>( parent() )->disp()->canvas()->constants();
    QString         qsCurrIP = g_pSet->value( "StratuxIP", "192.168.10.1" ).toString();
    SettingsDialog  dlg( this, &c );

    // Scale the menu dialog according to screen resolution
    dlg.setMinimumWidth( static_cast<int>( c.dW ) );
    dlg.setMinimumHeight( static_cast<int>( m_bPortrait ? c.dH2 : c.dH ) );
    dlg.setGeometry( 0, 0, static_cast<int>( c.dW ), static_cast<int>( m_bPortrait ? c.dH2 : c.dH ) );

    connect( &dlg, SIGNAL( trafficToggled( bool ) ), this, SIGNAL( trafficToggled( bool ) ) );
    connect( &dlg, SIGNAL( showAirports( Canvas::ShowAirports ) ), this, SIGNAL( showAirports( Canvas::ShowAirports ) ) );
    connect( &dlg, SIGNAL( showRunways( bool ) ), this, SIGNAL( showRunways( bool ) ) );

    dlg.exec();

    if( g_pSet->value( "StratuxIP" ).toString() != qsCurrIP )
    {
        static_cast<AHRSMainWin *>( parent() )->streamReader()->disconnectStreams();
        static_cast<AHRSMainWin *>( parent() )->streamReader()->connectStreams();
    }
}
