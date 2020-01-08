/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QSettings>
#include <QtDebug>

#include "ui_FuelTanksDialog.h"
#include "ui_FuelTanksDialogLandscape.h"

#include "FuelTanksDialog.h"
#include "ClickLabel.h"
#include "Keypad.h"
#include "AHRSCanvas.h"


extern QSettings *g_pSet;

Ui::FuelTanksDialog          *uiP;
Ui::FuelTanksDialogLandscape *uiL;


FuelTanksDialog::FuelTanksDialog( QWidget *pParent, AHRSCanvas *pAHRS, Canvas *pCanvas )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_pAHRSDisp( pAHRS ),
      m_bPortrait( pCanvas->constants().bPortrait )
{
    if( m_bPortrait )
    {
        uiP = new Ui::FuelTanksDialog;
        uiP->setupUi( this );
    }
    else
    {
        uiL = new Ui::FuelTanksDialogLandscape;
        uiL->setupUi( this );
    }

    loadSettings();

    if( m_bPortrait )
    {
        uiP->m_pLeftRemainLabel->setTitle( "LEFT REMAINING" );
        uiP->m_pRightRemainLabel->setTitle( "RIGHT REMAINING" );
        uiP->m_pLeftCapLabel->setTitle( "LEFT CAPACITY" );
        uiP->m_pRightCapLabel->setTitle( "RIGHT CAPACITY" );
        uiP->m_pCruiseRateLabel->setTitle( "CRUISE RATE" );
        uiP->m_pClimbRateLabel->setTitle( "CLIMB RATE" );
        uiP->m_pDescRateLabel->setTitle( "DESCENT RATE" );
        uiP->m_pTaxiRateLabel->setTitle( "TAXI RATE" );
        uiP->m_pSwitchIntLabel->setTitle( "SWITCH INTERVAL" );
        uiP->m_pSwitchIntLabel->setDecimals( 0 );

        connect( uiP->m_pStartLeftButton, SIGNAL( clicked() ), this, SLOT( saveSettings() ) );
        connect( uiP->m_pStartRightButton, SIGNAL( clicked() ), this, SLOT( saveSettings() ) );
        connect( uiP->m_pStopButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
        connect( uiP->m_pResetFuelButton, SIGNAL( clicked() ), this, SLOT( resetFuel() ) );
    }
    else
    {
        uiL->m_pLeftRemainLabel->setTitle( "LEFT REMAINING" );
        uiL->m_pRightRemainLabel->setTitle( "RIGHT REMAINING" );
        uiL->m_pLeftCapLabel->setTitle( "LEFT CAPACITY" );
        uiL->m_pRightCapLabel->setTitle( "RIGHT CAPACITY" );
        uiL->m_pCruiseRateLabel->setTitle( "CRUISE RATE" );
        uiL->m_pClimbRateLabel->setTitle( "CLIMB RATE" );
        uiL->m_pDescRateLabel->setTitle( "DESCENT RATE" );
        uiL->m_pTaxiRateLabel->setTitle( "TAXI RATE" );
        uiL->m_pSwitchIntLabel->setTitle( "SWITCH INTERVAL" );
        uiL->m_pSwitchIntLabel->setDecimals( 0 );

        connect( uiL->m_pStartLeftButton, SIGNAL( clicked() ), this, SLOT( saveSettings() ) );
        connect( uiL->m_pStartRightButton, SIGNAL( clicked() ), this, SLOT( saveSettings() ) );
        connect( uiL->m_pStopButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
        connect( uiL->m_pResetFuelButton, SIGNAL( clicked() ), this, SLOT( resetFuel() ) );
    }

    ClickLabel         *pCL;
    QList<ClickLabel *> kids = findChildren<ClickLabel *>();

    foreach( pCL, kids )
    {
        pCL->setCanvas( pCanvas );
    }

    g_pSet->beginGroup( "FuelTanks" );
    if( !g_pSet->value( "DualTanks" ).toBool() )
    {
        if( m_bPortrait )
        {
            uiP->m_pStartLeftButton->setText( "START" );
            uiP->m_pStartRightButton->hide();
            uiP->m_pRRemLabel->hide();
            uiP->m_pRCapLabel->hide();
            uiP->m_pRightRemainLabel->hide();
            uiP->m_pRightCapLabel->hide();
            uiP->m_pRightRemUnitsLabel->hide();
            uiP->m_pRightCapUnitsLabel->hide();
            uiP->m_pLRemLabel->setText( "REM :" );
            uiP->m_pLCapLabel->setText( "CAP :" );
            uiP->m_pSwitchIntLabel->hide();
            uiP->m_pSwitchIntLabelLbl->hide();
            uiP->m_pSwitchIntUnitsLabel->hide();
        }
        else
        {
            uiL->m_pStartLeftButton->setText( "START" );
            uiL->m_pStartRightButton->hide();
            uiL->m_pRRemLabel->hide();
            uiL->m_pRCapLabel->hide();
            uiL->m_pRightRemainLabel->hide();
            uiL->m_pRightCapLabel->hide();
            uiL->m_pRightRemUnitsLabel->hide();
            uiL->m_pRightCapUnitsLabel->hide();
            uiL->m_pLRemLabel->setText( "REM :" );
            uiL->m_pLCapLabel->setText( "CAP :" );
            uiL->m_pSwitchIntLabel->hide();
            uiL->m_pSwitchIntLabelLbl->hide();
            uiL->m_pSwitchIntUnitsLabel->hide();
        }
    }
    else
    {
        if( m_bPortrait )
        {
            uiP->m_pStartLeftButton->setText( "START L" );
            uiP->m_pStartRightButton->show();
            uiP->m_pRRemLabel->show();
            uiP->m_pRCapLabel->show();
            uiP->m_pRightRemainLabel->show();
            uiP->m_pRightCapLabel->show();
            uiP->m_pRightRemUnitsLabel->show();
            uiP->m_pRightCapUnitsLabel->show();
            uiP->m_pLRemLabel->setText( "REM :" );
            uiP->m_pLCapLabel->setText( "CAP :" );
            uiP->m_pSwitchIntLabel->show();
            uiP->m_pSwitchIntLabelLbl->show();
            uiP->m_pSwitchIntUnitsLabel->show();
        }
        else
        {
            uiL->m_pStartLeftButton->setText( "START L" );
            uiL->m_pStartRightButton->show();
            uiL->m_pRRemLabel->show();
            uiL->m_pRCapLabel->show();
            uiL->m_pRightRemainLabel->show();
            uiL->m_pRightCapLabel->show();
            uiL->m_pRightRemUnitsLabel->show();
            uiL->m_pRightCapUnitsLabel->show();
            uiL->m_pLRemLabel->setText( "REM :" );
            uiL->m_pLCapLabel->setText( "CAP :" );
            uiL->m_pSwitchIntLabel->show();
            uiL->m_pSwitchIntLabelLbl->show();
            uiL->m_pSwitchIntUnitsLabel->show();
        }
    }
    g_pSet->endGroup();
}


FuelTanksDialog::~FuelTanksDialog()
{
    if( m_bPortrait )
        delete uiP;
    else
        delete uiL;
}


void FuelTanksDialog::loadSettings()
{
    g_pSet->beginGroup( "FuelTanks" );
    if( m_bPortrait )
    {
        uiP->m_pLeftCapLabel->setText( QString::number( g_pSet->value( "LeftCapacity", 24.0 ).toDouble(), 'f', 2 ) );
        uiP->m_pRightCapLabel->setText( QString::number( g_pSet->value( "RightCapacity", 24.0 ).toDouble(), 'f', 2 ) );
        uiP->m_pLeftRemainLabel->setText( QString::number( g_pSet->value( "LeftRemaining", 24.0 ).toDouble(), 'f', 2 ) );
        uiP->m_pRightRemainLabel->setText( QString::number( g_pSet->value( "RightRemaining", 24.0 ).toDouble(), 'f', 2 ) );

        uiP->m_pCruiseRateLabel->setText( QString::number( g_pSet->value( "CruiseRate", 8.3 ).toDouble(), 'f', 2 ) );
        uiP->m_pClimbRateLabel->setText( QString::number( g_pSet->value( "ClimbRate", 9.0 ).toDouble(), 'f', 2 ) );
        uiP->m_pDescRateLabel->setText( QString::number( g_pSet->value( "DescentRate", 7.0 ).toDouble(), 'f', 2 ) );
        uiP->m_pTaxiRateLabel->setText( QString::number( g_pSet->value( "TaxiRate", 4.0 ).toDouble(), 'f', 2 ) );
        uiP->m_pSwitchIntLabel->setText( QString::number( g_pSet->value( "SwitchInterval", 15 ).toInt() ) );
    }
    else
    {
        uiL->m_pStartLeftButton->setText( "START" );
        uiL->m_pStartRightButton->hide();
        uiL->m_pRRemLabel->hide();
        uiL->m_pRCapLabel->hide();
        uiL->m_pRightRemainLabel->hide();
        uiL->m_pRightCapLabel->hide();
        uiL->m_pRightRemUnitsLabel->hide();
        uiL->m_pRightCapUnitsLabel->hide();
        uiL->m_pLRemLabel->setText( "REM :" );
        uiL->m_pLCapLabel->setText( "CAP :" );
        uiL->m_pSwitchIntLabel->hide();
        uiL->m_pSwitchIntLabelLbl->hide();
        uiL->m_pSwitchIntUnitsLabel->hide();
    }

    m_tanks.bDualTanks = g_pSet->value( "DualTanks", true ).toBool();

    g_pSet->endGroup();
}


void FuelTanksDialog::saveSettings()
{
    if( m_bPortrait )
    {
        m_tanks.dLeftCapacity = uiP->m_pLeftCapLabel->text().toDouble();
        m_tanks.dRightCapacity = uiP->m_pRightCapLabel->text().toDouble();
        m_tanks.dLeftRemaining = uiP->m_pLeftRemainLabel->text().toDouble();
        m_tanks.dRightRemaining = uiP->m_pRightRemainLabel->text().toDouble();
        m_tanks.dFuelRateCruise = uiP->m_pCruiseRateLabel->text().toDouble();
        m_tanks.dFuelRateClimb = uiP->m_pClimbRateLabel->text().toDouble();
        m_tanks.dFuelRateDescent = uiP->m_pDescRateLabel->text().toDouble();
        m_tanks.dFuelRateTaxi = uiP->m_pTaxiRateLabel->text().toDouble();
        m_tanks.iSwitchIntervalMins = static_cast<int>( uiP->m_pSwitchIntLabel->text().toDouble() );
    }
    else
    {
        m_tanks.dLeftCapacity = uiL->m_pLeftCapLabel->text().toDouble();
        m_tanks.dRightCapacity = uiL->m_pRightCapLabel->text().toDouble();
        m_tanks.dLeftRemaining = uiL->m_pLeftRemainLabel->text().toDouble();
        m_tanks.dRightRemaining = uiL->m_pRightRemainLabel->text().toDouble();
        m_tanks.dFuelRateCruise = uiL->m_pCruiseRateLabel->text().toDouble();
        m_tanks.dFuelRateClimb = uiL->m_pClimbRateLabel->text().toDouble();
        m_tanks.dFuelRateDescent = uiL->m_pDescRateLabel->text().toDouble();
        m_tanks.dFuelRateTaxi = uiL->m_pTaxiRateLabel->text().toDouble();
        m_tanks.iSwitchIntervalMins = static_cast<int>( uiL->m_pSwitchIntLabel->text().toDouble() );
    }

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
    if( m_bPortrait )
    {
        uiP->m_pLeftRemainLabel->setText( QString::number( iGalL ) );
        uiP->m_pRightRemainLabel->setText( QString::number( iGalR ) );
    }
    else
    {
        uiL->m_pLeftRemainLabel->setText( QString::number( iGalL ) );
        uiL->m_pRightRemainLabel->setText( QString::number( iGalR ) );
    }

    g_pSet->beginGroup( "FuelTanks" );
    g_pSet->setValue( "LeftRemaining", iGalL );
    g_pSet->setValue( "RightRemaining", iGalR );
    g_pSet->endGroup();
    g_pSet->sync();
}

