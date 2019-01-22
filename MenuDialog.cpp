/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QSettings>

#include "MenuDialog.h"

#include "ui_MenuDialog.h"


extern QSettings *g_pSet;


MenuDialog::MenuDialog( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );

    traffic( g_pSet->value( "ShowAllTraffic", true ).toBool() );
    m_eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", 2 ).toInt() );
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
    connect( m_pAirportButton, SIGNAL( clicked() ), this, SLOT( airports() ) );
    connect( m_pTimerButton, SIGNAL( clicked() ), this, SIGNAL( timer() ) );
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

