/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
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

//  static void buildVertSpeedTape( QPixmap *pVertTape, Canvas *pCanvas, bool bPortrait );
    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, int iNum, int iFieldWidth );
    static void buildNumber( QPixmap *pNumber, CanvasConstants *c, const QString &qsNum );
};

#endif // __BUILDER_H__
