/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QPixmap>
#include <QPainter>
#include <QProcess>
#include <QtDebug>
#include <QDir>

#include "Builder.h"
#include "Canvas.h"


extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;


Builder::Builder()
{
}


void Builder::buildNumber( QPixmap *pNumber, CanvasConstants *c, int iNum, int iFieldWidth )
{
    pNumber->fill( Qt::transparent );

    QPainter        numPainter( pNumber );
    QString         qsNum = QString( "%1" ).arg( iNum, iFieldWidth, 10, QChar( '0' ) );
    QChar           cNum;
    int             iX = 0;

    foreach( cNum, qsNum )
    {
        QPixmap num( QString( ":/num/resources/%1.png" ).arg( cNum ) );
        numPainter.drawPixmap( iX, 0, c->dWNum, c->dHNum, num );
        iX += static_cast<int>( c->dWNum );
    }
}


void Builder::buildNumber( QPixmap *pNumber, CanvasConstants *c, const QString &qsNum )
{
    pNumber->fill( Qt::transparent );

    QPainter numPainter( pNumber );
    QChar    cNum;
    int      iX = 0;
    QString  qsDigit;

    foreach( cNum, qsNum )
    {
        if( cNum == ':' )
            qsDigit = ":/num/resources/colon.png";
        else
            qsDigit = QString( ":/num/resources/%1.png" ).arg( cNum );
        QPixmap num( qsDigit );
        numPainter.drawPixmap( iX, 0, c->dWNum, c->dHNum, num );
        iX += static_cast<int>( c->dWNum );
    }
}


void Builder::getStorage( QString *pInternal, QString *pExternal )
{
#if defined( Q_OS_ANDROID )
    // Get the locations for internal and external storage
    QStringList systemEnvironment = QProcess::systemEnvironment();
    QStringList env;
    QString     qsVar;

    foreach( qsVar, systemEnvironment )
    {
        env = qsVar.split( '=' );
        if( qsVar.contains( "EXTERNAL_STORAGE" ) )
        {
            *pExternal = env.last();
            QDir d( *pExternal );
            d.mkdir( "data" );
            qDebug() << d.entryList();
        }
        else if( qsVar.contains( "ANDROID_DATA" ) )
            *pInternal = env.last();
    }
#else
    *pInternal = QDir::homePath() + "/stratofier_data";
    *pExternal = QString();
#endif
}

