/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QTimer>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

#include "AHRSMainWin.h"
#include "AHRSCanvas.h"
#include "StreamReader.h"
#include "MenuDialog.h"
#include "Canvas.h"
#include "Keypad.h"


QFont wee(   "Piboto", 8, QFont::Normal  );
QFont tiny(  "Piboto", 12, QFont::Normal );
QFont small( "Piboto", 16, QFont::Normal );
QFont med(   "Piboto", 18, QFont::Bold   );
QFont large( "Piboto", 24, QFont::Bold   );


// Setup minimal UI elements and make the connections
AHRSMainWin::AHRSMainWin( const QString &qsIP, bool bPortrait )
    : QMainWindow( 0, Qt::Window | Qt::FramelessWindowHint ),
      m_pStratuxStream( new StreamReader( this, qsIP ) ),
      m_bStartup( true ),
      m_pMenuDialog( 0 ),
      m_qsIP( qsIP ),
      m_bPortrait( bPortrait ),
      m_bTimerActive( false ),
      m_iReconnectTimer( -1 ),
      m_iTimerTimer( -1 ),
      m_iTimerSeconds( 0 )
{
    setupUi( this );

    m_pAHRSDisp->setPortrait( bPortrait );
    if( !bPortrait )
    {
        m_pMenuButton->setMinimumHeight( 20 );
        m_pMenuButton->setMaximumHeight( 20 );
        m_pMenuButton->setIconSize( QSize( 12, 12 ) );
        m_pStatusIndicator->setMaximumHeight( 20 );
        m_pAHRSIndicator->setMaximumHeight( 20 );
        m_pTrafficIndicator->setMaximumHeight( 20 );
        m_pGPSIndicator->setMaximumHeight( 20 );
    }
    else
    {
        m_pMenuButton->setMinimumHeight( 30 );
        m_pMenuButton->setMaximumHeight( 30 );
        m_pMenuButton->setIconSize( QSize( 16, 16 ) );
        m_pStatusIndicator->setMaximumHeight( 30 );
        m_pAHRSIndicator->setMaximumHeight( 30 );
        m_pTrafficIndicator->setMaximumHeight( 30 );
        m_pGPSIndicator->setMaximumHeight( 30 );
    }

    connect( m_pMenuButton, SIGNAL( clicked() ), this, SLOT( menu() ) );

    m_lastStatusUpdate = QDateTime::currentDateTime();

    connect( m_pStratuxStream, SIGNAL( newSituation( StratuxSituation ) ), m_pAHRSDisp, SLOT( situation( StratuxSituation ) ) );
    connect( m_pStratuxStream, SIGNAL( newTraffic( int, StratuxTraffic ) ), m_pAHRSDisp, SLOT( traffic( int, StratuxTraffic ) ) );
    connect( m_pStratuxStream, SIGNAL( newStatus( bool, bool, bool, bool ) ), this, SLOT( statusUpdate( bool, bool, bool, bool ) ) );

    m_pStratuxStream->connectStreams();

    QTimer::singleShot( 500, this, SLOT( init() ) );

    m_iReconnectTimer = startTimer( 5000 ); // Forever timer to periodically check if we need to reconnect
}


void AHRSMainWin::init()
{
    if( !m_bPortrait )
    {
        m_pMenuButton->setMinimumWidth( width() / 2 );
        m_pMenuButton->setMaximumWidth( width() / 2 );
    }
    else
    {
        m_pMenuButton->setMinimumWidth( 150 );
        m_pMenuButton->setMaximumWidth( 150 );
    }
}


// Delete the stream reader
AHRSMainWin::~AHRSMainWin()
{
    delete m_pStratuxStream;
    m_pStratuxStream = 0;

    delete m_pAHRSDisp;
    m_pAHRSDisp = 0;
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
    if( m_pMenuDialog == 0 )
    {
        m_pMenuDialog = new MenuDialog( this );

        m_pMenuDialog->setGeometry( x(), y() + height() - (m_bPortrait ? 330 : 320), 440, 300 );
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
    }
    else
    {
        delete m_pMenuDialog;
        m_pMenuDialog = 0;
    }
}


void AHRSMainWin::resetLevel()
{
    system( QString( "wget -q --post-data=\"\" http://%1/cageAHRS >/dev/null 2>&1" ).arg( m_qsIP ).toLatin1().data() );
    delete m_pMenuDialog;
    m_pMenuDialog = 0;
}


void AHRSMainWin::resetGMeter()
{
    system( QString( "wget -q --post-data=\"\" http://%1/resetGMeter >/dev/null 2>&1" ).arg( m_qsIP ).toLatin1().data() );
    delete m_pMenuDialog;
    m_pMenuDialog = 0;
}


void AHRSMainWin::upgradeRosco()
{
    delete m_pMenuDialog;
    m_pMenuDialog = 0;
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
    qApp->closeAllWindows();
}


void AHRSMainWin::shutdownStratux()
{
    qApp->closeAllWindows();
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
    if( pEvent == 0 )
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
        int       iTimer = m_iTimerSeconds - m_timerStart.secsTo( now );
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
    Keypad keypad( this, "TIMER", true );

    delete m_pMenuDialog;
    m_pMenuDialog = 0;

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
