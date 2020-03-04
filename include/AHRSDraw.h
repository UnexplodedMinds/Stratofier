/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __AHRSDRAW_H__
#define __AHRSDRAW_H__

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QList>
#include <QDateTime>

#include "StratuxStreams.h"
#include "Canvas.h"
#include "TrafficMath.h"


class AHRSDraw : public QWidget
{
    Q_OBJECT

public:
    explicit AHRSDraw( QPainter *pAHRS,
                       CanvasConstants *c,
                       Canvas *pCanvas,
                       Airport *pDirectAP,
                       Airport *pFromAP,
                       Airport *pToAP,
                       QList<Airport> *pAirports,
                       QList<Airspace> *pAirspaces,
                       double dZoomNM,
                       StratofierSettings *pSettings,
                       int iMagDev,
                       QPixmap *trafficRed,
                       QPixmap *trafficYellow,
                       QPixmap *trafficGreen,
                       QPixmap *trafficCyan,
                       QPixmap *trafficOrange );
    ~AHRSDraw();

    void drawDirectOrFromTo();
    void drawSlipSkid( double dSlipSkid );
    void drawCurrAlt( QPixmap *pNum );
    void drawCurrSpeed( QPixmap *pNum, bool bGS = false );
    void updateAirports();
    void updateAirspaces();
    void updateTraffic();
    void paintTemp();
    void paintSwitchNotice( FuelTanks *pTanks );
    void paintInfo();
    void paintTimer( int iTimerMin, int iTimerSec );

    QPainter           *m_pAHRS;
    CanvasConstants    *m_pC;
    Canvas             *m_pCanvas;
    Airport            *m_pDirectAP;
    Airport            *m_pFromAP;
    Airport            *m_pToAP;
    QList<Airport>     *m_pAirports;
    QList<Airspace>    *m_pAirspaces;
    double              m_dZoomNM;
    StratofierSettings *m_pSettings;
    int                 m_iMagDev;
    QPixmap            *m_trafficRed;
    QPixmap            *m_trafficYellow;
    QPixmap            *m_trafficGreen;
    QPixmap            *m_trafficCyan;
    QPixmap            *m_trafficOrange;
};

#endif // __AHRSDRAW_H__
