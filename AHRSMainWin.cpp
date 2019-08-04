/*
Stratofier Stratux AHRS Display
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
#if defined( Q_OS_ANDROID )
QFont itsy(  "Piboto", 8, QFont::Normal  );
QFont wee(   "Piboto", 10, QFont::Normal  );
QFont tiny(  "Piboto", 14, QFont::Normal );
#else
QFont itsy(  "Piboto", 6, QFont::Normal  );
QFont wee(   "Piboto", 8, QFont::Normal  );
QFont tiny(  "Piboto", 12, QFont::Normal );
#endif
QFont small( "Piboto", 16, QFont::Normal );
QFont med(   "Piboto", 18, QFont::Bold   );
QFont large( "Piboto", 24, QFont::Bold   );


bool g_bUnitsKnots = true;
bool g_bDayMode = true;
bool g_bTablet = false;


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

    if( (pScreen->logicalDotsPerInch() > 140) && (dWorH > 177) )
        g_bTablet = true;

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
    if( m_pMenuDialog == nullptr )
    {
        CanvasConstants c = m_pAHRSDisp->canvas()->constants();

        m_pMenuDialog = new MenuDialog( this, m_bPortrait );

        // Scale the menu dialog according to screen resolution
        m_pMenuDialog->setMinimumWidth( static_cast<int>( c.dW2 ) );
        m_pMenuDialog->setMinimumHeight( static_cast<int>( m_bPortrait ? c.dH2 : c.dH - c.dH5 ) );
        m_pMenuDialog->setGeometry( x() + c.dW - c.dW2, y(), static_cast<int>( c.dW2 ), static_cast<int>( c.dH ) );
        m_pMenuDialog->show();
        connect( m_pMenuDialog, SIGNAL( resetLevel() ), this, SLOT( resetLevel() ) );
        connect( m_pMenuDialog, SIGNAL( resetGMeter() ), this, SLOT( resetGMeter() ) );
        connect( m_pMenuDialog, SIGNAL( upgradeRosco() ), this, SLOT( upgradeRosco() ) );
        connect( m_pMenuDialog, SIGNAL( shutdownStratux() ), this, SLOT( shutdownStratux() ) );
        connect( m_pMenuDialog, SIGNAL( shutdownStratofier() ), this, SLOT( shutdownStratofier() ) );
        connect( m_pMenuDialog, SIGNAL( trafficToggled( bool ) ), this, SLOT( trafficToggled( bool ) ) );
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


void AHRSMainWin::upgradeRosco()
{
    delete m_pMenuDialog;
    m_pMenuDialog = nullptr;
    if( QMessageBox::question( this, "UPGRADE", "Upgrading Stratofier requires an active internet connection.\n\n"
                                                "Select 'Yes' to download and install the latest Stratofier version.",
                               QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
    {
        system( "/home/pi/Stratofier/upgrade.sh > /dev/null 2>&1 &" );
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


void AHRSMainWin::showAirports( Canvas::ShowAirports eShow )
{
    m_pAHRSDisp->showAirports( eShow );
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
