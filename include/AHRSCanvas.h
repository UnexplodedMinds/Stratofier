/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __AHRSCANVAS_H__
#define __AHRSCANVAS_H__

#include <QWidget>
#include <QPixmap>
#include <QMap>

#include "StratuxStreams.h"
#include "Canvas.h"


class QDial;


class AHRSCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit AHRSCanvas( QWidget *parent = 0 );
    ~AHRSCanvas();

public slots:
    void init();
    void situation( StratuxSituation s );
    void traffic( int iICAO, StratuxTraffic t );

protected:
    void paintEvent( QPaintEvent *pEvent );
    void mousePressEvent( QMouseEvent *pEvent );
    void timerEvent( QTimerEvent *pEvent );

private:
    void updateTraffic( QPainter *pAhrs, CanvasConstants *c );
    void cullTrafficMap();
    void zoomIn();
    void zoomOut();

    Canvas *m_pCanvas;

    bool     m_bInitialized;
    QPixmap  m_planeIcon;
    QPixmap  m_headIcon;
    QPixmap  m_windIcon;
    int      m_iHeadBugAngle;
    int      m_iWindBugAngle;
    QPixmap *m_pRollIndicator;
    QPixmap *m_pHeadIndicator;
    QPixmap *m_pAltTape;
    QPixmap *m_pSpeedTape;
    QPixmap *m_pVertSpeedTape;
    QPixmap *m_pZoomInPixmap;
    QPixmap *m_pZoomOutPixmap;
    double   m_dDPIMult;
    bool     m_iDispTimer;
    bool     m_bUpdated;
    bool     m_bShowGPSDetails;
    double   m_dZoomNM;
};

#endif // __AHRSCANVAS_H__
