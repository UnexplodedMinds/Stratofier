/*
Stratofier Stratux AHRS Display
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
extern bool       g_bUnitsKnots;


MenuDialog::MenuDialog( QWidget *pParent, bool bPortrait )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_bPortrait( bPortrait )
{
    bool                bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();
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

    m_pUnitsKnotsButton->setText( g_bUnitsKnots ? "KNOTS" : "MPH" );

    connect( m_pExitButton, SIGNAL( clicked() ), this, SIGNAL( shutdownStratux() ) );
    connect( m_pExitRoscoButton, SIGNAL( clicked() ), this, SIGNAL( shutdownStratofier() ) );
    connect( m_pResetLevelButton, SIGNAL( clicked() ), this, SIGNAL( resetLevel() ) );
    connect( m_pResetGMeterButton, SIGNAL( clicked() ), this, SIGNAL( resetGMeter() ) );
#if defined( Q_OS_ANDROID )
    m_pUpgradeButton->hide();
    m_pDayModeButton->hide();
#else
    connect( m_pUpgradeButton, SIGNAL( clicked() ), this, SIGNAL( upgradeRosco() ) );
    connect( m_pDayModeButton, SIGNAL( clicked() ), this, SIGNAL( dayMode() ) );
#endif
    connect( m_pTrafficFilterButton, SIGNAL( toggled( bool ) ), this, SIGNAL( trafficToggled( bool ) ) );
    connect( m_pTrafficFilterButton, SIGNAL( toggled( bool ) ), this, SLOT( traffic( bool ) ) );
    connect( m_pAirportButton, SIGNAL( clicked() ), this, SLOT( airports() ) );
    connect( m_pTimerButton, SIGNAL( clicked() ), this, SIGNAL( timer() ) );
    connect( m_pFuelButton, SIGNAL( clicked() ), this, SLOT( fuel() ) );
    connect( m_pUnitsKnotsButton, SIGNAL( clicked() ), this, SIGNAL( unitsKnots() ) );
}


MenuDialog::~MenuDialog()
{
}


void MenuDialog::traffic( bool bAll )
{
    if( bAll )
        m_pTrafficFilterButton->setText( "ALL TRAFFIC" );
    else
        m_pTrafficFilterButton->setText( "CLOSE TRAFFIC" );
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

