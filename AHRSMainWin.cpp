/*
Stratux AHRS Display
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


QFont wee(   "Piboto", 8, QFont::Normal  );
QFont tiny(  "Piboto", 12, QFont::Normal );
QFont small( "Piboto", 16, QFont::Normal );
QFont med(   "Piboto", 18, QFont::Bold   );
QFont large( "Piboto", 24, QFont::Bold   );


// Setup minimal UI elements and make the connections
AHRSMainWin::AHRSMainWin( const QString &qsIP )
    : QMainWindow( 0, Qt::Window | Qt::FramelessWindowHint ),
      m_pStratuxStream( new StreamReader( this, qsIP ) ),
      m_bStartup( true ),
      m_pMenuDialog( 0 ),
      m_qsIP( qsIP )
{
    setupUi( this );

    connect( m_pMenuButton, SIGNAL( clicked() ), this, SLOT( menu() ) );

    m_lastStatusUpdate = QDateTime::currentDateTime();

    connect( m_pStratuxStream, SIGNAL( newSituation( StratuxSituation ) ), m_pAHRSDisp, SLOT( situation( StratuxSituation ) ) );
    connect( m_pStratuxStream, SIGNAL( newTraffic( int, StratuxTraffic ) ), m_pAHRSDisp, SLOT( traffic( int, StratuxTraffic ) ) );
    connect( m_pStratuxStream, SIGNAL( newStatus( bool, bool, bool, bool ) ), this, SLOT( statusUpdate( bool, bool, bool, bool ) ) );

    m_pStratuxStream->connectStreams();

    // We don't care what the ID is since it's the one and only timer for this class and never gets killed
    startTimer( 5000 );
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
    QString qsOn( "QLabel { border: 5px solid black; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 white, stop:1 green ); }" );
    QString qsOff( "QLabel { border: 5px solid black; background-color: qlineargradient( x1:0, y1:0, x2:0, y2:1, stop: 0 white, stop:1 red ); }" );

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

        m_pMenuDialog->setGeometry( x(), y() + height() - 290, 220, 260 );
        m_pMenuDialog->show();
        connect( m_pMenuDialog, SIGNAL( resetLevel() ), this, SLOT( resetLevel() ) );
        connect( m_pMenuDialog, SIGNAL( resetGMeter() ), this, SLOT( resetGMeter() ) );
        connect( m_pMenuDialog, SIGNAL( upgradeRosco() ), this, SLOT( upgradeRosco() ) );
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
    if( QMessageBox::question( this, "UPGRADE", "Upgrading RoscoPi requires an active network connection.\n\n"
                                                "Select 'OK' to download and install the latest RoscoPi version.",
                               QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
    {
        system( "/home/pi/RoscoPi/upgrade.sh > /dev/null 2>&1" );
    }
}


void AHRSMainWin::menuRejected()
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
    if( m_lastStatusUpdate.secsTo( QDateTime::currentDateTime() ) > 10 )
    {
        m_pStratuxStream->disconnectStreams();
        m_pStratuxStream->connectStreams();
    }
}

