/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QApplication>
#include <QtDebug>

#include "AHRSMainWin.h"

bool g_bEmulated = false;


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

    QCoreApplication::setOrganizationName( "Unexploded Minds" );
    QCoreApplication::setOrganizationDomain( "unexplodedminds.com" );
    QCoreApplication::setApplicationName( "RoscoPi" );
    QGuiApplication::setApplicationDisplayName( "RoscoPi" );

    qInfo() << "Starting RoscoPi";
    AHRSMainWin mainWin( qsIP, bPortrait );
    if( bMax )
		mainWin.showMaximized();
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
