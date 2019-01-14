/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __BUILDER_H__
#define __BUILDER_H__


class Canvas;


class Builder
{
public:
    explicit Builder();

    static void buildRollIndicator( QPixmap *pRollInd, Canvas *pCanvas, bool bPortrait );
    static void buildHeadingIndicator( QPixmap *pHeadInd, Canvas *pCanvas );
    static void buildAltTape( QPixmap *pAltTape, Canvas *pCanvas );
    static void buildSpeedTape( QPixmap *pSpeedTape, Canvas *pCanvas );
    static void buildVertSpeedTape( QPixmap *pVertTape, Canvas *pCanvas, bool bPortrait );
};

#endif // __BUILDER_H__
