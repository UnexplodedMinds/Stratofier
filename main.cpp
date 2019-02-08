/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QtDebug>
#include <QDir>

#include "AHRSMainWin.h"
#if defined( Q_OS_ANDROID )
#include "ScreenLocker.h"
#endif

int main( int argc, char *argv[] )
{
    QApplication guiApp( argc, argv );
	QStringList  qslArgs = guiApp.arguments();
    QString      qsArg;
    QString      qsToken, qsVal;
	bool         bMax = true;
    QString      qsIP = "192.168.10.1";
    bool         bPortrait = true;
    AHRSMainWin *pMainWin = 0;
    QString      qsCurrWorkPath( "/home/pi/RoscoPi" );  // If you put RoscoPi anywhere else, specify home=<whatever> as an argument when running

#if defined( Q_OS_ANDROID )
    ScreenLocker locker;    // Keeps screen on until app exit where it's destroyed.
#endif

	foreach( qsArg, qslArgs )
    {
        QStringList qsl = qsArg.split( '=' );

        if( qsl.count() == 2 )
        {
            qsToken = qsl.first();
            qsVal = qsl.last();

            if( qsToken == "initdisp" )
                bMax = (qsVal == "max");
            else if( qsToken == "ip" )
                qsIP = qsVal;
            else if( qsToken == "orient" )
                bPortrait = (qsVal == "portrait");
            else if( qsToken == "home" )
                qsCurrWorkPath = qsVal;
        }
    }

// For Android, override any command line setting for portrait/landscape
#if defined( Q_OS_ANDROID )
    QScreen *pScreen = QGuiApplication::primaryScreen();

    QGuiApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    bPortrait = ((pScreen->orientation() == Qt::PortraitOrientation) || (pScreen->orientation() == Qt::InvertedPortraitOrientation));
// For running locally we need to set the correct working path so relative references work both locally and on the Pi
#else
    QDir::setCurrent( qsCurrWorkPath );
#endif

    QCoreApplication::setOrganizationName( "Unexploded Minds" );
    QCoreApplication::setOrganizationDomain( "unexplodedminds.com" );
    QCoreApplication::setApplicationName( "RoscoPi" );
    QGuiApplication::setApplicationDisplayName( "RoscoPi" );

    qInfo() << "Starting RoscoPi";
    pMainWin = new AHRSMainWin( qsIP, bPortrait );
    // This is the normal mode for a dedicated Raspberry Pi touchscreen or on Android
    if( bMax )
        pMainWin->showMaximized();
    // This is only intended for running directly on the host PC without an emulator
	else
	{
        pMainWin->show();
        if( bPortrait )
            pMainWin->resize( 480, 800 );
        else
            pMainWin->resize( 800, 480 );
	}

    while( guiApp.exec() != 0 )
    {
        // For Android, override any command line setting for portrait/landscape
#if defined( Q_OS_ANDROID )
        QScreen *pScreen = QGuiApplication::primaryScreen();

        bPortrait = ((pScreen->orientation() == Qt::PortraitOrientation) || (pScreen->orientation() == Qt::InvertedPortraitOrientation));
#endif
        delete pMainWin;
        pMainWin = new AHRSMainWin( qsIP, bPortrait );
        pMainWin->showMaximized();
    }

    return 0;
}
