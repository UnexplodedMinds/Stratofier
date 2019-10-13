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

    static void getStorage( QString *pInternal );

    static void populateUrlMap( QMap<Canvas::CountryCode, QString> *pMap );
};

#endif // __BUILDER_H__
