/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <Canvas.h>
#include <QString>


class QPixmap;


class Builder
{
public:
    explicit Builder();

    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, int iNum, int iFieldWidth );
    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, const QString &qsNum );

    static void getStorage( QString *pInternal, QString *pExternal );
};

#endif // __BUILDER_H__
