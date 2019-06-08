/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QSettings>
#include <QPushButton>

#include "MenuDialog.h"
#include "AHRSMainWin.h"
#include "AHRSCanvas.h"
#include "Canvas.h"
#include "FuelTanksDialog.h"

#include "ui_MenuDialog.h"


extern QSettings *g_pSet;


MenuDialog::MenuDialog( QWidget *pParent, bool bPortrait )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_bPortrait( bPortrait )
{
    bool                bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();
    bool                bShowOutsideHead = g_pSet->value( "ShowOutsideHeading", true ).toBool();
    CanvasConstants     c = static_cast<AHRSMainWin *>( pParent )->disp()->canvas()->constants();
    int                 iIconSize = static_cast<int>( c.dH * (bPortrait ? 0.05 : 0.07) );

    setupUi( this );

    QList<QPushButton*> kids = findChildren<QPushButton *>();
    QPushButton        *pKid;

    foreach( pKid, kids )
        pKid->setIconSize( QSize( iIconSize, iIconSize ) );

    m_pTrafficFilterButton->blockSignals( true );
    m_pTrafficFilterButton->setChecked( bShowAllTraffic );
    m_pTrafficFilterButton->blockSignals( false );
    traffic( bShowAllTraffic );
    m_pTrafficInOutButton->blockSignals( true );
    m_pTrafficInOutButton->setChecked( bShowOutsideHead );
    m_pTrafficInOutButton->blockSignals( false );
    inOut( bShowOutsideHead );

    m_eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", 1 ).toInt() );

    // Set the enum so the inital cycling handler works out to the correct value
    if( m_eShowAirports == Canvas::ShowNoAirports )
        m_eShowAirports = Canvas::ShowAllAirports;
    else if( m_eShowAirports == Canvas::ShowPublicAirports )
        m_eShowAirports = Canvas::ShowNoAirports;
    else
        m_eShowAirports = Canvas::ShowPublicAirports;
    // Call the cycler to update and initialize the stored airport show selection
    airports();

    connect( m_pExitButton, SIGNAL( clicked() ), this, SIGNAL( shutdownStratux() ) );
    connect( m_pExitRoscoButton, SIGNAL( clicked() ), this, SIGNAL( shutdownRoscoPi() ) );
    connect( m_pResetLevelButton, SIGNAL( clicked() ), this, SIGNAL( resetLevel() ) );
    connect( m_pResetGMeterButton, SIGNAL( clicked() ), this, SIGNAL( resetGMeter() ) );
    connect( m_pUpgradeButton, SIGNAL( clicked() ), this, SIGNAL( upgradeRosco() ) );
    connect( m_pTrafficFilterButton, SIGNAL( toggled( bool ) ), this, SIGNAL( trafficToggled( bool ) ) );
    connect( m_pTrafficFilterButton, SIGNAL( toggled( bool ) ), this, SLOT( traffic( bool ) ) );
    connect( m_pTrafficInOutButton, SIGNAL( toggled( bool ) ), this, SIGNAL( inOutToggled( bool ) ) );
    connect( m_pTrafficInOutButton, SIGNAL( toggled( bool ) ), this, SLOT( inOut( bool ) ) );
    connect( m_pAirportButton, SIGNAL( clicked() ), this, SLOT( airports() ) );
    connect( m_pTimerButton, SIGNAL( clicked() ), this, SIGNAL( timer() ) );
    connect( m_pFuelButton, SIGNAL( clicked() ), this, SLOT( fuel() ) );
    connect( m_pDayModeButton, SIGNAL( clicked() ), this, SIGNAL( dayMode() ) );
    connect( m_pUnitsKnotsButton, SIGNAL( clicked() ), this, SIGNAL( unitsKnots() ) );
}


MenuDialog::~MenuDialog()
{
}


void MenuDialog::traffic( bool bAll )
{
    if( bAll )
        m_pTrafficFilterButton->setText( "ALL" );
    else
        m_pTrafficFilterButton->setText( "CLS" );
}


void MenuDialog::inOut( bool bOut )
{
    if( bOut )
        m_pTrafficInOutButton->setText( "OUT" );
    else
        m_pTrafficInOutButton->setText( "IN" );
}


void MenuDialog::airports()
{
    if( m_eShowAirports == Canvas::ShowNoAirports )
    {
        m_pAirportButton->setStyleSheet( "QPushButton { border: none; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 goldenrod, stop:1 white ); margin: 2px }" );
        m_eShowAirports = Canvas::ShowPublicAirports;
        m_pAirportButton->setText( "PUB AIRPORTS" );
    }
    else if( m_eShowAirports == Canvas::ShowPublicAirports )
    {
        m_pAirportButton->setStyleSheet( "QPushButton { border: none; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 green, stop:1 white ); margin: 2px }" );
        m_eShowAirports = Canvas::ShowAllAirports;
        m_pAirportButton->setText( "ALL AIRPORTS" );
    }
    else
    {
        m_pAirportButton->setStyleSheet( "QPushButton { border: none; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 #641200, stop:1 white ); margin: 2px }" );
        m_eShowAirports = Canvas::ShowNoAirports;
        m_pAirportButton->setText( "NO AIRPORTS  " );
    }

    emit showAirports( m_eShowAirports );
}


void MenuDialog::fuel()
{
    FuelTanksDialog dlg( this );
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

