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

    QPainter numPainter( pNumber );
    QString  qsNum = QString( "%1" ).arg( iNum, iFieldWidth, 10, QChar( '0' ) );
    QChar    cNum;
    int      iX = 0;

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


void Builder::buildNumber(QPixmap *pNumber, CanvasConstants *c, double dNum, int iPrec )
{
    pNumber->fill( Qt::transparent );

    QPainter numPainter( pNumber );
    QString  qsNum = QString::number( dNum, 'f', iPrec );
    QChar    cNum;
    int      iX = 0;
    bool     bMantissa = false;
    int      iFull = static_cast<int>( c->dWNum );
    int      iHalf = static_cast<int>( c->dWNum / 1.5 );

    foreach( cNum, qsNum )
    {
        if( cNum == '.' )
        {
            bMantissa = true;
            continue;
        }

        QPixmap num( QString( ":/num/resources/%1b.png" ).arg( cNum ) );

        numPainter.drawPixmap( static_cast<double>( iX ), bMantissa ? (c->dHNum * 0.25) - 1.0 : 0.0, (bMantissa ? c->dWNum / 1.5 : c->dWNum), (bMantissa ? c->dHNum / 1.5 : c->dHNum), num );
        iX += (bMantissa ? iHalf : iFull);
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


void Builder::populateUrlMapAirports( QMap<Canvas::CountryCodeAirports, QString> *pMapAP )
{
    pMapAP->insert( Canvas::USap, "airports_united_states_us" );
    pMapAP->insert( Canvas::CAap, "airports_canada_ca" );
    pMapAP->insert( Canvas::AUap, "airports_australia_au" );
    pMapAP->insert( Canvas::ATap, "airports_austria_at" );
    pMapAP->insert( Canvas::BEap, "airports_belgium_be" );
    pMapAP->insert( Canvas::BRap, "airports_brazil_br" );
    pMapAP->insert( Canvas::CLap, "airports_chile_cl" );
    pMapAP->insert( Canvas::CNap, "airports_china_cn" );
    pMapAP->insert( Canvas::CZap, "airports_czech_republic_cz" );
    pMapAP->insert( Canvas::HUap, "airports_hungary_hu" );
    pMapAP->insert( Canvas::ISap, "airports_iceland_is" );
    pMapAP->insert( Canvas::INap, "airports_india_in" );
    pMapAP->insert( Canvas::ILap, "airports_israel_il" );
    pMapAP->insert( Canvas::KEap, "airports_kenya_ke" );
    pMapAP->insert( Canvas::KRap, "airports_korea_kr" );
    pMapAP->insert( Canvas::NZap, "airports_new_zealand_nz" );
    pMapAP->insert( Canvas::NEap, "airports_niger_ne" );
    pMapAP->insert( Canvas::PTap, "airports_portugal_pt" );
    pMapAP->insert( Canvas::PRap, "airports_rico_pr" );
    pMapAP->insert( Canvas::RUap, "airports_russian_federation_ru" );
    pMapAP->insert( Canvas::SEap, "airports_sweden_se" );
    pMapAP->insert( Canvas::UAap, "airports_ukraine_ua" );
}


void Builder::populateUrlMapAirspaces( QMap<Canvas::CountryCodeAirspace, QString> *pMapAS )
{
    pMapAS->insert( Canvas::USas, "airspace_united_states_us" );
    pMapAS->insert( Canvas::CAas, "airspace_canada_ca" );
    pMapAS->insert( Canvas::AUas, "airspace_austrailia_au" );
    pMapAS->insert( Canvas::BEas, "airspace_belgium_be" );
    pMapAS->insert( Canvas::BRas, "airspace_brazil_br" );
    pMapAS->insert( Canvas::CZas, "airspace_czech_republic_cz" );
    pMapAS->insert( Canvas::HUas, "airspace_hungary_hu" );
    pMapAS->insert( Canvas::ISas, "airspace_iceland_is" );
    pMapAS->insert( Canvas::NZas, "airspace_new_zealand_nz" );
    pMapAS->insert( Canvas::PTas, "airspace_portugal_pt" );
    pMapAS->insert( Canvas::SEas, "airspace_sweden_se" );
}
