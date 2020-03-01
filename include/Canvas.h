/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include <QList>
#include <QDataStream>
#include <QDateTime>
#include <QPolygonF>


class Keypad;


struct CanvasConstants
{
    double dW;
    double dWa;
    double dW2;
    double dW4;
    double dW5;
    double dW7;
    double dW10;
    double dW20;
	double dW30;
	double dW40;
    double dW80;

    double dH;
    double dH2;
    double dH4;
    double dH5;
    double dH7;
    double dH8;
    double dH10;
    double dH20;
	double dH30;
	double dH40;
    double dH80;
    double dH100;
    double dH160;

    double dHNum;
    double dWNum;

    double dAspectP;
    double dAspectL;

	int iWeeFontHeight;
    int iTinyFontHeight;
    int iSmallFontHeight;
    int iMedFontHeight;
    int iLargeFontHeight;
    int iHugeFontHeight;

    int iTinyFontWidth;

    int iThinPen;
    int iThickPen;
    int iFatPen;

    double dHeadDiam;
    double dHeadDiam2;

    bool bPortrait;
};


struct BearingDist
{
    double dBearing;
    double dDistance;
};


struct Frequency
{
    QString qsDescription;
    double  dFreq;
};


struct Airport
{
    QString          qsID;
    QString          qsName;
    double           dLat;
    double           dLong;
    double           dElev;
    bool             bGrass;
    BearingDist      bd;
    QList<int>       runways;
    QList<Frequency> frequencies;
};


struct FuelTanks
{
    double    dLeftCapacity;
    double    dRightCapacity;
    double    dLeftRemaining;
    double    dRightRemaining;
    double    dFuelRateCruise;
    double    dFuelRateClimb;
    double    dFuelRateDescent;
    double    dFuelRateTaxi;
    int       iSwitchIntervalMins;
    bool      bDualTanks;
    bool      bOnLeftTank;
    QDateTime lastSwitch;
};


class Canvas
{
public:
    enum AirspaceType
    {
        Airspace_Class_G,
        Airspace_Class_E,
        Airspace_Class_D,
        Airspace_Class_C,
        Airspace_Class_B,
        Airspace_TFR,
        Airspace_SFRA,
        Airspace_MOA,
        Airspace_Restricted,
        Airspace_Prohibited,
        Airspace_Danger,        // See comment in TrafficMath::cacheAirspaces() about this airspace type
        Airspace_Unknown
    };

    enum CountryCodeAirports
    {
        USap,
        CAap,
        AUap,
        ATap,
        BEap,
        BRap,
        CLap,
        CNap,
        CZap,
        HUap,
        ISap,
        INap,
        ILap,
        KEap,
        KRap,
        NZap,
        NEap,
        PTap,
        PRap,
        RUap,
        SEap,
        UAap
    };

    enum CountryCodeAirspace
    {
        USas,
        CAas,
        AUas,
        BEas,
        BRas,
        CZas,
        HUas,
        ISas,
        NZas,
        PTas,
        SEas
    };

    enum ShowAirports
    {
        ShowNoAirports,
        ShowGrassAirports,
        ShowPavedAirports,
        ShowAllAirports
    };

    enum Units
    {
        MPH,
        Knots,
        KPH
    };

    Canvas( double dWidth, double dHeight, bool bPortrait );

    void init( double dWidth, double dHeight, bool bPortrait );

    CanvasConstants constants();
    double          scaledH( double d );    // Simplify scalars for constants used for known 480x800/800x480 screen size
    double          scaledV( double d );    // Same for vertical
    int             largeWidth( const QString &qsText );
	int             medWidth( const QString &qsText );

    void setKeypadGeometry( Keypad *pKeypad );

private:
    CanvasConstants m_preCalc;
};


struct StratofierSettings
{
    Canvas::ShowAirports       eShowAirports;
    bool                       bShowPrivate;
    bool                       bShowAllTraffic;
    int                        iCurrDataSet;
    bool                       bSwitchableTanks;
    QString                    qsStratuxIP;
    bool                       bShowRunways;
    bool                       bShowAirspaces;
    bool                       bShowAltitudes;
    Canvas::Units              eUnits;
    QList<Canvas::CountryCodeAirports> listAirports;
    QList<Canvas::CountryCodeAirspace> listAirspaces;
    QString                    qsOwnshipID;
    int                        iMagDev;
    bool                       bWTScreenStayOn;
    double                     dAirspeedCal;
};


struct Airspace
{
    Canvas::AirspaceType eType;
    QString              qsName;
    int                  iAltTop;
    int                  iAltBottom;
    QPolygonF            shape;
    QList<BearingDist>   shapeHav;
};

#endif // __CANVAS_H__

