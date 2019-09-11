/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QSettings>
#include <QtDebug>

#include "FuelTanksDialog.h"
#include "ClickLabel.h"
#include "Keypad.h"
#include "AHRSCanvas.h"


extern QSettings *g_pSet;


FuelTanksDialog::FuelTanksDialog( QWidget *pParent, AHRSCanvas *pAHRS, Canvas *pCanvas )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_pAHRSDisp( pAHRS )
{
    setupUi( this );

    loadSettings();

    m_pLeftRemainLabel->setTitle( "LEFT REMAINING" );
    m_pRightRemainLabel->setTitle( "RIGHT REMAINING" );
    m_pLeftCapLabel->setTitle( "LEFT CAPACITY" );
    m_pRightCapLabel->setTitle( "RIGHT CAPACITY" );
    m_pCruiseRateLabel->setTitle( "CRUISE RATE" );
    m_pClimbRateLabel->setTitle( "CLIMB RATE" );
    m_pDescRateLabel->setTitle( "DESCENT RATE" );
    m_pTaxiRateLabel->setTitle( "TAXI RATE" );
    m_pSwitchIntLabel->setTitle( "SWITCH INTERVAL" );
    m_pSwitchIntLabel->setDecimals( 0 );

    connect( m_pStartLeftButton, SIGNAL( clicked() ), this, SLOT( saveSettings() ) );
    connect( m_pStartRightButton, SIGNAL( clicked() ), this, SLOT( saveSettings() ) );
    connect( m_pStopButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
    connect( m_pResetFuelButton, SIGNAL( clicked() ), this, SLOT( resetFuel() ) );

    ClickLabel         *pCL;
    QList<ClickLabel *> kids = findChildren<ClickLabel *>();

    foreach( pCL, kids )
    {
        pCL->setCanvas( pCanvas );
    }

    g_pSet->beginGroup( "FuelTanks" );
    if( !g_pSet->value( "DualTanks" ).toBool() )
    {
        m_pStartLeftButton->setText( "START" );
        m_pStartRightButton->hide();
        m_pRRemLabel->hide();
        m_pRCapLabel->hide();
        m_pRightRemainLabel->hide();
        m_pRightCapLabel->hide();
        m_pRightRemUnitsLabel->hide();
        m_pRightCapUnitsLabel->hide();
        m_pLRemLabel->setText( "REM :" );
        m_pLCapLabel->setText( "CAP :" );
        m_pSwitchIntLabel->hide();
        m_pSwitchIntLabelLbl->hide();
        m_pSwitchIntUnitsLabel->hide();
    }
    g_pSet->endGroup();
}


FuelTanksDialog::~FuelTanksDialog()
{
}


void FuelTanksDialog::loadSettings()
{
    g_pSet->beginGroup( "FuelTanks" );
    m_pLeftCapLabel->setText( QString::number( g_pSet->value( "LeftCapacity", 24.0 ).toDouble(), 'f', 2 ) );
    m_pRightCapLabel->setText( QString::number( g_pSet->value( "RightCapacity", 24.0 ).toDouble(), 'f', 2 ) );
    m_pLeftRemainLabel->setText( QString::number( g_pSet->value( "LeftRemaining", 24.0 ).toDouble(), 'f', 2 ) );
    m_pRightRemainLabel->setText( QString::number( g_pSet->value( "RightRemaining", 24.0 ).toDouble(), 'f', 2 ) );

    m_pCruiseRateLabel->setText( QString::number( g_pSet->value( "CruiseRate", 8.3 ).toDouble(), 'f', 2 ) );
    m_pClimbRateLabel->setText( QString::number( g_pSet->value( "ClimbRate", 9.0 ).toDouble(), 'f', 2 ) );
    m_pDescRateLabel->setText( QString::number( g_pSet->value( "DescentRate", 7.0 ).toDouble(), 'f', 2 ) );
    m_pTaxiRateLabel->setText( QString::number( g_pSet->value( "TaxiRate", 4.0 ).toDouble(), 'f', 2 ) );
    m_pSwitchIntLabel->setText( QString::number( g_pSet->value( "SwitchInterval", 15 ).toInt() ) );

    m_tanks.bDualTanks = g_pSet->value( "DualTanks", true ).toBool();

    g_pSet->endGroup();
}


void FuelTanksDialog::saveSettings()
{
    m_tanks.dLeftCapacity = m_pLeftCapLabel->text().toDouble();
    m_tanks.dRightCapacity = m_pRightCapLabel->text().toDouble();
    m_tanks.dLeftRemaining = m_pLeftRemainLabel->text().toDouble();
    m_tanks.dRightRemaining = m_pRightRemainLabel->text().toDouble();
    m_tanks.dFuelRateCruise = m_pCruiseRateLabel->text().toDouble();
    m_tanks.dFuelRateClimb = m_pClimbRateLabel->text().toDouble();
    m_tanks.dFuelRateDescent = m_pDescRateLabel->text().toDouble();
    m_tanks.dFuelRateTaxi = m_pTaxiRateLabel->text().toDouble();
    m_tanks.iSwitchIntervalMins = static_cast<int>( m_pSwitchIntLabel->text().toDouble() );

    g_pSet->beginGroup( "FuelTanks" );
    g_pSet->setValue( "LeftCapacity", m_tanks.dLeftCapacity );
    g_pSet->setValue( "RightCapacity", m_tanks.dRightCapacity );
    g_pSet->setValue( "LeftRemaining", m_tanks.dLeftRemaining );
    g_pSet->setValue( "RightRemaining", m_tanks.dRightRemaining );
    g_pSet->setValue( "CruiseRate", m_tanks.dFuelRateCruise );
    g_pSet->setValue( "ClimbRate", m_tanks.dFuelRateClimb );
    g_pSet->setValue( "DescentRate", m_tanks.dFuelRateDescent );
    g_pSet->setValue( "TaxiRate", m_tanks.dFuelRateTaxi );
    g_pSet->setValue( "SwitchInterval", m_tanks.iSwitchIntervalMins );
    g_pSet->endGroup();
    g_pSet->sync();

    m_tanks.bOnLeftTank = (sender()->objectName() == "m_pStartLeftButton");
    m_tanks.lastSwitch = QDateTime::currentDateTime();

    accept();
}


void FuelTanksDialog::resetFuel()
{
    Keypad keypadL( this, "RESET LEFT" );
    int    iGalR = 0, iGalL = 0;

    m_pAHRSDisp->canvas()->setKeypadGeometry( &keypadL );
    keypadL.exec();

    iGalL = static_cast<int>( keypadL.value() );

    if( m_tanks.bDualTanks )
    {
        Keypad keypadR( this, "RESET RIGHT" );

        m_pAHRSDisp->canvas()->setKeypadGeometry( &keypadR );
        keypadR.exec();

        iGalR = static_cast<int>( keypadR.value() );
        if( iGalR < 0 )
            iGalR = 0;
    }

    m_tanks.dLeftRemaining = iGalL;
    m_tanks.dRightRemaining = iGalR;
    m_pLeftRemainLabel->setText( QString::number( iGalL ) );
    m_pRightRemainLabel->setText( QString::number( iGalR ) );

    g_pSet->beginGroup( "FuelTanks" );
    g_pSet->setValue( "LeftRemaining", iGalL );
    g_pSet->setValue( "RightRemaining", iGalR );
    g_pSet->endGroup();
    g_pSet->sync();
}
