/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
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
#include <QPushButton>
#include <QSpacerItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QByteArray>

#include "AHRSMainWin.h"
#include "AHRSCanvas.h"
#include "StreamReader.h"
#include "MenuDialog.h"
#include "Canvas.h"
#include "Keypad.h"


extern QSettings *g_pSet;


// Standard fonts used throughout the app
QFont itsy(  "Droid Sans", 8,  QFont::Normal );
QFont wee(   "Droid Sans", 10, QFont::Normal );
QFont tiny(  "Droid Sans", 14, QFont::Normal );
QFont small( "Droid Sans", 16, QFont::Normal );
QFont med(   "Droid Sans", 18, QFont::Bold   );
QFont large( "Droid Sans", 24, QFont::Bold   );


Canvas::Units g_eUnitsAirspeed = Canvas::Knots;
bool          g_bDayMode = true;


// Setup minimal UI elements and make the connections
AHRSMainWin::AHRSMainWin( const QString &qsIP, bool bPortrait )
    : QMainWindow( Q_NULLPTR, Qt::Window | Qt::FramelessWindowHint ),
      m_pStratuxStream( new StreamReader( this, qsIP ) ),
      m_bStartup( true ),
      m_pMenuDialog( nullptr ),
      m_qsIP( qsIP ),
      m_bPortrait( bPortrait ),
      m_iTimerSeconds( 0 ),
      m_bTimerActive( false ),
      m_iReconnectTimer( -1 ),
      m_iTimerTimer( -1 )
{
    m_pStratuxStream->setUnits( static_cast<Canvas::Units>( g_pSet->value( "UnitsAirspeed" ).toInt() ) );

    itsy.setLetterSpacing( QFont::PercentageSpacing, 120.0 );
    wee.setLetterSpacing( QFont::PercentageSpacing, 120.0 );
    tiny.setLetterSpacing( QFont::PercentageSpacing, 120.0 );
    med.setLetterSpacing( QFont::PercentageSpacing, 120.0 );
    large.setLetterSpacing( QFont::PercentageSpacing, 120.0 );

    setupUi( this );

    m_pAHRSDisp->setPortrait( bPortrait );

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
#if defined( Q_OS_ANDROID )
    QScreen *pScreen = QGuiApplication::primaryScreen();
    QSizeF   physicalSize = pScreen->physicalSize();
    double   dWorH = 0;

    if( physicalSize.width() > physicalSize.height() )
        dWorH = physicalSize.width();
    else
        dWorH = physicalSize.height();

    m_pAHRSDisp->orient( m_bPortrait );
#endif

    m_pStatusIndicator->setMinimumHeight( height() * (m_bPortrait ? 0.0125 : 0.025) );
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
    QString qsOn( "QLabel { border: 2px solid black; background-color: LimeGreen; color: black; }" );
    QString qsOff( "QLabel { border: 2px solid black; background-color: LightCoral; color: black; }" );

    m_pStatusIndicator->setStyleSheet( bStratux ? qsOn : qsOff );
    m_pAHRSIndicator->setStyleSheet( bAHRS ? qsOn : qsOff );
    m_pTrafficIndicator->setStyleSheet( bTraffic ? qsOn : qsOff );
    m_pGPSIndicator->setStyleSheet( bGPS ? qsOn : qsOff );

    m_lastStatusUpdate = QDateTime::currentDateTime();
}


// Display the menu dialog and handle specific returns
void AHRSMainWin::menu()
{
    if( m_pMenuDialog == nullptr )
    {
        CanvasConstants c = m_pAHRSDisp->canvas()->constants();

        m_pMenuDialog = new MenuDialog( this, m_bPortrait );

        // Scale the menu dialog according to screen resolution
        m_pMenuDialog->setMinimumWidth( static_cast<int>( c.dW4 ) );
        m_pMenuDialog->setGeometry( c.dW - c.dW4, 0.0, static_cast<int>( c.dW4 ), static_cast<int>( c.dH ) );
        m_pMenuDialog->show();
        connect( m_pMenuDialog, SIGNAL( resetLevel() ), this, SLOT( resetLevel() ) );
        connect( m_pMenuDialog, SIGNAL( resetGMeter() ), this, SLOT( resetGMeter() ) );
        connect( m_pMenuDialog, SIGNAL( upgradeStratofier() ), this, SLOT( upgradeStratofier() ) );
        connect( m_pMenuDialog, SIGNAL( shutdownStratux() ), this, SLOT( shutdownStratux() ) );
        connect( m_pMenuDialog, SIGNAL( shutdownStratofier() ), this, SLOT( shutdownStratofier() ) );
        connect( m_pMenuDialog, SIGNAL( trafficToggled( bool ) ), this, SLOT( trafficToggled( bool ) ) );
        connect( m_pMenuDialog, SIGNAL( showAirports( Canvas::ShowAirports ) ), this, SLOT( showAirports( Canvas::ShowAirports ) ) );
        connect( m_pMenuDialog, SIGNAL( showRunways( bool ) ), this, SLOT( showRunways( bool ) ) );
        connect( m_pMenuDialog, SIGNAL( showAirspaces( bool ) ), this, SLOT( showAirspaces( bool ) ) );
        connect( m_pMenuDialog, SIGNAL( timer() ), this, SLOT( changeTimer() ) );
        connect( m_pMenuDialog, SIGNAL( fuelTanks( FuelTanks ) ), this, SLOT( fuelTanks( FuelTanks ) ) );
        connect( m_pMenuDialog, SIGNAL( stopFuelFlow() ), this, SLOT( stopFuelFlow() ) );
        connect( m_pMenuDialog, SIGNAL( unitsAirspeed() ), this, SLOT( unitsAirspeed() ) );
        connect( m_pMenuDialog, SIGNAL( dayMode() ), this, SLOT( dayMode() ) );
        connect( m_pMenuDialog, SIGNAL( setSwitchableTanks( bool ) ), this, SLOT( setSwitchableTanks( bool ) ) );
    }
    else
    {
        delete m_pMenuDialog;
        m_pMenuDialog = nullptr;
    }
}


void AHRSMainWin::resetLevel()
{
    emptyHttpPost( "cageAHRS" );
}


void AHRSMainWin::resetGMeter()
{
    emptyHttpPost( "resetGMeter" );
}


void AHRSMainWin::emptyHttpPost( const QString &qsToken )
{
    QNetworkAccessManager manager;

    manager.post( QNetworkRequest( QUrl( QString( "http://%1/%2" ).arg( m_qsIP ).arg( qsToken ) ) ), QByteArray() );

    QNetworkReply        *pResp = manager.get( QNetworkRequest( QUrl( QString( "http://%1/cageAHRS" ).arg( m_qsIP ) ) ) );
    QEventLoop            eventloop;

    connect( pResp, SIGNAL( finished() ), &eventloop, SLOT( quit() ) );
    eventloop.exec();

    QString throwaway = pResp->readAll();

    delete m_pMenuDialog;
    m_pMenuDialog = nullptr;
}


void AHRSMainWin::upgradeStratofier()
{
    delete m_pMenuDialog;
    m_pMenuDialog = nullptr;
    if( QMessageBox::question( this, "UPGRADE", "Upgrading Stratofier requires an active internet connection.\n\n"
                                                "Select 'Yes' to download and install the latest Stratofier version.",
                               QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
    {
        if( system( "/home/pi/Stratofier/upgrade.sh > /dev/null 2>&1 &" ) != 0 )
            qDebug() << "Error executing upgrade script.";
        QApplication::processEvents();
        qApp->closeAllWindows();
    }
}


void AHRSMainWin::shutdownStratofier()
{
    qApp->exit( 0 );
}


void AHRSMainWin::shutdownStratux()
{
    qApp->exit( 0 );
    if( system( "sudo shutdown -h now" ) != 0 )
        qDebug() << "Error shutting down.";
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


void AHRSMainWin::showAirports( Canvas::ShowAirports eShow )
{
    m_pAHRSDisp->showAirports( eShow );
}


void AHRSMainWin::showRunways( bool bShow )
{
    m_pAHRSDisp->showRunways( bShow );
}


void AHRSMainWin::showAirspaces( bool bShow )
{
    m_pAHRSDisp->showAirspaces( bShow );
}


void AHRSMainWin::changeTimer()
{
    Keypad keypad( this, "TIMER", true );

    m_pAHRSDisp->canvas()->setKeypadGeometry( &keypad );

    delete m_pMenuDialog;
    m_pMenuDialog = nullptr;

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

    QApplication::closeAllWindows();
    qApp->exit( 1 );
}


void AHRSMainWin::fuelTanks( FuelTanks tanks )
{
    if( !tanks.bDualTanks )
    {
        tanks.dRightCapacity = 0.0;
        tanks.dRightRemaining = 0.0;
    }
    m_pAHRSDisp->setFuelTanks( tanks );
    m_pAHRSDisp->m_bFuelFlowStarted = true;
    QTimer::singleShot( 10, this, SLOT( fuelTanks2() ) );
}


void AHRSMainWin::fuelTanks2()
{
    delete m_pMenuDialog;
    m_pMenuDialog = nullptr;
}


void AHRSMainWin::stopFuelFlow()
{
    m_pAHRSDisp->m_bFuelFlowStarted = false;
    QTimer::singleShot( 10, this, SLOT( fuelTanks2() ) );
}


void AHRSMainWin::unitsAirspeed()
{
    MenuDialog *pDlg = static_cast<MenuDialog *>( sender() );
    int         i = static_cast<int>( g_eUnitsAirspeed );

    i++;
    if( i > static_cast<int>( Canvas::KPH ) )
        i = static_cast<int>( Canvas::MPH );
    g_eUnitsAirspeed = static_cast<Canvas::Units>( i );
    if( pDlg != nullptr )
    {
        if( g_eUnitsAirspeed == Canvas::MPH )
            pDlg->m_pUnitsAirspeedButton->setText( "MPH" );
        else if( g_eUnitsAirspeed == Canvas::Knots )
            pDlg->m_pUnitsAirspeedButton->setText( "KNOTS" );
        else
            pDlg->m_pUnitsAirspeedButton->setText( "KPH" );
    }
    g_pSet->setValue( "UnitsAirspeed", static_cast<int>( g_eUnitsAirspeed ) );
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


void AHRSMainWin::setSwitchableTanks( bool bSwitchable )
{
    m_pAHRSDisp->setSwitchableTanks( bSwitchable );
}
