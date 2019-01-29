/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __CANVAS_H__
#define __CANVAS_H__


struct CanvasConstants
{
    double dW;
    double dW2;
    double dW4;
    double dW5;
    double dW7;
    double dW10;
    double dW20;
	double dW30;
	double dW40;

    double dH;
    double dH2;
    double dH4;
    double dH5;
    double dH7;
    double dH10;
    double dH20;
	double dH30;
	double dH40;
    double dH160;

    double dAspectP;
    double dAspectL;

	int iWeeFontHeight;
    int iTinyFontHeight;
    int iSmallFontHeight;
    int iMedFontHeight;
    int iLargeFontHeight;

    int iTinyFontWidth;
};


struct BearingDist
{
    double dBearing;
    double dDistance;
};


struct Airport
{
    QString     qsID;
    QString     qsName;
    double      dLat;
    double      dLong;
    bool        bMilitary;
    bool        bPublic;
    BearingDist bd;
    QList<int>  runways;
};


class Canvas
{
public:
    enum ShowAirports
    {
        ShowNoAirports,
        ShowPublicAirports,
        ShowAllAirports
    };

    Canvas( double dWidth, double dHeight, bool bPortrait );

    CanvasConstants contants();
    int             largeWidth( const QString &qsText );
	int             medWidth( const QString &qsText );

private:
    CanvasConstants m_preCalc;
    bool            m_bPortrait;
};


#endif // __CANVAS_H__

