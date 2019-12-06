/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <Canvas.h>
#include <QString>
#include <QMap>


class QPixmap;


class Builder
{
public:
    explicit Builder();

    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, int iNum, int iFieldWidth );
    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, const QString &qsNum );
    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, double dNum, int iPrec );

    static void getStorage( QString *pInternal );

    static void populateUrlMapAirports( QMap<Canvas::CountryCodeAirports, QString> *pMapAP );
    static void populateUrlMapAirspaces( QMap<Canvas::CountryCodeAirspace, QString> *pMapAS );
};

#endif // __BUILDER_H__
