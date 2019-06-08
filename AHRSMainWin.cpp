/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QTimer>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QFont>
#include <QDesktopWidget>
#include <QScreen>
#include <QGuiApplication>
#include <QSettings>

#include "AHRSMainWin.h"
#include "AHRSCanvas.h"
#include "StreamReader.h"
#include "MenuDialog.h"
#include "Canvas.h"
#include "Keypad.h"


extern QSettings *g_pSet;


#if defined( Q_OS_ANDROID )
// Android default font
QFont itsy(  "Roboto", 6, QFont::Normal  );
QFont wee(   "Roboto", 8, QFont::Normal  );
QFont tiny(  "Roboto", 12, QFont::Normal );
QFont small( "Roboto", 16, QFont::Normal );
QFont med(   "Roboto", 18, QFont::Bold   );
QFont large( "Roboto", 24, QFont::Bold   );
QFont huge(  "Roboto", 30, QFont::Bold   );
#else
// Raspbian default font
QFont itsy(  "Piboto", 6, QFont::Normal  );
QFont wee(   "Piboto", 8, QFont::Normal  );
QFont tiny(  "Piboto", 12, QFont::Normal );
QFont small( "Piboto", 16, QFont::Normal );
QFont med(   "Piboto", 18, QFont::Bold   );
QFont large( "Piboto", 24, QFont::Bold   );
QFont huge(  "Piboto", 30, QFont::Bold   );
#endif


bool g_bUnitsKnots = true;
bool g_bDayMode = true;


// Setup minimal UI elements and make the connections
AHRSMainWin::AHRSMainWin( const QString &qsIP, bool bPortrait )
    : QMainWindow( Q_NULLPTR, Qt::Window | Qt::FramelessWindowHint ),
      m_pStratuxStream( new StreamReader( this, qsIP ) ),
      m_bStartup( true ),
      m_pMenuDialog( Q_NULLPTR ),
      m_qsIP( qsIP ),
      m_bPortrait( bPortrait ),
      m_iTimerSeconds( 0 ),
      m_bTimerActive( false ),
      m_iReconnectTimer( -1 ),
      m_iTimerTimer( -1 )
{
    setupUi( this );

    m_pAHRSDisp->setPortrait( bPortrait );

    connect( m_pMenuButton, SIGNAL( clicked() ), this, SLOT( menu() ) );

    m_lastStatusUpdate = QDateTime::currentDateTime();

    connect( m_pStratuxStream, SIGNAL( newSituation( StratuxSituation ) ), m_pAHRSDisp, SLOT( situation( StratuxSituation ) ) );
    connect( m_pStratuxStream, SIGNAL( newTraffic( int, StratuxTraffic ) ), m_pAHRSDisp, SLOT( traffic( int, StratuxTraffic ) ) );
    connect( m_pStratuxStream, SIGNAL( newStatus( bool, bool, bool, bool ) ), this, SLOT( statusUpdate( bool, bool, bool, bool ) ) );

    m_pStratuxStream->connectStreams();

    QTimer::singleShot( 500, this, SLOT( init() ) );

    QScreen *pScreen = QGuiApplication::primaryScreen();
    pScreen->setOrientationUpdateMask( Qt::PortraitOrientation | Qt::InvertedPortraitOrientation | Qt::LandscapeOrientation | Qt::InvertedLandscapeOrientation );
    connect( pScreen, SIGNAL( orientationChanged( Qt::ScreenOrientation ) ), this, SLOT( orient( Qt::ScreenOrientation ) ) );

    m_iReconnectTimer = startTimer( 5000 ); // Forever timer to periodically check if we need to reconnect
}


void AHRSMainWin::init()
{
    if( !m_bPortrait )
    {
        int iBtnHeight = static_cast<int>( static_cast<double>( height() ) * 0.04187 );
        int iIconSize = static_cast<int>( static_cast<double>( height() ) * 0.025 );

        m_pMenuButton->setMinimumHeight( iBtnHeight );
        m_pMenuButton->setMaximumHeight( iBtnHeight );
        m_pMenuButton->setIconSize( QSize( iIconSize, iIconSize ) );
        m_pStatusIndicator->setMaximumHeight( iBtnHeight );
        m_pAHRSIndicator->setMaximumHeight( iBtnHeight );
        m_pTrafficIndicator->setMaximumHeight( iBtnHeight );
        m_pGPSIndicator->setMaximumHeight( iBtnHeight );

        m_pMenuButton->setMinimumWidth( width() / 2 );
        m_pMenuButton->setMaximumWidth( width() / 2 );
    }
    else
    {
        int iBtnHeight = static_cast<int>( static_cast<double>( height() ) * 0.0375 );
        int iIconSize = static_cast<int>( static_cast<double>( height() ) * 0.02 );
        int iMenuBtnWidth = static_cast<int>( static_cast<double>( width() ) * 0.3125 );

        m_pMenuButton->setMinimumHeight( iBtnHeight );
        m_pMenuButton->setMaximumHeight( iBtnHeight );
        m_pMenuButton->setIconSize( QSize( iIconSize, iIconSize ) );
        m_pStatusIndicator->setMaximumHeight( iBtnHeight );
        m_pAHRSIndicator->setMaximumHeight( iBtnHeight );
        m_pTrafficIndicator->setMaximumHeight( iBtnHeight );
        m_pGPSIndicator->setMaximumHeight( iBtnHeight );

        m_pMenuButton->setMinimumWidth( iMenuBtnWidth );
        m_pMenuButton->setMaximumWidth( iMenuBtnWidth );
    }
#if defined( Q_OS_ANDROID )
    m_pAHRSDisp->orient( m_bPortrait );
#endif
}


// Delete the stream reader
AHRSMainWin::~AHRSMainWin()
{
    delete m_pStratuxStream;
    m_pStratuxStream = Q_NULLPTR;

    delete m_pAHRSDisp;
    m_pAHRSDisp = Q_NULLPTR;
}


// Status stream is received here instead of the canvas since here is where the indicators are
void AHRSMainWin::statusUpdate( bool bStratux, bool bAHRS, bool bGPS, bool bTraffic )
{
    QString qsOn( "QLabel { border: 2px solid black; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 white, stop:1 green ); }" );
    QString qsOff( "QLabel { border: 2px solid black; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 white, stop:1 red ); }" );

    m_pStatusIndicator->setStyleSheet( bStratux ? qsOn : qsOff );
    m_pAHRSIndicator->setStyleSheet( bAHRS ? qsOn : qsOff );
    m_pTrafficIndicator->setStyleSheet( bTraffic ? qsOn : qsOff );
    m_pGPSIndicator->setStyleSheet( bGPS ? qsOn : qsOff );

    m_lastStatusUpdate = QDateTime::currentDateTime();
}


// Display the menu dialog and handle specific returns
void AHRSMainWin::menu()
{
    if( m_pMenuDialog == Q_NULLPTR )
    {
        CanvasConstants c = m_pAHRSDisp->canvas()->constants();
        int             iW = static_cast<int>( c.dWa * (m_bPortrait ? 1.0 : 0.5) );
        int             iH = static_cast<int>( c.dH * (m_bPortrait ? 0.375 : 0.625) );

        m_pMenuDialog = new MenuDialog( this, m_bPortrait );

        // Scale the menu dialog according to screen resolution
        m_pMenuDialog->setMinimumWidth( iW );
        m_pMenuDialog->setMinimumHeight( iH );
        m_pMenuDialog->setGeometry( x(), y() + c.dH - iH, iW, iH );
        m_pMenuDialog->show();
        connect( m_pMenuDialog, SIGNAL( resetLevel() ), this, SLOT( resetLevel() ) );
        connect( m_pMenuDialog, SIGNAL( resetGMeter() ), this, SLOT( resetGMeter() ) );
        connect( m_pMenuDialog, SIGNAL( upgradeRosco() ), this, SLOT( upgradeRosco() ) );
        connect( m_pMenuDialog, SIGNAL( shutdownStratux() ), this, SLOT( shutdownStratux() ) );
        connect( m_pMenuDialog, SIGNAL( shutdownRoscoPi() ), this, SLOT( shutdownRoscoPi() ) );
        connect( m_pMenuDialog, SIGNAL( trafficToggled( bool ) ), this, SLOT( trafficToggled( bool ) ) );
        connect( m_pMenuDialog, SIGNAL( inOutToggled( bool ) ), this, SLOT( inOutToggled( bool ) ) );
        connect( m_pMenuDialog, SIGNAL( showAirports( Canvas::ShowAirports ) ), this, SLOT( showAirports( Canvas::ShowAirports ) ) );
        connect( m_pMenuDialog, SIGNAL( timer() ), this, SLOT( changeTimer() ) );
        connect( m_pMenuDialog, SIGNAL( fuelTanks( FuelTanks ) ), this, SLOT( fuelTanks( FuelTanks ) ) );
        connect( m_pMenuDialog, SIGNAL( stopFuelFlow() ), this, SLOT( stopFuelFlow() ) );
        connect( m_pMenuDialog, SIGNAL( unitsKnots() ), this, SLOT( unitsKnots() ) );
        connect( m_pMenuDialog, SIGNAL( dayMode() ), this, SLOT( dayMode() ) );
    }
    else
    {
        delete m_pMenuDialog;
        m_pMenuDialog = Q_NULLPTR;
    }
}


void AHRSMainWin::resetLevel()
{
    system( QString( "wget -q --post-data=\"\" http://%1/cageAHRS >/dev/null 2>&1" ).arg( m_qsIP ).toLatin1().data() );
    delete m_pMenuDialog;
    m_pMenuDialog = Q_NULLPTR;
}


void AHRSMainWin::resetGMeter()
{
    system( QString( "wget -q --post-data=\"\" http://%1/resetGMeter >/dev/null 2>&1" ).arg( m_qsIP ).toLatin1().data() );
    delete m_pMenuDialog;
    m_pMenuDialog = Q_NULLPTR;
}


void AHRSMainWin::upgradeRosco()
{
    delete m_pMenuDialog;
    m_pMenuDialog = Q_NULLPTR;
    if( QMessageBox::question( this, "UPGRADE", "Upgrading RoscoPi requires an active internet connection.\n\n"
                                                "Select 'Yes' to download and install the latest RoscoPi version.",
                               QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
    {
        system( "/home/pi/RoscoPi/upgrade.sh > /dev/null 2>&1 &" );
        QApplication::processEvents();
        qApp->closeAllWindows();
    }
}


void AHRSMainWin::shutdownRoscoPi()
{
    qApp->exit( 0 );
}


void AHRSMainWin::shutdownStratux()
{
    qApp->exit( 0 );
    system( "sudo shutdown -h now" );
}


// Use the escape key to close the app since there is no title bar
void AHRSMainWin::keyReleaseEvent( QKeyEvent *pEvent )
{
    if( pEvent->key() == Qt::Key_Escape )
        qApp->closeAllWindows();
    pEvent->accept();
}


void AHRSMainWin::timerEvent( QTimerEvent *pEvent )
{
    if( pEvent == Q_NULLPTR )
        return;

    // If we haven't gotten a status update for over ten seconds, force a reconnect
    if( pEvent->timerId() == m_iReconnectTimer )
    {
        if( m_lastStatusUpdate.secsTo( QDateTime::currentDateTime() ) > 10 )
        {
            m_pStratuxStream->disconnectStreams();
            m_pStratuxStream->connectStreams();
        }
    }
    else if( pEvent->timerId() == m_iTimerTimer )
    {
        QDateTime now = QDateTime::currentDateTime();
        int       iTimer = m_iTimerSeconds - static_cast<int>( m_timerStart.secsTo( now ) );
        int       iMinutes = iTimer / 60;
        int       iSeconds = iTimer - (iMinutes * 60);

        m_pAHRSDisp->timerReminder( iMinutes, iSeconds );
    }
}


void AHRSMainWin::trafficToggled( bool bAll )
{
    m_pAHRSDisp->showAllTraffic( bAll );
}


void AHRSMainWin::inOutToggled( bool bOut )
{
    m_pAHRSDisp->showOutside( bOut );
}


void AHRSMainWin::showAirports( Canvas::ShowAirports eShow )
{
    m_pAHRSDisp->showAirports( eShow );
}


void AHRSMainWin::changeTimer()
{
    Keypad          keypad( this, "TIMER", true );
    CanvasConstants c = m_pAHRSDisp->canvas()->constants();

    keypad.setGeometry( (m_bPortrait ? c.dW2 : c.dW + c.dW2) - (m_bPortrait ? c.dWa * 0.4167 : c.dWa * 0.25),
                        c.dH - (m_pAHRSDisp->m_pHeadIndicator->height() / 2) - 10 - static_cast<int>( m_bPortrait ? c.dH * 0.2 : c.dH * 0.3333 ),
                        m_bPortrait ? c.dH2 : c.dH * 0.8333, m_bPortrait ? c.dH * 0.4 : c.dH * 0.6667 );
    keypad.setMinimumSize( m_bPortrait ? c.dH2 : c.dH * 0.8333, m_bPortrait ? c.dH * 0.4 : c.dH * 0.6667 );

    delete m_pMenuDialog;
    m_pMenuDialog = Q_NULLPTR;

    if( keypad.exec() == QDialog::Accepted )
    {
        QString qsTime = keypad.textValue();
    
        m_timerStart = QDateTime::currentDateTime();
        m_iTimerSeconds = (qsTime.left( 2 ).toInt() * 60) + qsTime.right( 2 ).toInt();
        m_iTimerTimer = startTimer( 500 );
        m_bTimerActive = true;
    }
    else
        stopTimer();
}


void AHRSMainWin::restartTimer()
{
    m_timerStart = QDateTime::currentDateTime();
    m_bTimerActive = true;
}


void AHRSMainWin::stopTimer()
{
    if( m_iTimerTimer == -1 )
        killTimer( m_iTimerTimer );
    m_pAHRSDisp->timerReminder( -1, -1 );
    m_iTimerTimer = -1;
    m_bTimerActive = false;
}


void AHRSMainWin::orient( Qt::ScreenOrientation o )
{
    Q_UNUSED( o )

    qApp->exit( 1 );
}


void AHRSMainWin::fuelTanks( FuelTanks tanks )
{
    m_pAHRSDisp->setFuelTanks( tanks );
    m_pAHRSDisp->m_bFuelFlowStarted = true;
    QTimer::singleShot( 10, this, SLOT( fuelTanks2() ) );
}


void AHRSMainWin::fuelTanks2()
{
    delete m_pMenuDialog;
    m_pMenuDialog = Q_NULLPTR;
}


void AHRSMainWin::stopFuelFlow()
{
    m_pAHRSDisp->m_bFuelFlowStarted = false;
    QTimer::singleShot( 10, this, SLOT( fuelTanks2() ) );
}


void AHRSMainWin::unitsKnots()
{
    MenuDialog *pDlg = static_cast<MenuDialog *>( sender() );

    g_bUnitsKnots = (!g_bUnitsKnots);
    pDlg->m_pUnitsKnotsButton->setText( g_bUnitsKnots ? "KNOTS" : "MPH" );
    g_pSet->setValue( "UnitsKnots", g_bUnitsKnots );
    g_pSet->sync();
    m_pAHRSDisp->update();
}


void AHRSMainWin::dayMode()
{
    MenuDialog *pDlg = static_cast<MenuDialog *>( sender() );

    g_bDayMode = (!g_bDayMode);
    pDlg->m_pDayModeButton->setText( g_bDayMode ? "DAY MODE" : "NIGHT MODE" );
    m_pAHRSDisp->update();
}
