/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QPixmap>
#include <QPainter>
#include <QProcess>
#include <QtDebug>
#include <QDir>
#include <QMap>

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


void Builder::getStorage( QString *pInternal )
{
#if defined( Q_OS_ANDROID )
    // Get the location for internal storage depending on OS
    // Note this used to attempt to get the external path but there is a bug and the files aren't that big anyway.
    QStringList systemEnvironment = QProcess::systemEnvironment();
    QStringList env;
    QString     qsVar;

    foreach( qsVar, systemEnvironment )
    {
        env = qsVar.split( '=' );
        if( qsVar.contains( "ANDROID_DATA" ) )
        {
            *pInternal = env.last();
            break;
        }
    }
#else
    *pInternal = QDir::homePath() + "/stratofier_data";
#endif
}


void Builder::populateUrlMap( QMap<Canvas::CountryCode, QString> *pMap )
{
    pMap->insert( Canvas::AU, "australia_au" );
    pMap->insert( Canvas::AT, "austria_at" );
    pMap->insert( Canvas::BE, "belgium_be" );
    pMap->insert( Canvas::BR, "brazil_br" );
    pMap->insert( Canvas::CA, "canada_ca" );
    pMap->insert( Canvas::CL, "chile_cl" );
    pMap->insert( Canvas::CN, "china_cn" );
    pMap->insert( Canvas::CZ, "czech_republic_cz" );
    pMap->insert( Canvas::HU, "hungary_hu" );
    pMap->insert( Canvas::IS, "iceland_is" );
    pMap->insert( Canvas::IN, "india_in" );
    pMap->insert( Canvas::IL, "israel_il" );
    pMap->insert( Canvas::KE, "kenya_ke" );
    pMap->insert( Canvas::KR, "korea_kr" );
    pMap->insert( Canvas::NZ, "new_zealand_nz" );
    pMap->insert( Canvas::NE, "niger_ne" );
    pMap->insert( Canvas::PT, "portugal_pt" );
    pMap->insert( Canvas::PR, "rico_pr" );
    pMap->insert( Canvas::RU, "russian_federation_ru" );
    pMap->insert( Canvas::SE, "sweden_se" );
    pMap->insert( Canvas::UA, "ukraine_ua" );
    pMap->insert( Canvas::US, "united_states_us" );
}
