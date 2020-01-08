/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __AHRSCANVAS_H__
#define __AHRSCANVAS_H__

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QGestureEvent>
#include <QSwipeGesture>
#include <QPinchGesture>

#include "StratuxStreams.h"
#include "Canvas.h"
#include "TrafficMath.h"


class AHRSCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit AHRSCanvas( QWidget *parent = 0 );
    ~AHRSCanvas();

    void    setPortrait( bool bPortrait ) { m_bPortrait = bPortrait; }
    void    setFuelTanks( FuelTanks tanks ) { m_tanks = tanks; }
    void    timerReminder( int iMinutes, int iSeconds );
    Canvas *canvas() { return m_pCanvas; }
#if defined( Q_OS_ANDROID )
    void    orient( bool bPortrait );
#endif
    int     magDev() { return m_iMagDev; }
    void    setMagDev( int iMagDev );
    void    setSwitchableTanks( bool bSwitchable );

    bool m_bFuelFlowStarted;

public slots:
    void init();
    void situation( StratuxSituation s );
    void traffic( int iICAO, StratuxTraffic t );

    void showAllTraffic( bool bAll );
    void showAirports( Canvas::ShowAirports eShow );
    void showRunways( bool bShow );
    void showAirspaces( bool bShow );
    void showAltitudes( bool bShow );

protected:
    void paintEvent( QPaintEvent *pEvent );
    void mouseReleaseEvent( QMouseEvent *pEvent );
    void mousePressEvent( QMouseEvent *pEvent );
    void mouseMoveEvent( QMouseEvent *pEvent );
    void timerEvent( QTimerEvent *pEvent );

private:
    void updateTraffic( QPainter *pAhrs, CanvasConstants *c );
    void updateAirports( QPainter *pAhrs, CanvasConstants *c );
    void updateAirspaces( QPainter *pAhrs, CanvasConstants *c );
    void cullTrafficMap();
    void zoomIn();
    void zoomOut();
    void handleScreenPress( const QPoint &pressPt );
    void paintPortrait();
    void paintLandscape();
    void paintInfo( QPainter *pAhrs, CanvasConstants *c );
    void paintTimer( QPainter *pAhrs, CanvasConstants *c );
    void paintSwitchNotice( QPainter *pAhrs, CanvasConstants *c );
    void loadSettings();
    void drawDayMode( QPainter *pArs, CanvasConstants *c );
    void drawDirectOrFromTo( QPainter *pAhrs, CanvasConstants *pC );
    void drawSlipSkid( QPainter *pAhrs, CanvasConstants *pC, double dSlipSkid );
    void drawCurrAlt( QPainter *pAhrs, CanvasConstants *pC, QPixmap *pNum );
    void drawCurrSpeed( QPainter *pAhrs, CanvasConstants *pC, QPixmap *pNum, bool bGS = false );
    void swipeLeft();
    void swipeRight();
    void swipeUp();
    void swipeDown();
    const QString speedUnits();

    Canvas   *m_pCanvas;

    bool      m_bInitialized;
    QPixmap   m_planeIcon;
    QPixmap   m_headIcon;
    QPixmap   m_windIcon;
    QPixmap   m_directIcon;
    int       m_iHeadBugAngle;
    int       m_iWindBugAngle;
    int       m_iWindBugSpeed;
    int       m_iAltBug;
    int       m_iDispTimer;
    bool      m_bUpdated;
    bool      m_bShowGPSDetails;
    double    m_dZoomNM;
    bool      m_bPortrait;
    bool      m_bLongPress;
    QDateTime m_longPressStart;
    bool      m_bShowCrosswind;
    int       m_iTimerMin;
    int       m_iTimerSec;
    int       m_iMagDev;
    bool      m_bDisplayTanksSwitchNotice;
    QPoint    m_SwipeStart;
    int       m_iSwiping;
    Airport   m_directAP;
    Airport   m_fromAP;
    Airport   m_toAP;

    QPixmap m_HeadIndicator;
    QPixmap m_HeadIndicatorOverlay;
    QPixmap m_RollIndicator;
    QPixmap m_Lfuel;
    QPixmap m_Rfuel;
    QPixmap m_AltTape;
    QPixmap m_SpeedTape;
    QPixmap m_VertSpeedTape;
    QPixmap m_DirectTo;
    QPixmap m_AltBug;
    QPixmap m_FromTo;

    StratofierSettings m_settings;
    QList<Airport>     m_airports;
    QList<Airspace>    m_airspaces;
    FuelTanks          m_tanks;

    double m_dBaroPress;

#if defined( Q_OS_ANDROID )
private slots:
    void orient2();
#endif
};

#endif // __AHRSCANVAS_H__
