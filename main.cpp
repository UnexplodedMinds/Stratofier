/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QtDebug>

#include "AHRSMainWin.h"


int main( int argc, char *argv[] )
{
    QApplication guiApp( argc, argv );
	QStringList  qslArgs = guiApp.arguments();
    QString      qsArg;
    QString      qsToken, qsVal;
	bool         bMax = true;
    QString      qsIP = "192.168.10.1";
    bool         bPortrait = true;
    
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
        }
    }

    // For Android, override any command line setting for portrait/landscape
#ifdef ANDROID
    QScreen *pScreen = QGuiApplication::primaryScreen();

    bPortrait = ((pScreen->orientation() == Qt::PortraitOrientation) || (pScreen->orientation() == Qt::InvertedPortraitOrientation));
#endif

    QCoreApplication::setOrganizationName( "Unexploded Minds" );
    QCoreApplication::setOrganizationDomain( "unexplodedminds.com" );
    QCoreApplication::setApplicationName( "RoscoPi" );
    QGuiApplication::setApplicationDisplayName( "RoscoPi" );

    qInfo() << "Starting RoscoPi";
    AHRSMainWin mainWin( qsIP, bPortrait );
    // This is the normal mode for a dedicated Raspberry Pi touchscreen or on Android
    if( bMax )
		mainWin.showMaximized();
    // This is only intended for running directly on the host PC without an emulator
	else
	{
		mainWin.show();
        if( bPortrait )
    		mainWin.resize( 480, 800 );
        else
            mainWin.resize( 800, 480 );
	}

    return guiApp.exec();
}
