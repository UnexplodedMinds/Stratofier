/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QPainter>
#include <QtDebug>
#include <QMouseEvent>
#include <QTimer>
#include <QFont>
#include <QLinearGradient>
#include <QLineF>
#include <QSettings>
#include <QtConcurrent>
#include <QTransform>
#include <QVariant>
#include <QScreen>
#include <QGesture>
#include <QGestureEvent>
#include <QSwipeGesture>
#include <QPinchGesture>
#include <QBitmap>

#include <math.h>

#include "AHRSCanvas.h"
#include "BugSelector.h"
#include "Keypad.h"
#include "AHRSMainWin.h"
#include "StreamReader.h"
#include "Builder.h"
#include "StratofierDefs.h"
#include "TimerDialog.h"
#include "AirportDialog.h"


extern QFont itsy;
extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;

extern bool g_bUnitsKnots;
extern bool g_bDayMode;

extern QList<Airport> g_airportCache;

extern QSettings *g_pSet;

StratuxSituation          g_situation;
QMap<int, StratuxTraffic> g_trafficMap;

QString g_qsStratofierVersion( "1.4.1.1" );


/*
IMPORTANT NOTE:

Wherever the dH constant is used for scaling, even for width or x offset, was deliberate, since
that constant is always the same regardless of whether we're in landscape or portrait mode.  The
dWa constant is used differently which is why the more convenient dH is used instead.
*/


AHRSCanvas::AHRSCanvas( QWidget *parent )
    : QWidget( parent ),
      m_bFuelFlowStarted( false ),
      m_pCanvas( Q_NULLPTR ),
      m_bInitialized( false ),
      m_iHeadBugAngle( -1 ),
      m_iWindBugAngle( -1 ),
      m_iWindBugSpeed( 0 ),
      m_pMagHeadOffLessPixmap( Q_NULLPTR ),
      m_pMagHeadOffMorePixmap( Q_NULLPTR ),
      m_bUpdated( false ),
      m_bShowGPSDetails( false ),
      m_bPortrait( true ),
      m_bLongPress( false ),
      m_longPressStart( QDateTime::currentDateTime() ),
      m_bShowCrosswind( false ),
      m_iTimerMin( -1 ),
      m_iTimerSec( -1 ),
      m_bDisplayTanksSwitchNotice( false ),
      m_iMagDev( 0 ),
      m_tanks( { 0.0, 0.0, 0.0, 0.0, 9.0, 10.0, 8.0, 5.0, 30, true, true, QDateTime::currentDateTime() } ),
      m_SwipeStart( 0, 0 ),
      m_iSwiping( 0 ),
      m_iAltBug( -1 )
{
    m_directAP.qsID = "NULL";
    m_directAP.qsName = "NULL";
    m_fromAP.qsID = "NULL";
    m_fromAP.qsName = "NULL";
    m_toAP.qsID = "NULL";
    m_toAP.qsName = "NULL";

    // Initialize AHRS settings
    // No need to init the traffic because it starts out as an empty QMap.
    StreamReader::initSituation( g_situation );

    // Preload the fancier icons that are impractical to paint programmatically
    m_planeIcon.load( ":/graphics/resources/Plane.png" );
    m_headIcon.load( ":/icons/resources/HeadingIcon.png" );
    m_windIcon.load( ":/icons/resources/WindIcon.png" );
    m_DirectTo.load( ":/graphics/resources/DirectTo.png" );
    m_FromTo.load( ":/graphics/resources/FromTo.png" );
    m_AltBug.load( ":/icons/resources/AltBug.png" );

    loadSettings();

    // Quick and dirty way to ensure we're shown full screen before any calculations happen
    QTimer::singleShot( 1500, this, SLOT( init() ) );
}


// Delete everything that needs deleting
AHRSCanvas::~AHRSCanvas()
{
    if( g_pSet != Q_NULLPTR )
    {
        g_pSet->sync();
        delete g_pSet;
        g_pSet = Q_NULLPTR;
    }
    if( m_pCanvas != Q_NULLPTR )
    {
        delete m_pCanvas;
        m_pCanvas = Q_NULLPTR;
    }
    if( m_pMagHeadOffLessPixmap != Q_NULLPTR )
    {
        delete m_pMagHeadOffLessPixmap;
        m_pMagHeadOffLessPixmap = Q_NULLPTR;
    }
    if( m_pMagHeadOffMorePixmap != Q_NULLPTR )
    {
        delete m_pMagHeadOffMorePixmap;
        m_pMagHeadOffMorePixmap = Q_NULLPTR;
    }
}


// Create the canvas utility instance, create the various pixmaps that are built up for fast painting
// and start the update timer.
void AHRSCanvas::init()
{
    if( m_pCanvas != Q_NULLPTR )
        delete m_pCanvas;

    m_pCanvas = new Canvas( width(), height(), m_bPortrait );

    CanvasConstants c = m_pCanvas->constants();
    int             iBugSize = static_cast<int>( c.dWa * (m_bPortrait ? 0.1333 : 0.08) );

    m_headIcon = m_headIcon.scaled( iBugSize, iBugSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    m_windIcon = m_windIcon.scaled( iBugSize, iBugSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    if( m_bPortrait )
        m_VertSpeedTape.load( ":/graphics/resources/vspeedP.png" );
    else
        m_VertSpeedTape.load( ":/graphics/resources/vspeedL.png" );

    m_HeadIndicator.load( ":/graphics/resources/head.png" );
    m_RollIndicator.load( ":/graphics/resources/roll.png" );
    m_AltTape.load( ":/graphics/resources/alttape.png" );
    m_Lfuel.load( ":/graphics/resources/fuel.png" );
    m_SpeedTape.load( ":/graphics/resources/speedtape.png" );
    m_AltTape = m_AltTape.scaledToWidth( c.dW10, Qt::SmoothTransformation );
    m_SpeedTape = m_SpeedTape.scaledToWidth( c.dW10 - c.dW40, Qt::SmoothTransformation );
    QTransform flipper;
    flipper.scale( -1, 1 );
    m_Rfuel = m_Lfuel.transformed( flipper );

    QPixmap zIn( ":/graphics/resources/ZoomIn.png" );
    QPixmap zOut( ":/graphics/resources/ZoomOut.png" );
    QTransform trans;

    trans.rotate( -90.0 );
    m_pMagHeadOffLessPixmap = new QPixmap( zIn.transformed( trans ).scaled( c.iMedFontHeight, c.iMedFontHeight ) );
    trans.rotate( 360.0 );
    m_pMagHeadOffMorePixmap = new QPixmap( zOut.transformed( trans ).scaled( c.iMedFontHeight, c.iMedFontHeight ) );

    m_iDispTimer = startTimer( 5000 );     // Update the in-memory airspace objects every 15 seconds

    QtConcurrent::run( TrafficMath::cacheAirports );

    m_bInitialized = true;
}


void AHRSCanvas::loadSettings()
{
    m_dZoomNM = g_pSet->value( "ZoomNM", 10.0 ).toDouble();
    m_settings.bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();
    m_settings.eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", 2 ).toInt() );
    m_settings.bShowRunways = g_pSet->value( "ShowRunways", true ).toBool();
    m_iMagDev = g_pSet->value( "MagDev", 0.0 ).toInt();
    g_bUnitsKnots = g_pSet->value( "UnitsKnots", true ).toBool();

    g_pSet->beginGroup( "FuelTanks" );
    m_tanks.dLeftCapacity = g_pSet->value( "LeftCapacity", 24.0 ).toDouble();
    m_tanks.dRightCapacity = g_pSet->value( "RightCapacity", 24.0 ).toDouble();
    m_tanks.dLeftRemaining = g_pSet->value( "LeftRemaining", 24.0 ).toDouble();
    m_tanks.dRightRemaining = g_pSet->value( "RightRemaining", 24.0 ).toDouble();
    m_tanks.dFuelRateCruise = g_pSet->value( "CruiseRate", 8.3 ).toDouble();
    m_tanks.dFuelRateClimb = g_pSet->value( "ClimbRate", 9.0 ).toDouble();
    m_tanks.dFuelRateDescent = g_pSet->value( "DescentRate", 7.0 ).toDouble();
    m_tanks.dFuelRateTaxi = g_pSet->value( "TaxiRate", 4.0 ).toDouble();
    m_tanks.iSwitchIntervalMins = g_pSet->value( "SwitchInterval", 15 ).toInt();
    g_pSet->endGroup();
}


// Just a utility timer that periodically updates the display when it's not being driven by the streams
// coming from the Stratux.
void AHRSCanvas::timerEvent( QTimerEvent *pEvent )
{
    if( pEvent == 0 )
        return;

    QDateTime qdtNow = QDateTime::currentDateTime();

    m_settings.eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", 2 ).toInt() );
    update();

    cullTrafficMap();

    // If we have a valid GPS position, run the list of airports within range by threading it so it doesn't interfere with the display update
    if( (g_situation.dGPSlat != 0.0) && (g_situation.dGPSlong != 0.0) )
        QtConcurrent::run( TrafficMath::updateNearbyAirports, &m_airports, m_dZoomNM );

    if( m_bFuelFlowStarted )
    {
        double dInterval = 0.00416666666667;   // 15 seconds / 3600 seconds (1 hour); 15 seconds is the interval of the one and only timer

        // If the aircraft is moving slowly and there is virtually no vertical movement then we are taxiing
        if( (g_situation.dGPSGroundSpeed > 5.0) && (g_situation.dGPSGroundSpeed < 20.0) && (fabs( g_situation.dBaroVertSpeed ) < 5.0) )
        {
            if( m_tanks.bOnLeftTank || (!m_tanks.bDualTanks) )
                m_tanks.dLeftRemaining -= (m_tanks.dFuelRateTaxi * dInterval);
            else
                m_tanks.dRightRemaining -= (m_tanks.dFuelRateTaxi * dInterval);
        }
        // If we're moving faster than 35 knots and we're at least anemically climbing, then use the climb rate
        else if( (g_situation.dGPSGroundSpeed > 35.0) && (g_situation.dBaroVertSpeed > 50.0) )
        {
            if( m_tanks.bOnLeftTank || (!m_tanks.bDualTanks) )
                m_tanks.dLeftRemaining -= (m_tanks.dFuelRateClimb * dInterval);
            else
                m_tanks.dRightRemaining -= (m_tanks.dFuelRateClimb * dInterval);
        }
        // If we're at least at an anemic airspeed and reasonbly acceptable altitude control, then use the cruise rate
        else if( (g_situation.dGPSGroundSpeed > 70.0) && (g_situation.dBaroVertSpeed < 100.0) )
        {
            if( m_tanks.bOnLeftTank || (!m_tanks.bDualTanks) )
                m_tanks.dLeftRemaining -= (m_tanks.dFuelRateCruise * dInterval);
            else
                m_tanks.dRightRemaining -= (m_tanks.dFuelRateCruise * dInterval);
        }
        // If we're in at least a slow descent, use the descent rate
        else if( (g_situation.dGPSGroundSpeed > 70.0) && (g_situation.dBaroVertSpeed < -250.0) )
        {
            if( m_tanks.bOnLeftTank || (!m_tanks.bDualTanks) )
                m_tanks.dLeftRemaining -= (m_tanks.dFuelRateDescent * dInterval);
            else
                m_tanks.dRightRemaining -= (m_tanks.dFuelRateDescent * dInterval);
        }

        if( (m_tanks.lastSwitch.secsTo( qdtNow ) > (m_tanks.iSwitchIntervalMins * 60)) && m_tanks.bDualTanks )
            m_bDisplayTanksSwitchNotice = true;
    }

    m_bUpdated = false;
}


// Where all the magic happens
void AHRSCanvas::paintEvent( QPaintEvent *pEvent )
{
    if( (!m_bInitialized) || (pEvent == 0) )
        return;

    if( m_bPortrait )
        paintPortrait();
    else
        paintLandscape();
}


// Draw the traffic onto the heading indicator and the tail numbers on the side
void AHRSCanvas::updateTraffic( QPainter *pAhrs, CanvasConstants *c )
{
    QList<StratuxTraffic> trafficList = g_trafficMap.values();
    StratuxTraffic        traffic;
    QPen                  planePen( Qt::black, 15, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin );
    double				  dPxPerNM = static_cast<double>( c->dW - 20.0 ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
	QLineF				  track, ball;
	double                dAlt;
	QString               qsSign;
    QFontMetrics          smallMetrics( small );
    int                   iBallPenWidth = static_cast<int>( c->dH * (m_bPortrait ? 0.01875 : 0.03125) );
    int                   iCourseLinePenWidth = static_cast<int>( c->dH * (m_bPortrait ? 0.00625 : 0.010417) );
    int                   iCourseLineLength = static_cast<int>( c->dH * (m_bPortrait ? 0.0375 : 0.0625) );
    QPainterPath          maskPath;

    maskPath.addEllipse( 10.0 + (m_bPortrait ? 0 : c->dW),
                         c->dH - c->dW,
                         c->dW - c->dW20, c->dW - c->dW20 );
    pAhrs->setClipPath( maskPath );

    // Draw a large dot for each aircraft; the outer edge of the heading indicator is calibrated to be 20 NM out from your position
	foreach( traffic, trafficList )
    {
        // If bearing and distance were able to be calculated then show relative position
        if( traffic.bHasADSB )
        {
			double dTrafficDist = traffic.dDist * dPxPerNM;
            double dAltDist = traffic.dAlt - g_situation.dBaroPressAlt;

            if( m_settings.bShowAllTraffic || (fabs( dAltDist ) < 5000) )
            {
                ball.setP1( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - c->dW2 - 30.0 ) );
                ball.setP2( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - c->dW2 - 30.0 - dTrafficDist ) );

			    // Traffic angle in reference to you (which clock position they're at)
                ball.setAngle( -(traffic.dBearing + g_situation.dAHRSGyroHeading) + 90.0 );

			    // Draw the black part of the track line
                planePen.setWidth( static_cast<int>( c->dH * (m_bPortrait ? 0.00625 : 0.010417) ) );
                planePen.setColor( Qt::black );
			    track.setP1( ball.p2() );
                track.setP2( QPointF( ball.p2().x(), ball.p2().y() + iCourseLineLength ) );
                track.setAngle( -(traffic.dTrack + g_situation.dAHRSGyroHeading) + 90.0 );
                pAhrs->setPen( planePen );
			    pAhrs->drawLine( track );

                // Draw the dot
                planePen.setWidth( iBallPenWidth );
                planePen.setColor( Qt::black );
                pAhrs->setPen( planePen );
                pAhrs->drawPoint( ball.p2() );
                planePen.setColor( Qt::green );
                pAhrs->setPen( planePen );
                pAhrs->drawPoint( ball.p2().x() - 2, ball.p2().y() - 2 );

                // Draw the green part of the track line
                planePen.setWidth( iCourseLinePenWidth );
                planePen.setColor( Qt::green );
                track.setP1( QPointF( ball.p2().x() - 2, ball.p2().y() - 2 ) );
                track.setP2( QPointF( ball.p2().x() - 2, ball.p2().y() + iCourseLineLength - 2 ) );
                track.setAngle( -(traffic.dTrack + g_situation.dAHRSGyroHeading) + 90.0 );
                pAhrs->setPen( planePen );
                pAhrs->drawLine( track );

			    // Draw the ID, numerical track heading and altitude delta
			    dAlt = (traffic.dAlt - g_situation.dBaroPressAlt) / 100.0;
			    if( dAlt > 0 )
				    qsSign = "+";
			    else if( dAlt < 0 )
				    qsSign = "-";
			    pAhrs->setPen( Qt::black );
                pAhrs->setFont( wee );
                pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0, traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
                pAhrs->setFont( tiny );
#if defined( Q_OS_ANDROID )
                pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + (c->iWeeFontHeight / 2.0), QString::number( static_cast<int>( traffic.dTrack ) ) );
                pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + c->iWeeFontHeight, QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
#else
                pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + c->iWeeFontHeight, QString::number( static_cast<int>( traffic.dTrack ) ) );
                pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + (c->iWeeFontHeight * 2.0), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
#endif
			    pAhrs->setPen( Qt::green );
                pAhrs->setFont( wee );
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0, traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
                pAhrs->setFont( tiny );
#if defined( Q_OS_ANDROID )
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + (c->iWeeFontHeight / 2.0), QString::number( static_cast<int>( traffic.dTrack ) ) );
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + c->iWeeFontHeight, QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
#else
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + c->iWeeFontHeight, QString::number( static_cast<int>( traffic.dTrack ) ) );
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + (c->iWeeFontHeight * 2.0), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
#endif
            }
		}
    }

    pAhrs->setClipping( false );

    // TODO: Move these out to their own painters

    QString qsZoom = QString( "%1nm" ).arg( static_cast<int>( m_dZoomNM ) );
    QString qsMagDev = QString( "%1%2" ).arg( m_iMagDev ).arg( QChar( 0xB0 ) );
    double  dInfoH = (smallMetrics.boundingRect( qsZoom ).height() / 2.0) + 4.0;

    if( m_iMagDev < 0 )
        qsMagDev.prepend( "   " );  // There will already be a negative symbol
    else if( m_iMagDev > 0 )
        qsMagDev.prepend( "   +" );
    else
        qsMagDev.prepend( "   " );

    // Draw the zoom level
    pAhrs->setFont( tiny );
    pAhrs->setPen( Qt::black );
    pAhrs->drawText( (m_bPortrait ? c->dW : c->dWa) - c->dW20 - (m_bPortrait ? c->dW20 + c->dW80 : c->dW10) + 2,
                     c->dH - 20.0 - (dInfoH * 2.0) + 2.0 - (m_bPortrait ? (c->dH10 - c->dH40) : c->dH40),
                     qsZoom );
    pAhrs->setPen( QColor( 80, 255, 80 ) );
    pAhrs->drawText( (m_bPortrait ? c->dW : c->dWa) - c->dW20 - (m_bPortrait ? c->dW20 + c->dW80 : c->dW10),
                     c->dH - 20.0 - (dInfoH * 2.0) - (m_bPortrait ? (c->dH10 - c->dH40) : c->dH40),
                     qsZoom );

    // Draw the magnetic deviation
    pAhrs->setPen( Qt::black );
    pAhrs->drawText( (m_bPortrait ? c->dW : c->dWa) - c->dW20 + (m_bPortrait ? c->dW80 : 0.0) - c->dW10 + 2,
                     c->dH - dInfoH + 2.0 - (m_bPortrait ? (c->dH10 - c->dH40) : 0.0),
                     qsMagDev );
    pAhrs->setPen( Qt::yellow );
    pAhrs->drawText( (m_bPortrait ? c->dW : c->dWa) - c->dW20 + (m_bPortrait ? c->dW80 : 0.0) - c->dW10,
                     c->dH - dInfoH - (m_bPortrait ? (c->dH10 - c->dH40) : 0.0),
                     qsMagDev );
}


// Situation (mostly AHRS data) update
void AHRSCanvas::situation( StratuxSituation s )
{
    g_situation = s;
    g_situation.dAHRSGyroHeading += static_cast<double>( m_iMagDev );
    m_bUpdated = true;
    update();
}


// Traffic update
void AHRSCanvas::traffic( int iICAO, StratuxTraffic t )
{
    cullTrafficMap();

    g_trafficMap.insert( iICAO, t );
    m_bUpdated = true;
    update();
}


void AHRSCanvas::cullTrafficMap()
{
    QMapIterator<int, StratuxTraffic> it( g_trafficMap );
    bool                              bTrafficRemoved = true;
    QDateTime                         now = QDateTime::currentDateTime();

    // Each time this is updated, remove an old entry
    while( bTrafficRemoved )
    {
        bTrafficRemoved = false;
        while( it.hasNext() )
        {
            it.next();
            // Anything older than 15 seconds discard
            if( abs( it.value().lastActualReport.secsTo( now ) ) > 15.0 )
            {
                g_trafficMap.remove( it.key() );
                bTrafficRemoved = true;
                break;
            }
        }
    }
}


// Handle various screen presses (pressing the screen is handled the same as a mouse click here)
void AHRSCanvas::mouseReleaseEvent( QMouseEvent *pEvent )
{
    if( pEvent == 0 )
        return;

    QPoint          pt( pEvent->pos() );
    CanvasConstants c = m_pCanvas->constants();

    // Any x delta greater than half the width of the screen and a continuous mouse move of at least 10 events, is a swipe left or right
    if( ((pt.x() - m_SwipeStart.x()) < -c.dW7) && (m_iSwiping > 5) && (abs( pt.y() - m_SwipeStart.y() ) < c.dH4) )
    {
        m_SwipeStart = QPoint( 0, 0 );
        m_iSwiping = 0;
        swipeLeft();
        return;
    }
    else if( ((pt.x() - m_SwipeStart.x()) > c.dW7) && (m_iSwiping > 5) && (abs( pt.y() - m_SwipeStart.y() ) < c.dH4) )
    {
        m_SwipeStart = QPoint( 0, 0 );
        m_iSwiping = 0;
        swipeRight();
        return;
    }
    // Any y delta greater than 1/2 the screen height (or 1/4 in portrait) and a continuous mouse move of at least 10 events, is a swipe up or down
    else if( ((pt.y() - m_SwipeStart.y()) < -c.dH8) && (m_iSwiping > 5) )
    {
        m_SwipeStart = QPoint( 0, 0 );
        m_iSwiping = 0;
        swipeUp();
        return;
    }
    else if( ((pt.y() - m_SwipeStart.y()) > c.dH8) && (m_iSwiping > 5) )
    {
        m_SwipeStart = QPoint( 0, 0 );
        m_iSwiping = 0;
        swipeDown();
        return;
    }

    AHRSMainWin *pMainWin = static_cast<AHRSMainWin *>( parentWidget()->parentWidget() );
    QDateTime    qdtNow = QDateTime::currentDateTime();

    if( pMainWin->menuActive() )
    {
        pMainWin->menu();
        return;
    }

    if( m_bShowGPSDetails )
    {
        m_bShowGPSDetails = false;
        update();
        return;
    }

    if( m_bDisplayTanksSwitchNotice )
    {
        m_bDisplayTanksSwitchNotice = false;
        m_tanks.bOnLeftTank = (!m_tanks.bOnLeftTank);
        m_tanks.lastSwitch = qdtNow;
        update();
        return;
    }

    if( m_bLongPress && (m_longPressStart.msecsTo( qdtNow ) > 500) )
    {
        m_bShowCrosswind = (!m_bShowCrosswind);
        m_bLongPress = false;
        update();
    }
    else
    {
        m_bLongPress = false;
        handleScreenPress( pt );

        m_bUpdated = true;
        update();
    }
}


void AHRSCanvas::handleScreenPress( const QPoint &pressPt )
{
    CanvasConstants c = m_pCanvas->constants();
    QRect           headRect( (m_bPortrait ? c.dW2 : c.dW + c.dW2) - c.dW4, c.dH - 10.0 - c.dW2 - c.dW4, c.dW2, c.dW2 );
    QRect           gpsRect( (m_bPortrait ? c.dW : c.dWa) - c.dW5, c.dH - (c.iLargeFontHeight * 2.0), c.dW5, c.iLargeFontHeight * 2.0 );
    QRect           directRect;
    QRect           fromtoRect;

    if( m_bPortrait )
    {
        directRect.setRect( c.dW40, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20 );
        fromtoRect.setRect( c.dW40 + c.dH20, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20 );
    }
    else
    {
        directRect.setRect( c.dW + c.dW40, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20 );
        fromtoRect.setRect( c.dW + c.dW40 + c.dH20, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20 );
    }

    QRectF          magHeadLessRect;
    QRectF          magHeadMoreRect;
    QRectF          altRect;

    if( m_bPortrait )
        altRect.setRect( c.dW - c.dW5, 0.0, c.dW5, c.dH2 - 40.0 );
    else
        altRect.setRect( c.dW - c.dW5 - c.dW40, 0.0, c.dW5 + c.dW40, c.dH );

    if( m_bPortrait )
    {
        magHeadLessRect.setRect( c.dW2 - (c.dWNum * 3.0 / 2.0) - (c.dW * 0.0125) - c.dW10,
                                 c.dH - c.dW - 10.0 - c.dH20 - c.dHNum - (c.dH * 0.0075),
                                 c.dW20 + c.dW40,
                                 c.dHNum + (c.dH * 0.015) );
        magHeadMoreRect.setRect( c.dW2 + (c.dWNum * 3.0 / 2.0) + (c.dW * 0.0125) + c.dW10 - c.dW20 - c.dW40,
                                 c.dH - c.dW - 10.0 - c.dH20 - c.dHNum - (c.dH * 0.0075),
                                 c.dW20 + c.dW40,
                                 c.dHNum + (c.dH * 0.015) );
    }
    else
    {
        magHeadLessRect.setRect( c.dW + c.dW2 - (c.dWNum * 3.0 / 2.0) - (c.dW * 0.0125) - c.dW10,
                                 10.0 + (c.dH * 0.0075),
                                 c.dW20 + c.dW40,
                                 c.dHNum );
        magHeadMoreRect.setRect( c.dW + c.dW2 + (c.dWNum * 3.0 / 2.0) + (c.dW * 0.0125) + c.dW10 - c.dW20 - c.dW40,
                                 10.0 + (c.dH * 0.0075),
                                 c.dW20 + c.dW40,
                                 c.dHNum );
    }

    if( altRect.contains( pressPt ) )
    {
        Keypad keypad( this, "ALTITUDE BUG" );

        m_pCanvas->setKeypadGeometry( &keypad );
        if( keypad.exec() == QDialog::Accepted )
            m_iAltBug = static_cast<int>( keypad.value() );
        else
            m_iAltBug = -1;
        update();
    }
    // User pressed the GPS Lat/long area. This needs to be before the test for the heading indicator since it's within that area's rectangle
    else if( gpsRect.contains( pressPt ) )
        m_bShowGPSDetails = (!m_bShowGPSDetails);
    else if( magHeadLessRect.contains( pressPt ) )
    {
        m_iMagDev--;
        if( m_iMagDev < -40.0 )
            m_iMagDev++;
        g_pSet->setValue( "MagDev", m_iMagDev );
        g_pSet->sync();
        TrafficMath::updateNearbyAirports( &m_airports, m_dZoomNM );
    }
    else if( magHeadMoreRect.contains( pressPt ) )
    {
        m_iMagDev++;
        if( m_iMagDev > 40.0 )
            m_iMagDev--;
        g_pSet->setValue( "MagDev", m_iMagDev );
        g_pSet->sync();
        TrafficMath::updateNearbyAirports( &m_airports, m_dZoomNM );
    }
    // User pressed on the heading indicator
    else if( headRect.contains( pressPt ) )
    {
        int         iButton = -1;
        BugSelector bugSel( this );

        if( m_bPortrait )
            bugSel.setGeometry( c.dW2 - c.dW4, c.dH2, c.dW2, c.dH2 - c.dH10 );
        else
            bugSel.setGeometry( c.dW + c.dW2 - c.dW4, c.dH2 - c.dH4, c.dW2, c.dH2 );

        iButton = bugSel.exec();

        // Cancelled (do nothing)
        if( iButton == QDialog::Rejected )
            return;
        // Bugs cleared - reset both to invalid and get out
        else if( iButton == static_cast<int>( BugSelector::ClearBugs ) )
        {
            m_iHeadBugAngle = -1;
            m_iWindBugAngle = -1;
            return;
        }

        Keypad keypad( this, "HEADING" );

        m_pCanvas->setKeypadGeometry( &keypad );

        if( iButton == static_cast<int>( BugSelector::WindBug ) )
            keypad.setTitle( "WIND FROM HEADING" );

        if( keypad.exec() == QDialog::Accepted )
        {
            int iAngle = static_cast<int>( keypad.value() );

            // Automatically wrap around
            while( iAngle > 360 )
                iAngle -= 360;
            // Heading bug
            if( iButton == static_cast<int>( BugSelector::HeadingBug ) )
                m_iHeadBugAngle = iAngle;
            // Wind bug
            else if( iButton == static_cast<int>( BugSelector::WindBug ) )
            {
                m_iWindBugAngle = iAngle;
                keypad.clear();
                keypad.setTitle( "WIND SPEED" );
                keypad.exec();
                m_iWindBugSpeed = keypad.value();
            }
        }
        else
        {   // Heading bug
            if( iButton == QDialog::Accepted )
                m_iHeadBugAngle = -1;
            // Wind bug
            else if( iButton == QDialog::Rejected )
                m_iWindBugAngle = -1;
        }
    }
    else if( directRect.contains( pressPt ) )
    {
        AirportDialog dlg( this, &c, "DIRECT TO AIRPORT" );

        // This geometry works for both orientations
        dlg.setGeometry( 0, 0, c.dW, c.dH );
        // If the dialog wasn't cancelled then find the lat/long of the airport selected
        if( dlg.exec() != QDialog::Rejected )
        {
            Airport ap;
            QString qsName = dlg.selectedAirport();

            foreach( ap, g_airportCache )
            {
                if( ap.qsName == qsName )
                {
                    m_directAP = ap;
                    break;
                }
            }
            // Invalidate from-to in favor of direct-to
            m_fromAP.qsID = "NULL";
            m_fromAP.qsName = "NULL";
            m_toAP.qsID = "NULL";
            m_toAP.qsName = "NULL";
        }
        else
        {
            m_directAP.qsID = "NULL";
            m_directAP.qsName = "NULL";
        }
    }
    else if( fromtoRect.contains( pressPt ) )
    {
        AirportDialog dlgFrom( this, &c, "FROM AIRPORT" );

        // This geometry works for both orientations
        dlgFrom.setGeometry( 0, 0, c.dW, c.dH );
        // If the dialog wasn't cancelled then find the lat/long of the airport selected
        if( dlgFrom.exec() != QDialog::Rejected )
        {
            Airport ap;
            QString qsName = dlgFrom.selectedAirport();

            foreach( ap, g_airportCache )
            {
                if( ap.qsName == qsName )
                {
                    m_fromAP = ap;
                    break;
                }
            }

            AirportDialog dlgTo( this, &c, "TO AIRPORT" );

            dlgTo.setGeometry( 0, 0, c.dW, c.dH );
            if( dlgTo.exec() != QDialog::Rejected )
            {
                qsName = dlgTo.selectedAirport();

                foreach( ap, g_airportCache )
                {
                    if( ap.qsName == qsName )
                    {
                        m_toAP = ap;
                        break;
                    }
                }
                // Invalidate direct-to in favor of from-to
                m_directAP.qsID = "NULL";
                m_directAP.qsName = "NULL";
            }
        }
        else
        {
            m_directAP.qsID = "NULL";
            m_directAP.qsName = "NULL";
        }
    }
}


void AHRSCanvas::mousePressEvent( QMouseEvent *pEvent )
{
    CanvasConstants c = m_pCanvas->constants();
    QRect           headRect;

    if( m_bPortrait )
        headRect = QRect( c.dW2 - c.dW4, c.dH2, c.dW2, c.dH2 - c.dH10 );
    else
        headRect = QRect( c.dW + c.dW2 - c.dW4, c.dH2 - c.dH4, c.dW2, c.dH2 );

    m_SwipeStart = pEvent->pos();
    m_iSwiping = 0;

    if( headRect.contains( pEvent->pos() ) )
    {
        m_longPressStart = QDateTime::currentDateTime();
        m_bLongPress = true;
    }
}


void AHRSCanvas::mouseMoveEvent( QMouseEvent *pEvent )
{
    Q_UNUSED( pEvent )

    m_iSwiping++;
}


void AHRSCanvas::zoomIn()
{
    m_dZoomNM -= 5.0;
    if( m_dZoomNM < 5.0 )
        m_dZoomNM = 5.0;
    g_pSet->setValue( "ZoomNM", m_dZoomNM );
    g_pSet->sync();
    QtConcurrent::run( TrafficMath::updateNearbyAirports, &m_airports, m_dZoomNM );
}


void AHRSCanvas::zoomOut()
{
    m_dZoomNM += 5.0;
    if( m_dZoomNM > 100.0 )
        m_dZoomNM = 100.0;
    g_pSet->setValue( "ZoomNM", m_dZoomNM );
    g_pSet->sync();
    QtConcurrent::run( TrafficMath::updateNearbyAirports, &m_airports, m_dZoomNM );
}


void AHRSCanvas::showAllTraffic( bool bAll )
{
    m_settings.bShowAllTraffic = bAll;
    g_pSet->setValue( "ShowAllTraffic", bAll );
    g_pSet->sync();
    update();
}


void AHRSCanvas::showAirports( Canvas::ShowAirports eShow )
{
    m_settings.eShowAirports = eShow;
    g_pSet->setValue( "ShowAirports", static_cast<int>( eShow ) );
    g_pSet->sync();
    update();
}


void AHRSCanvas::showRunways( bool bShow )
{
    m_settings.bShowRunways = bShow;
    g_pSet->setValue( "ShowRunways", bShow );
    g_pSet->sync();
    update();
}


void AHRSCanvas::paintPortrait()
{
    QPainter        ahrs( this );
    CanvasConstants c = m_pCanvas->constants();
    double          dPitchH = c.dH4 + (g_situation.dAHRSpitch / 22.5 * c.dH4);     // The visible portion is only 1/4 of the 90 deg range
    QPixmap         num( 320, 84 );
    QPolygon        shape;
    QPen            linePen( Qt::black );
    double          dSlipSkid = c.dW2 - ((g_situation.dAHRSSlipSkid / 100.0) * c.dW2);
    double          dPxPerVSpeed = c.dH2 / 40.0;
    double          dPxPerFt = static_cast<double>( m_AltTape.height() ) / 20000.0 * 0.99;
    double          dPxPerKnot = static_cast<double>( m_SpeedTape.height() ) / 300.0 * 0.99;

    if( dSlipSkid < (c.dW4 + 25.0) )
        dSlipSkid = c.dW4 + 25.0;
    else if( dSlipSkid > (c.dW2 + c.dW4 - 25.0) )
        dSlipSkid = c.dW2 + c.dW4 - 25.0;

    linePen.setWidth( c.iThinPen );

    // Don't draw past the bottom of the fuel indicators
    ahrs.setClipRect( 0, 0, c.dW, c.dH2 + c.dH5 );

    ahrs.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing, true );

    // Translate to dead center and rotate by stratux roll then translate back
    ahrs.translate( c.dW2, c.dH4 );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -c.dW2, -c.dH4 );

    // Top half sky blue gradient offset by stratux pitch
    QLinearGradient skyGradient( 0.0, -c.dH2, 0.0, dPitchH );
    skyGradient.setColorAt( 0, Qt::blue );
    skyGradient.setColorAt( 1, QColor( 85, 170, 255 ) );
    ahrs.fillRect( -400.0, -c.dH4, c.dW + 800.0, dPitchH + c.dH4, skyGradient );

    // Draw brown gradient horizon half offset by stratux pitch
    // Extreme overdraw accounts for extreme roll angles that might expose the corners
    QLinearGradient groundGradient( 0.0, dPitchH, 0, c.dH2 );
    groundGradient.setColorAt( 0, QColor( 170, 85, 0  ) );
    groundGradient.setColorAt( 1, Qt::black );

    ahrs.fillRect( -400.0, dPitchH, c.dW + 800.0, c.dH4 + c.dH5, groundGradient );
    ahrs.setPen( linePen );
    ahrs.drawLine( -400, dPitchH, c.dW + 800.0, dPitchH );

    for( int i = 0; i < 20; i += 10 )
    {
        linePen.setColor( Qt::cyan );
        ahrs.setPen( linePen );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH - ((i + 2.5) / 45.0 * c.dH4), c.dW2 + c.dW20, dPitchH - ((i + 2.5) / 45.0 * c.dH4) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH - ((i + 5.0) / 45.0 * c.dH4), c.dW2 + c.dW20, dPitchH - ((i + 5.0) / 45.0 * c.dH4) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH - ((i + 7.5) / 45.0 * c.dH4), c.dW2 + c.dW20, dPitchH - ((i + 7.5) / 45.0 * c.dH4) );
        ahrs.drawLine( c.dW2 - c.dW5, dPitchH - ((i + 10.0) / 45.0 * c.dH4), c.dW2 + c.dW5, dPitchH - (( i + 10.0) / 45.0 * c.dH4) );
        linePen.setColor( QColor( 67, 33, 9 ) );
        ahrs.setPen( linePen );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH + ((i + 2.5) / 45.0 * c.dH4), c.dW2 + c.dW20, dPitchH + ((i + 2.5) / 45.0 * c.dH4) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH + ((i + 5.0) / 45.0 * c.dH4), c.dW2 + c.dW20, dPitchH + ((i + 5.0) / 45.0 * c.dH4) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH + ((i + 7.5) / 45.0 * c.dH4), c.dW2 + c.dW20, dPitchH + ((i + 7.5) / 45.0 * c.dH4) );
        ahrs.drawLine( c.dW2 - c.dW5, dPitchH + ((i + 10.0) / 45.0 * c.dH4), c.dW2 + c.dW5, dPitchH + (( i + 10.0) / 45.0 * c.dH4) );
    }

    // Reset rotation and clipping
    ahrs.resetTransform();
    ahrs.setClipping( false );

    // Slip/Skid indicator
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW2 - c.dW4, 1, c.dW2, c.dH40 );
    ahrs.drawRect( c.dW2 - m_pCanvas->scaledH( 15.0 ), 1.0, m_pCanvas->scaledH( 30.0 ), c.dH40 );
    ahrs.setPen( Qt::NoPen );
    ahrs.setBrush( Qt::white );
    ahrs.drawEllipse( dSlipSkid - m_pCanvas->scaledH( 10.0 ),
                      1.0,
                      m_pCanvas->scaledV( 20.0 ),
                      c.dH40 );

    // Draw the top roll indicator
    ahrs.translate( c.dW2, c.dH20 + ((c.dW - c.dW5) / 2.0) );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -c.dW2, -(c.dH20 + ((c.dW - c.dW5) / 2.0)) );
    ahrs.drawPixmap( c.dW10, c.dH20, c.dW - c.dW5, c.dW - c.dW5, m_RollIndicator );
    ahrs.resetTransform();

    QPolygonF arrow;

    arrow.append( QPointF( c.dW2, c.dH40 + (c.dH * (m_bPortrait ? 0.0625 : 0.104167)) ) );
    arrow.append( QPointF( c.dW2 + (c.dWa * (m_bPortrait ? 0.03125 : 0.01875)), c.dH40 + (c.dH * (m_bPortrait ? 0.08125 : 0.1354167)) ) );
    arrow.append( QPointF( c.dW2 - (c.dWa * (m_bPortrait ? 0.03125 : 0.01875)), c.dH40 + (c.dH * (m_bPortrait ? 0.08125 : 0.1354167)) ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the yellow pitch indicators
    ahrs.setBrush( Qt::yellow );
    shape.append( QPoint( c.dW5 + c.dW20, c.dH4 - c.dH160 ) );
    shape.append( QPoint( c.dW2 - c.dW10, c.dH4 - c.dH160 ) );
    shape.append( QPoint( c.dW2 - c.dW10 + 20, c.dH4 ) );
    shape.append( QPoint( c.dW2 - c.dW10, c.dH4 + c.dH160 ) );
    shape.append( QPoint( c.dW5 + c.dW20, c.dH4 + c.dH160 ) );
    ahrs.drawPolygon( shape );
    shape.clear();
    shape.append( QPoint( c.dW - c.dW5 - c.dW20, c.dH4 - c.dH160 ) );
    shape.append( QPoint( c.dW2 + c.dW10, c.dH4 - c.dH160 ) );
    shape.append( QPoint( c.dW2 + c.dW10 - 20, c.dH4 ) );
    shape.append( QPoint( c.dW2 + c.dW10, c.dH4 + c.dH160 ) );
    shape.append( QPoint( c.dW - c.dW5 - c.dW20, c.dH4 + c.dH160 ) );
    ahrs.drawPolygon( shape );
    shape.clear();
    shape.append( QPoint( c.dW2, c.dH4 ) );
    shape.append( QPoint( c.dW2 - c.dW20, c.dH4 + 20 ) );
    shape.append( QPoint( c.dW2 + c.dW20, c.dH4 + 20 ) );
    ahrs.drawPolygon( shape );

    // Left Tank indicators background
    ahrs.drawPixmap( 0.0, c.dH2 + c.dH40, c.dW20, c.dH2 - c.dH5, m_Lfuel );
    // Tank indicators level
    QPen levelPen( Qt::black, c.dH40 + 4 );
    levelPen.setCapStyle( Qt::RoundCap );
    ahrs.setPen( levelPen );
    ahrs.drawLine( 0.0,
                   c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)),
                   c.dW40,
                   c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)) );
    levelPen.setWidth( c.dH40 );
    levelPen.setColor( QColor( 255, 150, 255 ) );
    ahrs.setPen( levelPen );
    ahrs.drawLine( 0.0,
                   c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)),
                   c.dW40,
                   c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)) );

    if( m_tanks.bDualTanks )
    {
        // Right Tank indicators background
        ahrs.drawPixmap( c.dW - c.dW20 - 1, c.dH2 + c.dH40, c.dW20, c.dH2 - c.dH5, m_Rfuel );
        // Right Tank indicators level
        levelPen.setColor( Qt::black );
        levelPen.setWidth( c.dH40 + 4 );
        ahrs.setPen( levelPen );
        ahrs.drawLine( c.dW,
                       c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)),
                       c.dW - c.dW40 - 1,
                       c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)) );
        levelPen.setWidth( c.dH40 );
        levelPen.setColor( QColor( 255, 150, 255 ) );
        ahrs.setPen( levelPen );
        ahrs.drawLine( c.dW,
                       c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)),
                       c.dW - c.dW40 - 1,
                       c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)) );
    }

    // Tank indicator active indicators
    ahrs.setFont( large );
    if( m_bFuelFlowStarted )
    {
        QPen fuelPen( Qt::yellow, c.dH80 );

        ahrs.setPen( fuelPen );

        if( m_tanks.bOnLeftTank || (!m_tanks.bDualTanks) )
            ahrs.drawLine( 0, c.dH2 + c.dH40 - 15, c.dW10 - 2, c.dH2 + c.dH40 - 15 );
        else
            ahrs.drawLine( c.dW - c.dW10 + 2, c.dH2 + c.dH40 - 15, c.dW, c.dH2 + c.dH40 - 15 );
    }

    // Arrow for heading position above heading dial
    arrow.clear();
    arrow.append( QPointF( c.dW2, c.dH - c.dW - 10.0 - c.dH80 ) );
    arrow.append( QPointF( c.dW2 + c.dW40, c.dH - c.dW - 10.0 - c.dH40 ) );
    arrow.append( QPointF( c.dW2 - c.dW40, c.dH - c.dW - 10.0 - c.dH40 ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the heading value over the indicator
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW2 - (c.dWNum * 3.0 / 2.0) - (c.dW * 0.0125), arrow.boundingRect().y() - c.dHNum - c.dH40 - (c.dH * 0.0075), (c.dWNum * 3.0) + (c.dW * 0.025), c.dHNum + (c.dH * 0.015) );
    Builder::buildNumber( &num, &c, static_cast<int>( g_situation.dAHRSGyroHeading ), 3 );
    ahrs.drawPixmap( c.dW2 - (c.dWNum * 3.0 / 2.0), arrow.boundingRect().y() - c.dHNum - c.dH40, num );

    // Draw the magnetic deviation less/more controls
    ahrs.drawPixmap( c.dW2 - (c.dWNum * 3.0 / 2.0) - (c.dW * 0.0125) - c.dW10,
                     arrow.boundingRect().y() - c.dHNum - c.dH40 - (c.dH * 0.0075),
                     c.dW20 + c.dW40,
                     c.dHNum + (c.dH * 0.015),
                     *m_pMagHeadOffLessPixmap );
    ahrs.drawPixmap( c.dW2 + (c.dWNum * 3.0 / 2.0) + (c.dW * 0.0125) + c.dW10 - c.dW20 - c.dW40,
                     arrow.boundingRect().y() - c.dHNum - c.dH40 - (c.dH * 0.0075),
                     c.dW20 + c.dW40,
                     c.dHNum + (c.dH * 0.015),
                    *m_pMagHeadOffMorePixmap );

    // Draw the heading pixmap and rotate it to the current heading
    ahrs.translate( c.dW2, c.dH - c.dW2 - 20.0 );
    ahrs.rotate( -g_situation.dAHRSGyroHeading );
    ahrs.translate( -c.dW2, -(c.dH - c.dW2 - 20.0) );
    ahrs.drawPixmap( 10, c.dH - c.dW - 10.0, c.dW - 20, c.dW - 20,  m_HeadIndicator );
    ahrs.resetTransform();

    drawDirectOrFromTo( &ahrs, &c );

    // Draw the central airplane
    ahrs.drawPixmap( c.dW2 - c.dW20, c.dH - 20.0 - c.dW2 - c.dW20, c.dW10, c.dW10, m_planeIcon );

    // Draw the heading bug
    if( m_iHeadBugAngle >= 0 )
    {
        ahrs.translate( c.dW2, c.dH - 20.0 - c.dW2 );
        ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -c.dW2, -(c.dH - 20.0 - c.dW2) );
        ahrs.drawPixmap( c.dW2 - (m_headIcon.width() / 2), c.dH - c.dH10 - c.dW + (m_headIcon.height() / 2), m_headIcon );

        // If long press triggered crosswind component display and the wind bug is set
        if( m_bShowCrosswind && (m_iWindBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( QColor( 0xFF, 0x90, 0x01 ) );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW2, c.dH - c.dW - 10.0, c.dW2, c.dH - 10.0 - c.dW2 );
        }

        ahrs.resetTransform();
    }

    // Draw the wind bug
    if( m_iWindBugAngle >= 0 )
    {
        ahrs.translate( c.dW2, c.dH - c.dW2 - 20.0 );
        ahrs.rotate( m_iWindBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -c.dW2, -(c.dH - c.dW2 - 20.0) );
        ahrs.drawPixmap( c.dW2 - (m_headIcon.width() / 2), c.dH - c.dH10 - c.dW + (m_headIcon.height() / 2), m_windIcon );

        QString      qsWind = QString::number( m_iWindBugSpeed );
        QFontMetrics windMetrics( med );
        QRect        windRect = windMetrics.boundingRect( qsWind );

#if defined( Q_OS_ANDROID )
        windRect.setWidth( windRect.width() * 4 );
#endif

        ahrs.setFont( med );
        ahrs.setPen( Qt::black );
        ahrs.drawText( c.dW2 - (windRect.width() / 2), c.dH - c.dW - c.dH160, qsWind );
        ahrs.setPen( Qt::white );
        ahrs.drawText( c.dW2 - (windRect.width() / 2) - 1, c.dH - c.dW - c.dH160 - 1, qsWind );

        // If long press triggered crosswind component display and the heading bug is set
        if( m_bShowCrosswind && (m_iHeadBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( Qt::cyan );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW2, c.dH - c.dW - 10, c.dW2, c.dH - 10.0 - c.dW2 );

            // Draw the crosswind component calculated from heading vs wind
            double dAng = fabs( static_cast<double>( m_iWindBugAngle ) - static_cast<double>( m_iHeadBugAngle ) );
            while( dAng > 180.0 )
                dAng -= 360.0;
            dAng = fabs( dAng );
            double dCrossComp = fabs( static_cast<double>( m_iWindBugSpeed ) * sin( dAng * ToRad ) );
            double dCrossPos = c.dH - (c.dW / 1.3) - 10.0;
            QString qsCrossAng = QString( "%1%2" ).arg( static_cast<int>( dAng ) ).arg( QChar( 176 ) );

            ahrs.resetTransform();
            ahrs.translate( c.dW2, c.dH - c.dW2 - 10.0 );
            ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
            ahrs.translate( -c.dW2, -(c.dH - c.dW2 - 10.0) );
            ahrs.setFont( large );
            ahrs.setPen( Qt::black );
            ahrs.drawText( c.dW2 + 5.0, dCrossPos, QString::number( static_cast<int>( dCrossComp ) ) );
            ahrs.setPen( QColor( 0xFF, 0x90, 0x01 ) );
            ahrs.drawText( c.dW2 + 3.0, dCrossPos - 2.0, QString::number( static_cast<int>( dCrossComp ) ) );
            ahrs.setFont( med );
            ahrs.setPen( Qt::black );
            ahrs.drawText( c.dW2 + 5.0, dCrossPos + c.iMedFontHeight - 5, qsCrossAng );
            ahrs.setPen( Qt::cyan );
            ahrs.drawText( c.dW2 + 3.0, dCrossPos + c.iMedFontHeight - 7, qsCrossAng );
        }

        ahrs.resetTransform();
    }

    // Draw the Altitude tape
    ahrs.fillRect( c.dW - c.dW5, 0, c.dW5, c.dH2 - 40.0, QColor( 0, 0, 0, 100 ) );
    ahrs.setClipRect( c.dW - c.dW5 + 1.0, 2.0, c.dW5 - 4.0, c.dH2 - 50 );
    ahrs.drawPixmap( c.dW - c.dW5 + 5, c.dH4 + 10.0 - m_AltTape.height() + (g_situation.dBaroPressAlt * dPxPerFt), m_AltTape );
    ahrs.setClipping( false );

    // Draw the altitude bug
    if( m_iAltBug >= 0 )
    {
        double dAltTip = c.dH4 - 10.0 - ((static_cast<double>( m_iAltBug ) - g_situation.dBaroPressAlt) * dPxPerFt);
#if !defined( Q_OS_ANDROID )
        dAltTip += c.dH80;
#endif
        ahrs.drawPixmap( c.dW - c.dW5 - c.dW20, dAltTip - c.dH40, c.dW20, c.dH20, m_AltBug );
    }

    // Draw the vertical speed static pixmap
    ahrs.fillRect( c.dW - c.dW20 - c.dW40, 0.0, c.dW40, c.dH2, QColor( 0, 0, 0, 100 ) );
    ahrs.drawPixmap( c.dW - c.dW20, 0, c.dW20, c.dH2, m_VertSpeedTape );

    // Draw the vertical speed indicator
    ahrs.translate( 0.0, c.dH4 - (dPxPerVSpeed * g_situation.dGPSVertSpeed / 100.0 * 0.98) );   // 98% accounts for the slight margin on each end
    arrow.clear();
    arrow.append( QPoint( c.dW - m_pCanvas->scaledH( 30.0 ), 0.0 ) );
    arrow.append( QPoint( c.dW - m_pCanvas->scaledH( 20.0 ), m_pCanvas->scaledV( -7.0 ) ) );
    arrow.append( QPoint( c.dW, m_pCanvas->scaledV( -15.0 ) ) );
    arrow.append( QPoint( c.dW, m_pCanvas->scaledV( 15.0 ) ) );
    arrow.append( QPoint( c.dW - m_pCanvas->scaledH( 20.0 ), m_pCanvas->scaledV( 7.0 ) ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.drawPolygon( arrow );

    QString qsFullVspeed = QString::number( g_situation.dGPSVertSpeed / 100.0, 'f', 1 );
    QString qsFracVspeed = qsFullVspeed.right( 1 );
    QString qsIntVspeed = qsFullVspeed.left( qsFullVspeed.length() - 2 );
    QFontMetrics weeMetrics( wee );

    // Draw vertical speed indicator as In thousands and hundreds of FPM in tiny text on the vertical speed arrow
    ahrs.setFont( wee );
    QRect intRect( weeMetrics.boundingRect( qsIntVspeed ) );
#if defined( Q_OS_ANDROID )
    intRect.setWidth( intRect.width() * 4 );
#endif
    ahrs.drawText( c.dW - m_pCanvas->scaledH( 20.0 ), m_pCanvas->scaledV( 4.0 ), qsIntVspeed );
    ahrs.setFont( itsy );
    ahrs.drawText( c.dW - m_pCanvas->scaledH( 20.0 ) + intRect.width() + 2, m_pCanvas->scaledV( 4.0 ), qsFracVspeed );
    ahrs.resetTransform();

    // Draw the current altitude
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( QColor( 0, 0, 0, 175 ) );
    ahrs.drawRect( c.dW - c.dW5 - (c.dW * 0.0125), c.dH4 - (c.dHNum / 2.0) - (c.dH * 0.0075), c.dW5 + (c.dW * 0.025), c.dHNum + (c.dH * 0.015) );
    ahrs.setPen( Qt::white );
    Builder::buildNumber( &num, &c, static_cast<int>( g_situation.dBaroPressAlt ), 0 );
    ahrs.drawPixmap( c.dW - c.dW5, c.dH4 - (c.dHNum / 2.0), num );

    // Draw the Speed tape
    ahrs.fillRect( 0, 0, c.dW10 + 5.0, c.dH2, QColor( 0, 0, 0, 100 ) );
    ahrs.setClipRect( 2.0, 2.0, c.dW5 - 4.0, c.dH2 - 4.0 );
    ahrs.drawPixmap( 5, c.dH4 + 5.0 - m_SpeedTape.height() + (g_situation.dGPSGroundSpeed * dPxPerKnot), m_SpeedTape );
    ahrs.setClipping( false );

    // Draw the current speed
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( QColor( 0, 0, 0, 175 ) );
    ahrs.drawRect( 0, c.dH4 - (c.dHNum / 2.0) - (c.dH * 0.0075), c.dW5, c.dHNum + (c.dH * 0.015) );
    Builder::buildNumber( &num, &c, static_cast<int>( g_situation.dGPSGroundSpeed ), 0 );
    ahrs.drawPixmap( c.dW * 0.0125, c.dH4 - (c.dHNum / 2.0), num );

    ahrs.setFont( wee );
    ahrs.setPen( Qt::black );
    ahrs.drawText( c.dW10 + c.dW40 + (c.dW80 / 2.0) + 1, c.dH4 + 1, g_bUnitsKnots ? "KTS" : "MPH"  );
    ahrs.setPen( Qt::white );
    ahrs.drawText( c.dW10 + c.dW40 + (c.dW80 / 2.0), c.dH4, g_bUnitsKnots ? "KTS" : "MPH"  );

    // Draw the G-Force indicator box and scale
    ahrs.setFont( tiny );
    ahrs.setPen( Qt::black );
    ahrs.drawText( c.dW - c.dW5, c.dH - 15, "2" );
    ahrs.drawText( c.dW - c.dW5 + c.dW10 - (c.iTinyFontWidth / 2), c.dH - 15, "0" );
    ahrs.drawText( c.dW - c.iTinyFontWidth - 10, c.dH - 15, "2" );
    ahrs.setPen( Qt::white );
    ahrs.drawText( c.dW - c.dW5 + 1, c.dH - 16, "2" );
    ahrs.drawText( c.dW - c.dW5 + c.dW10 - (c.iTinyFontWidth / 2), c.dH - 16, "0" );
    ahrs.drawText( c.dW - c.iTinyFontWidth - 11, c.dH - 16, "2" );

    // Arrow for G-Force indicator
    arrow.clear();
    arrow.append( QPoint( 1, c.dH - c.iTinyFontHeight - m_pCanvas->scaledV( 10.0 ) ) );
    arrow.append( QPoint( m_pCanvas->scaledH( -14.0 ), c.dH - c.iTinyFontHeight - m_pCanvas->scaledV( 25.0 ) ) );
    arrow.append( QPoint( m_pCanvas->scaledH( 16.0 ), c.dH - c.iTinyFontHeight - m_pCanvas->scaledV( 25.0 ) ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.translate( c.dW - c.dW5 + c.dW10 - m_pCanvas->scaledH( 10.0 ) + ((g_situation.dAHRSGLoad - 1.0) * (c.dW10 - m_pCanvas->scaledH( 10.0 ))), c.dH80 );
    ahrs.drawPolygon( arrow );
    ahrs.resetTransform();

    // Update the airport positions
    if( m_settings.eShowAirports != Canvas::ShowNoAirports )
        updateAirports( &ahrs, &c );

    // Update the traffic positions
    updateTraffic( &ahrs, &c );

    if( m_bShowGPSDetails )
        paintInfo( &ahrs, &c );
    else if( m_bDisplayTanksSwitchNotice )
        paintSwitchNotice( &ahrs, &c );

    if( (m_iTimerMin >= 0) && (m_iTimerSec >= 0) )
        paintTimer( &ahrs, &c );

    ahrs.drawPixmap( c.dW40, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20, m_DirectTo );
    ahrs.drawPixmap( c.dW40 + c.dH20, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20, m_FromTo );

    drawDayMode( &ahrs, &c );
}


void AHRSCanvas::paintSwitchNotice( QPainter *pAhrs, CanvasConstants *c )
{
    QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, c->dH - 50.0 );
    QPen            linePen( Qt::black );

    cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
    cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

    linePen.setWidth( c->iThickPen );
    pAhrs->setPen( linePen );
    pAhrs->setBrush( cloudyGradient );

    pAhrs->drawRect( c->dW10, c->dH10, (m_bPortrait ? c->dW : c->dWa) - c->dW5, c->dH - c->dH5 );
    pAhrs->setFont( large );
    pAhrs->drawText( c->dW5, c->dH5 + c->iLargeFontHeight, "TANK SWITCH" );
    pAhrs->setFont( med );
    pAhrs->drawText( c->dW5, c->dH5 + (c->iMedFontHeight * 3),  QString( "Switch to  %1  tank" ).arg( m_tanks.bOnLeftTank ? "RIGHT" : "LEFT" ) );
    pAhrs->drawText( c->dW5, c->dH5 + (c->iMedFontHeight * 5),  QString( "ADJUST MIXTURE" ) );
    pAhrs->drawText( c->dW5, c->dH5 + (c->iMedFontHeight * 7),  QString( "PRESS TO CONFIRM" ) );
}


void AHRSCanvas::paintTimer( QPainter *pAhrs, CanvasConstants *c )
{
    QString      qsTimer = QString( "%1:%2" ).arg( m_iTimerMin, 2, 10, QChar( '0' ) ).arg( m_iTimerSec, 2, 10, QChar( '0' ) );
    QFontMetrics largeMetrics( large );
    QPen         linePen( Qt::white );
    QPixmap      Num( c->dW5, c->dH20 );
    QPixmap      timerNum( c->dW5, c->dH20 );

    linePen.setWidth( c->iThinPen );

    pAhrs->setPen( linePen );
    pAhrs->setBrush( Qt::black );

    Builder::buildNumber( &Num, c, qsTimer );
    timerNum.fill( Qt::cyan );
    timerNum.setMask( Num.createMaskFromColor( Qt::transparent ) );

    if( m_bPortrait )
    {
        pAhrs->drawRect( c->dW2 - c->dW10 - c->dW40, c->dH - c->dH20, c->dW5 + c->dW20, c->dH20 );
        pAhrs->drawPixmap( c->dW2 - c->dW10, c->dH - c->dH20 + c->dH100, timerNum );
    }
    else
    {
        pAhrs->drawRect( c->dW2 - c->dW5, c->dH - c->dH5, c->dW5 + c->dW10, c->dH20 );
        pAhrs->drawPixmap( c->dW2 - c->dW10 + c->dW20 - (timerNum.width() / 2), c->dH - c->dH5 + c->dH100, timerNum );
    }
}


void AHRSCanvas::paintInfo( QPainter *pAhrs, CanvasConstants *c )
{
    QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, c->dH - 50.0 );
    QFont           med_bu( med );
    QPen            linePen( Qt::black );
    int             iMedFontHeight = c->iMedFontHeight;
    int             iSmallFontHeight = c->iSmallFontHeight;
    int             iVoff = 0;

#if defined( Q_OS_ANDROID )
    iVoff = 50;
#endif

    med_bu.setUnderline( true );
    med_bu.setBold( true );

    cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
    cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

    linePen.setWidth( c->iThickPen );
    pAhrs->setPen( linePen );
    pAhrs->setBrush( cloudyGradient );
    pAhrs->drawRect( 50, 50, (m_bPortrait ? c->dW : c->dWa) - 100, c->dH - 100 );
    pAhrs->setFont( med_bu );
    pAhrs->drawText( 75, 95 + iVoff, "GPS Status" );
    pAhrs->setFont( small );
    pAhrs->drawText( 75, 95 + iVoff + iMedFontHeight,  QString( "GPS Satellites Seen: %1" ).arg( g_situation.iGPSSatsSeen ) );
    pAhrs->drawText( 75, 95 + iVoff + (iMedFontHeight * 2),  QString( "GPS Satellites Tracked: %1" ).arg( g_situation.iGPSSatsTracked ) );
    pAhrs->drawText( 75, 95 + iVoff + (iMedFontHeight * 3),  QString( "GPS Satellites Locked: %1" ).arg( g_situation.iGPSSats ) );
    pAhrs->drawText( 75, 95 + iVoff + (iMedFontHeight * 4),  QString( "GPS Fix Quality: %1" ).arg( g_situation.iGPSFixQuality ) );

    QList<StratuxTraffic> trafficList = g_trafficMap.values();
    StratuxTraffic        traffic;
    int                   iY = 0;
    int                   iLine;

    // Draw a large dot for each aircraft; the outer edge of the heading indicator is calibrated to be 20 NM out from your position
    pAhrs->setFont( med_bu );
    pAhrs->drawText( m_bPortrait ? 75 : c->dW, m_bPortrait ? c->dH2 : 95 + iVoff, "Non-ADS-B Traffic" );
    pAhrs->setFont( small );
    foreach( traffic, trafficList )
    {
        // If bearing and distance were able to be calculated then show relative position
        if( !traffic.bHasADSB && (!traffic.qsTail.isEmpty()) )
        {
            iLine = (m_bPortrait ? static_cast<int>( c->dH2 ) : 95 + iVoff) + iMedFontHeight + (iY * iSmallFontHeight);
            pAhrs->drawText( m_bPortrait ? 75 : c->dW, iLine, traffic.qsTail );
            pAhrs->drawText( m_bPortrait ? m_pCanvas->scaledH( 200.0 ) : m_pCanvas->scaledH( 500.0 ), iLine, QString( "%1 ft" ).arg( static_cast<int>( traffic.dAlt ) ) );
            if( traffic.iSquawk > 0 )
                pAhrs->drawText( m_bPortrait ? m_pCanvas->scaledH( 325 ) : m_pCanvas->scaledH( 600 ), iLine, QString::number( traffic.iSquawk ) );
            iY++;
        }
        if( iY > 10 )
            break;
    }

    pAhrs->setFont( med );
    pAhrs->setPen( Qt::blue );
    pAhrs->drawText( 75, m_bPortrait ? m_pCanvas->scaledV( 700.0 ) : m_pCanvas->scaledV( 390.0 ), QString( "Version: %1" ).arg( g_qsStratofierVersion ) );
}

void AHRSCanvas::paintLandscape()
{
    QPainter        ahrs( this );
    CanvasConstants c = m_pCanvas->constants();
    double          dPitchH = c.dH2 + (g_situation.dAHRSpitch / 22.5 * c.dH2);     // The visible portion is only 1/4 of the 90 deg range
    QString         qsHead( QString( "%1" ).arg( static_cast<int>( g_situation.dAHRSGyroHeading ), 3, 10, QChar( '0' ) ) );
    QPolygon        shape;
    QPen            linePen( Qt::black );
    double          dSlipSkid = c.dW2 - ((g_situation.dAHRSSlipSkid / 100.0) * c.dW2);
    double          dPxPerVSpeed = c.dH / 40.0;
    double          dPxPerKnot = static_cast<double>( m_SpeedTape.height() ) / 300.0 * 0.99;
    double          dPxPerFt = static_cast<double>( m_AltTape.height() ) / 20000.0 * 0.99;  // 0.99 accounts for the few pixels above and below the numbers in the pixmap that offset the position at the extremes of the scale
    QFontMetrics    tinyMetrics( tiny );
    QPixmap         num( 320, 84 );

    if( dSlipSkid < (c.dW4 + 25.0) )
        dSlipSkid = c.dW4 + 25.0;
    else if( dSlipSkid > (c.dW2 + c.dW4 - 25.0) )
        dSlipSkid = c.dW2 + c.dW4 - 25.0;

    linePen.setWidth( c.iThinPen );

    ahrs.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing, true );

    // Clip the attitude to the left half of the display
    ahrs.setClipRect( 0, 0, c.dW, c.dH );

    // Translate to dead center and rotate by stratux roll then translate back
    ahrs.translate( c.dW2 - c.dW20, c.dH2 );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -(c.dW2 - c.dW20), -c.dH2 );

    ahrs.translate( -c.dW20, 0.0 );

    // Top half sky blue gradient offset by stratux pitch
    QLinearGradient skyGradient( 0.0, -c.dH4, 0.0, dPitchH );
    skyGradient.setColorAt( 0, Qt::blue );
    skyGradient.setColorAt( 1, QColor( 85, 170, 255 ) );
    ahrs.fillRect( -400.0, -c.dH4, c.dW + 800.0, dPitchH + c.dH4, skyGradient );

    // Draw brown gradient horizon half offset by stratux pitch
    // Extreme overdraw accounts for extreme roll angles that might expose the corners
    QLinearGradient groundGradient( 0.0, c.dH2, 0, c.dH + c.dH4 );
    groundGradient.setColorAt( 0, QColor( 170, 85, 0 ) );
    groundGradient.setColorAt( 1, Qt::black );

    ahrs.fillRect( -400.0, dPitchH, c.dW + 800.0, c.dH, groundGradient );
    ahrs.setPen( linePen );
    ahrs.drawLine( -400, dPitchH, c.dW + 800.0, dPitchH );

    for( int i = 0; i < 20; i += 10 )
    {
        linePen.setColor( Qt::cyan );
        ahrs.setPen( linePen );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH - ((i + 2.5) / 45.0 * c.dH2), c.dW2 + c.dW20, dPitchH - ((i + 2.5) / 45.0 * c.dH2) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH - ((i + 5.0) / 45.0 * c.dH2), c.dW2 + c.dW20, dPitchH - ((i + 5.0) / 45.0 * c.dH2) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH - ((i + 7.5) / 45.0 * c.dH2), c.dW2 + c.dW20, dPitchH - ((i + 7.5) / 45.0 * c.dH2) );
        ahrs.drawLine( c.dW2 - c.dW5, dPitchH - ((i + 10.0) / 45.0 * c.dH2), c.dW2 + c.dW5, dPitchH - (( i + 10.0) / 45.0 * c.dH2) );
        linePen.setColor( QColor( 67, 33, 9 ) );
        ahrs.setPen( linePen );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH + ((i + 2.5) / 45.0 * c.dH2), c.dW2 + c.dW20, dPitchH + ((i + 2.5) / 45.0 * c.dH2) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH + ((i + 5.0) / 45.0 * c.dH2), c.dW2 + c.dW20, dPitchH + ((i + 5.0) / 45.0 * c.dH2) );
        ahrs.drawLine( c.dW2 - c.dW20, dPitchH + ((i + 7.5) / 45.0 * c.dH2), c.dW2 + c.dW20, dPitchH + ((i + 7.5) / 45.0 * c.dH2) );
        ahrs.drawLine( c.dW2 - c.dW5, dPitchH + ((i + 10.0) / 45.0 * c.dH2), c.dW2 + c.dW5, dPitchH + (( i + 10.0) / 45.0 * c.dH2) );
    }

    ahrs.translate( c.dW20, 0.0 );

    // Reset rotation
    ahrs.resetTransform();

    // Remove the clipping rect
    ahrs.setClipping( false );

    ahrs.translate( -c.dW20, 0.0 );

    // Slip/Skid indicator
    ahrs.setPen( QPen( Qt::white, 2 ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW2 - c.dW4, 1, c.dW2, c.dH40 );
    ahrs.drawRect( c.dW2 - 15.0, 1.0, 30.0, c.dH40 );
    ahrs.setPen( Qt::NoPen );
    ahrs.setBrush( Qt::white );
    ahrs.drawEllipse( dSlipSkid - 10.0,
                      1.0,
                      20.0,
                      c.dH40 );

    ahrs.translate( c.dW20, 0.0 );

    // Draw the top roll indicator
    ahrs.translate( c.dW2, c.dH20 + ((c.dW - c.dW5) / 2.0) );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -c.dW2, -(c.dH20 + ((c.dW - c.dW5) / 2.0)) );
    ahrs.drawPixmap( c.dW10, c.dH20, c.dW - c.dW5 - c.dW10, c.dW - c.dW5 - c.dW10, m_RollIndicator );
    ahrs.resetTransform();

    ahrs.translate( -c.dW20, 0.0 );

    QPolygonF arrow;

    arrow.append( QPointF( c.dW2, c.dH10 ) );
    arrow.append( QPointF( c.dW2 + c.dW40, c.dH10 + c.dH40 ) );
    arrow.append( QPointF( c.dW2 - c.dW40, c.dH10 + c.dH40 ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the yellow pitch indicators
    ahrs.setBrush( Qt::yellow );
    shape.append( QPoint( c.dW5 + c.dW10, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 - c.dW10, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 - c.dW10 + 20, c.dH2 ) );
    shape.append( QPoint( c.dW2 - c.dW10, c.dH2 + c.dH160 + 4.0 ) );
    shape.append( QPoint( c.dW5 + c.dW10, c.dH2 + c.dH160 + 4.0 ) );
    ahrs.drawPolygon( shape );
    shape.clear();
    shape.append( QPoint( c.dW - c.dW5 - c.dW10, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 + c.dW10, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 + c.dW10 - 20, c.dH2 ) );
    shape.append( QPoint( c.dW2 + c.dW10, c.dH2 + c.dH160 + 4.0 ) );
    shape.append( QPoint( c.dW - c.dW5 - c.dW10, c.dH2 + c.dH160 + 4.0 ) );
    ahrs.drawPolygon( shape );
    shape.clear();
    shape.append( QPoint( c.dW2, c.dH2 ) );
    shape.append( QPoint( c.dW2 - c.dW20, c.dH2 + 20 ) );
    shape.append( QPoint( c.dW2 + c.dW20, c.dH2 + 20 ) );
    ahrs.drawPolygon( shape );

    ahrs.translate( c.dW20, 0.0 );

    // Arrow for heading position above heading dial
    arrow.clear();
    arrow.append( QPointF( c.dW + c.dW2, c.dH - c.dW - c.dH80 ) );
    arrow.append( QPointF( c.dW + c.dW2 + c.dW40, c.dH - c.dW - c.dH40 ) );
    arrow.append( QPointF( c.dW + c.dW2 - c.dW40, c.dH - c.dW - c.dH40 ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the heading pixmap and rotate it to the current heading
    ahrs.translate( c.dW + c.dW2, c.dH - c.dW2 - 10.0 );
    ahrs.rotate( -g_situation.dAHRSGyroHeading );
    ahrs.translate( -(c.dW + c.dW2), -(c.dH - c.dW2 - 10.0) );
    ahrs.drawPixmap( c.dW + 10, c.dH - c.dW, c.dW - 20, c.dW - 20,  m_HeadIndicator );
    ahrs.resetTransform();

    drawDirectOrFromTo( &ahrs, &c );

    // Draw the central airplane
    ahrs.drawPixmap( c.dW + c.dW2 - c.dW20, c.dH - c.dW2 - c.dW20 - 20.0, c.dW10, c.dW10, m_planeIcon );

    // Draw the heading bug
    if( m_iHeadBugAngle >= 0 )
    {
        ahrs.translate( c.dW + c.dW2, c.dH - c.dW2 - 20.0 );
        ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -(c.dW + c.dW2), -(c.dH - c.dW2 - 20.0) );
        ahrs.drawPixmap( c.dW + c.dW2 - (m_headIcon.width() / 2), c.dH - c.dH10 - c.dW, m_headIcon );

        // If long press triggered crosswind component display and the wind bug is set
        if( m_bShowCrosswind && (m_iWindBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( QColor( 0xFF, 0x90, 0x01 ) );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW + c.dW2, c.dH - c.dW - 10.0, c.dW + c.dW2, c.dH - 10.0 - c.dW2 );
        }

        ahrs.resetTransform();
    }

    // Draw the wind bug
    if( m_iWindBugAngle >= 0 )
    {
        ahrs.translate( c.dW + c.dW2, c.dH - c.dW2 - 20.0 );
        ahrs.rotate( m_iWindBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -(c.dW + c.dW2), -(c.dH - c.dW2 - 20.0) );
        ahrs.drawPixmap( c.dW + c.dW2 - (m_headIcon.width() / 2), c.dH - c.dH10 - c.dW, m_windIcon );

        QString      qsWind = QString::number( m_iWindBugSpeed );
        QFontMetrics windMetrics( med );
        QRect        windRect = windMetrics.boundingRect( qsWind );

#if defined( Q_OS_ANDROID )
        windRect.setWidth( windRect.width() * 4 );
#endif

        ahrs.setFont( med );
        ahrs.setPen( Qt::black );
        ahrs.drawText( c.dW + c.dW2 - (windRect.width() / 2), c.dH - c.dW - 3, qsWind );
        ahrs.setPen( Qt::white );
        ahrs.drawText( c.dW + c.dW2 - (windRect.width() / 2) - 1, c.dH - c.dW - 4, qsWind );

        // If long press triggered crosswind component display and the heading bug is set
        if( m_bShowCrosswind && (m_iHeadBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( Qt::cyan );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW + c.dW2, c.dH - c.dW - 10, c.dW + c.dW2, c.dH - 10.0 - c.dW2 );

            // Draw the crosswind component calculated from heading vs wind
            double dAng = fabs( static_cast<double>( m_iWindBugAngle ) - static_cast<double>( m_iHeadBugAngle ) );
            while( dAng > 180.0 )
                dAng -= 360.0;
            dAng = fabs( dAng );
            double dCrossComp = fabs( static_cast<double>( m_iWindBugSpeed ) * sin( dAng * ToRad ) );
            double dCrossPos = c.dH - (c.dW / 1.3) - 10.0;
            QString qsCrossAng = QString( "%1%2" ).arg( static_cast<int>( dAng ) ).arg( QChar( 176 ) );

            ahrs.resetTransform();
            ahrs.translate( c.dW + c.dW2, c.dH - c.dW2 - 10.0 );
            ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
            ahrs.translate( -(c.dW + c.dW2), -(c.dH - c.dW2 - 10.0) );
            ahrs.setFont( large );
            ahrs.setPen( Qt::black );
            ahrs.drawText( c.dW + c.dW2 + 5.0, dCrossPos, QString::number( static_cast<int>( dCrossComp ) ) );
            ahrs.setPen( QColor( 0xFF, 0x90, 0x01 ) );
            ahrs.drawText( c.dW + c.dW2 + 3.0, dCrossPos - 2.0, QString::number( static_cast<int>( dCrossComp ) ) );
            ahrs.setFont( med );
            ahrs.setPen( Qt::black );
            ahrs.drawText( c.dW + c.dW2 + 5.0, dCrossPos + c.iMedFontHeight - 5, qsCrossAng );
            ahrs.setPen( Qt::cyan );
            ahrs.drawText( c.dW + c.dW2 + 3.0, dCrossPos + c.iMedFontHeight - 7, qsCrossAng );
        }

        ahrs.resetTransform();
    }

    // Draw the Altitude tape
    ahrs.fillRect( c.dW - c.dW5 - c.dW40, 0, c.dW5 + c.dW40, c.dH, QColor( 0, 0, 0, 100 ) );
    ahrs.drawPixmap( c.dW - c.dW5 - c.dW40 + 5, c.dH2 + 10.0 - m_AltTape.height() + (g_situation.dBaroPressAlt * dPxPerFt) , m_AltTape );

    // Draw the altitude bug
    if( m_iAltBug >= 0 )
    {
        double dAltTip = c.dH2 - 10.0 - ((static_cast<double>( m_iAltBug ) - g_situation.dBaroPressAlt) * dPxPerFt);

#if !defined( Q_OS_ANDROID )
        dAltTip += c.dH40;
#endif
        ahrs.drawPixmap( c.dW - c.dW5 - c.dW20 - c.dW40, dAltTip - c.dH40, c.dW20, c.dH20, m_AltBug );
    }

    // Draw the vertical speed static pixmap
    ahrs.fillRect( c.dW - c.dW20 - c.dW40, 0.0, c.dW40, c.dH, QColor( 0, 0, 0, 100 ) );
    ahrs.drawPixmap( c.dW - c.dW20, 0, c.dW20, c.dH, m_VertSpeedTape );

    // Draw the vertical speed indicator
    ahrs.translate( 0.0, c.dH2 - (dPxPerVSpeed * g_situation.dGPSVertSpeed / 100.0 * 0.98) );   // 98% accounts for the slight margin on each end
    arrow.clear();
    arrow.append( QPoint( c.dW - m_pCanvas->scaledH( 30.0 ), 0.0 ) );
    arrow.append( QPoint( c.dW - m_pCanvas->scaledH( 20.0 ), m_pCanvas->scaledV( -7.0 ) ) );
    arrow.append( QPoint( c.dW, m_pCanvas->scaledV( -15.0 ) ) );
    arrow.append( QPoint( c.dW, m_pCanvas->scaledV( 15.0 ) ) );
    arrow.append( QPoint( c.dW - m_pCanvas->scaledH( 20.0 ), m_pCanvas->scaledV( 7.0 ) ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.drawPolygon( arrow );

    QString qsFullVspeed = QString::number( g_situation.dGPSVertSpeed / 100.0, 'f', 1 );
    QString qsFracVspeed = qsFullVspeed.right( 1 );
    QString qsIntVspeed = qsFullVspeed.left( qsFullVspeed.length() - 2 );
    QFontMetrics weeMetrics( wee );

    // Draw vertical speed indicator as In thousands and hundreds of FPM in tiny text on the vertical speed arrow
    ahrs.setFont( wee );
    QRect intRect( weeMetrics.boundingRect( qsIntVspeed ) );
#if defined( Q_OS_ANDROID )
    intRect.setWidth( intRect.width() * 4 );
#endif
    ahrs.drawText( c.dW - m_pCanvas->scaledH( 20.0 ), m_pCanvas->scaledV( 4.0 ), qsIntVspeed );
    ahrs.setFont( itsy );
    ahrs.drawText( c.dW - m_pCanvas->scaledH( 20.0 ) + intRect.width() + 2, m_pCanvas->scaledV( 4.0 ), qsFracVspeed );
    ahrs.resetTransform();

    // Draw the current altitude
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( QColor( 0, 0, 0, 175 ) );
    ahrs.drawRect( c.dW - c.dW5 - (c.dW * 0.0125), c.dH2 - (c.dHNum / 2.0) - (c.dH * 0.0075), c.dW5 + (c.dW * 0.025), c.dHNum + (c.dH * 0.015) );
    ahrs.setPen( Qt::white );
    Builder::buildNumber( &num, &c, static_cast<int>( g_situation.dBaroPressAlt ), 0 );
    ahrs.drawPixmap( c.dW - c.dW5, c.dH2 - (c.dHNum / 2.0), num );

    // Draw the Speed tape
    ahrs.fillRect( 0, 0, c.dW10 + 5.0, c.dH, QColor( 0, 0, 0, 100 ) );
    ahrs.drawPixmap( 5, c.dH2 + 5.0 - m_SpeedTape.height() + (g_situation.dGPSGroundSpeed * dPxPerKnot), m_SpeedTape );

    // Draw the current speed
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( QColor( 0, 0, 0, 175 ) );
    ahrs.drawRect( 0, c.dH2 - (c.dHNum / 2.0) - (c.dH * 0.0075), c.dW5, c.dHNum + (c.dH * 0.015) );
    Builder::buildNumber( &num, &c, static_cast<int>( g_situation.dGPSGroundSpeed ), 0 );
    ahrs.drawPixmap( c.dW * 0.0125, c.dH2 - (c.dHNum / 2.0), num );

    ahrs.setFont( wee );
    ahrs.setPen( Qt::black );
    ahrs.drawText( c.dW10 + c.dW40 + (c.dW80 / 2.0) + 1, c.dH2 + c.dH80 + 1, g_bUnitsKnots ? "KTS" : "MPH"  );
    ahrs.setPen( Qt::white );
    ahrs.drawText( c.dW10 + c.dW40 + (c.dW80 / 2.0), c.dH2 + c.dH80, g_bUnitsKnots ? "KTS" : "MPH"  );

    // Draw the heading value over the indicator
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW + c.dW2 - (c.dWNum * 3.0 / 2.0) - (c.dW * 0.0125), 10.0, (c.dWNum * 3.0) + (c.dW * 0.025), c.dHNum + (c.dH * 0.015) );
    Builder::buildNumber( &num, &c, static_cast<int>( g_situation.dAHRSGyroHeading ), 3 );
    ahrs.drawPixmap( c.dW + c.dW2 - (c.dWNum * 3.0 / 2.0), 10.0 + (c.dH * 0.0075), num );

    // Draw the magnetic deviation less/more controls
    ahrs.drawPixmap( c.dW + c.dW2 - (c.dWNum * 3.0 / 2.0) - (c.dW * 0.0125) - c.dW10,
                     10.0 + (c.dH * 0.0075),
                     c.dW20 + c.dW40,
                     c.dHNum,
                     *m_pMagHeadOffLessPixmap );
    ahrs.drawPixmap( c.dW + c.dW2 + (c.dWNum * 3.0 / 2.0) + (c.dW * 0.0125) + c.dW10 - c.dW20 - c.dW40,
                     10.0 + (c.dH * 0.0075),
                     c.dW20 + c.dW40,
                     c.dHNum,
                    *m_pMagHeadOffMorePixmap );

    // Draw the G-Force indicator box and scale
    ahrs.setFont( tiny );
    ahrs.setPen( Qt::black );
    ahrs.drawText( c.dW2 - c.dW20 - c.dW10 - (c.iTinyFontWidth / 2) + 1, c.dH - 15, "2" );
    ahrs.drawText( c.dW2 - c.dW20 - (c.iTinyFontWidth / 2) + 1, c.dH - 15, "0" );
    ahrs.drawText( c.dW2 - c.dW20 + c.dW10 - (c.iTinyFontWidth / 2) + 1, c.dH - 15, "2" );
    ahrs.setPen( Qt::white );
    ahrs.drawText( c.dW2 - c.dW20 - c.dW10 - (c.iTinyFontWidth / 2), c.dH - 16, "2" );
    ahrs.drawText( c.dW2 - c.dW20 - (c.iTinyFontWidth / 2), c.dH - 16, "0" );
    ahrs.drawText( c.dW2 - c.dW20 + c.dW10 - (c.iTinyFontWidth / 2), c.dH - 16, "2" );

    // Arrow for G-Force indicator
    arrow.clear();
    arrow.append( QPoint( 0, c.dH - c.iTinyFontHeight - 10.0 ) );
    arrow.append( QPoint( -14.0, c.dH - c.iTinyFontHeight - 25.0 ) );
    arrow.append( QPoint( 16.0, c.dH - c.iTinyFontHeight - 25.0 ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.translate( c.dW2 - c.dW20 + ((g_situation.dAHRSGLoad - 1.0) * c.dW10), 0.0 );
    ahrs.drawPolygon( arrow );
    ahrs.resetTransform();

    // Left Tank indicators background
    ahrs.drawPixmap( c.dW10 + c.dW20, c.dH2 + c.dH10, c.dW20, c.dH2 - c.dH5, m_Lfuel );
    // Tank indicators level
    QPen levelPen( Qt::black, c.dH40 + 4 );
    levelPen.setCapStyle( Qt::RoundCap );
    ahrs.setPen( levelPen );
    ahrs.drawLine( c.dW10 + c.dW20,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)),
                   c.dW40 + c.dW10 + c.dW20,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)) );
    levelPen.setWidth( c.dH40 );
    levelPen.setColor( QColor( 255, 150, 255 ) );
    ahrs.setPen( levelPen );
    ahrs.drawLine( c.dW10 + c.dW20,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)),
                   c.dW40 + c.dW10 + c.dW20,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)) );
    // Right Tank indicators background
    ahrs.drawPixmap( c.dW - c.dW5 - c.dW10 - 1, c.dH2 + c.dH10, c.dW20, c.dH2 - c.dH5, m_Rfuel );
    // Right Tank indicators level
    levelPen.setColor( Qt::black );
    levelPen.setWidth( c.dH40 + 4 );
    ahrs.setPen( levelPen );
    ahrs.drawLine( c.dW - c.dW5 - c.dW20,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)),
                   c.dW - c.dW40 - c.dW5 - c.dW20 - 1,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)) );
    levelPen.setWidth( c.dH40 );
    levelPen.setColor( QColor( 255, 150, 255 ) );
    ahrs.setPen( levelPen );
    ahrs.drawLine( c.dW - c.dW5 - c.dW20,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)),
                   c.dW - c.dW40 - c.dW5 - c.dW20 - 1,
                   c.dH2 + c.dH10 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)) );

    // Tank indicator active indicators
    ahrs.setFont( large );
    if( m_bFuelFlowStarted )
    {
        QPen fuelPen( Qt::yellow, c.dH80 );

        ahrs.setPen( fuelPen );

        if( !m_tanks.bDualTanks )
        {
            ahrs.drawLine( c.dW10, c.dH2 + c.dH10 - c.dH80 - 5, c.dW10, c.dH2 + c.dH10 - c.dH80 - 5 );
            ahrs.drawLine( c.dW - c.dW5 - c.dW10, c.dH2 + c.dH10 - c.dH80 - 5, c.dW - c.dW5, c.dH2 + c.dH10 - c.dH80 - 5 );
        }
        else
        {
            if( m_tanks.bOnLeftTank )
                ahrs.drawLine( c.dW10, c.dH2 + c.dH10 - 15, c.dW5, c.dH2 + c.dH10 - 15 );
            else
                ahrs.drawLine( c.dW - c.dW5 - c.dW10, c.dH2 + c.dH10 - 15, c.dW - c.dW5, c.dH2 + c.dH10 - 15 );
        }
    }

    // Update the airport positions
    if( m_settings.eShowAirports != Canvas::ShowNoAirports )
        updateAirports( &ahrs, &c );

    // Update the traffic positions
    updateTraffic( &ahrs, &c );

    if( m_bShowGPSDetails )
        paintInfo( &ahrs, &c );
    else if( m_bDisplayTanksSwitchNotice )
        paintSwitchNotice( &ahrs, &c );

    if( (m_iTimerMin >= 0) && (m_iTimerSec >= 0) )
        paintTimer( &ahrs, &c );

    ahrs.drawPixmap( c.dW + c.dW40, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20, m_DirectTo );
    ahrs.drawPixmap( c.dW + c.dW40 + c.dH20, c.dH - c.dH20 - c.dH40, c.dH20, c.dH20, m_FromTo );

    drawDayMode( &ahrs, &c );
}


void AHRSCanvas::timerReminder( int iMinutes, int iSeconds )
{
    CanvasConstants c = m_pCanvas->constants();

    m_iTimerMin = iMinutes;
    m_iTimerSec = iSeconds;

    if( (iMinutes == 0) && (iSeconds == 0) )
    {
        TimerDialog  dlg( this );

        dlg.setGeometry( c.dW2 - c.dW5, c.dH - c.dW2 - 20 - c.dH5, c.dW5 + c.dW5, c.dH5 + c.dH5 );

        int          iSel = dlg.exec();
        AHRSMainWin *pMainWin = static_cast<AHRSMainWin *>( parentWidget()->parentWidget() );

        if( iSel == QDialog::Rejected )
        {
            m_iTimerMin = m_iTimerSec = -1;
            return;
        }
        else if( iSel == TimerDialog::Restart )
            pMainWin->restartTimer();
        else if( iSel == TimerDialog::Change )
            pMainWin->changeTimer();
    }
}


void AHRSCanvas::updateAirports( QPainter *pAhrs, CanvasConstants *c )
{
    Airport      ap;
    QLineF       ball;
    double	     dPxPerNM = static_cast<double>( c->dW - 20.0 ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
    double       dAPDist;
    QPen         apPen( Qt::magenta );
    QLineF       runwayLine;
    int          iRunway, iAPRunway;
    double       dAirportDiam = c->dWa * (m_bPortrait ? 0.03125 : 0.01875);
    QPainterPath maskPath;
    QFont        apFont;
    QRect        apRect;

#if defined( Q_OS_ANDROID )
        apFont = tiny;
#else
        apFont = wee;
#endif

    QFontMetrics apMetrics( apFont );

    pAhrs->setFont( apFont );

    maskPath.addEllipse( 20.0 + (m_bPortrait ? 0 : c->dW),
                         c->dH - c->dW,
                         c->dW - 40.0, c->dW - 40.0 );
    pAhrs->setClipPath( maskPath );

    apPen.setWidth( c->iThinPen );
    pAhrs->setBrush( Qt::NoBrush );
    foreach( ap, m_airports )
    {
        if( ap.bGrass && (m_settings.eShowAirports == Canvas::ShowPavedAirports) )
            continue;
        else if( (!ap.bGrass) && (m_settings.eShowAirports == Canvas::ShowGrassAirports) )
            continue;

        apRect = apMetrics.boundingRect( ap.qsID );

        dAPDist = ap.bd.dDistance * dPxPerNM;

        ball.setP1( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - c->dW2 - 30.0 ) );
        ball.setP2( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - c->dW2 - 30.0 - dAPDist ) );

        // Traffic angle in reference to you (which clock position they're at)
        ball.setAngle( -(ap.bd.dBearing + g_situation.dAHRSGyroHeading) + 180.0 );

        apPen.setWidth( c->iThinPen );
        apPen.setColor( Qt::black );
        pAhrs->setPen( apPen );
        pAhrs->drawEllipse( ball.p2().x() - (dAirportDiam / 2.0) + 1, ball.p2().y() - (dAirportDiam / 2.0) + 1, dAirportDiam, dAirportDiam );
        apPen.setColor( Qt::magenta );
        pAhrs->setPen( apPen );
        pAhrs->drawEllipse( ball.p2().x() - (dAirportDiam / 2.0), ball.p2().y() - (dAirportDiam / 2.0), dAirportDiam, dAirportDiam );
        apPen.setColor( Qt::black );
        pAhrs->setPen( apPen );
        pAhrs->drawText( ball.p2().x() - (dAirportDiam / 2.0) - (apRect.width() / 2) + 2, ball.p2().y() - (dAirportDiam / 2.0) + apRect.height() - 1, ap.qsID );
        // Draw the runways and tiny headings after the black ID shadow but before the yellow ID text
        if( (m_dZoomNM <= 30) && m_settings.bShowRunways )
        {
            for( iRunway = 0; iRunway < ap.runways.count(); iRunway++ )
            {
                iAPRunway = ap.runways.at( iRunway );
                runwayLine.setP1( QPointF( ball.p2().x(), ball.p2().y() ) );
                runwayLine.setP2( QPointF( ball.p2().x(), ball.p2().y() + (dAirportDiam * 2.0) ) );
                runwayLine.setAngle( 270.0 - static_cast<double>( iAPRunway ) );
                apPen.setColor( Qt::magenta );
                apPen.setWidth( c->iThickPen );
                pAhrs->setPen( apPen );
                pAhrs->drawLine( runwayLine );
                if( ((iAPRunway - g_situation.dAHRSGyroHeading) > 90) && ((iAPRunway - g_situation.dAHRSGyroHeading) < 270) )
                    runwayLine.setLength( runwayLine.length() + c->dW80 );
                QPixmap num( 128, 84 ); // It would need to be resized back anyway so creating a new one each time is faster
                num.fill( Qt::transparent );
                Builder::buildNumber( &num, c, iAPRunway / 10, 2 );
                num = num.scaledToWidth( c->dW20, Qt::SmoothTransformation );
                pAhrs->drawPixmap( runwayLine.p2(), num );
            }
        }
        apPen.setColor( Qt::yellow );
        pAhrs->setPen( apPen );
        pAhrs->drawText( ball.p2().x() - (dAirportDiam / 2.0) - (apRect.width() / 2) + 1, ball.p2().y() - (dAirportDiam / 2.0) + apRect.height() - 2, ap.qsID );
    }

    pAhrs->setClipping( false );
}


#if defined( Q_OS_ANDROID )
void AHRSCanvas::orient( bool bPortrait )
{
    if( !m_bInitialized )
        return;

    m_bInitialized = false;
    m_bPortrait = bPortrait;

    killTimer( m_iDispTimer );
    init();
}
#endif


void AHRSCanvas::drawDayMode( QPainter *pAhrs, CanvasConstants *c )
{
    if( !g_bDayMode )
    {
        pAhrs->fillRect( 0.0, 0.0, c->dWa, c->dH, QColor( 0, 0, 0, 128 ) );
    }
}


void AHRSCanvas::swipeLeft()
{
    AHRSMainWin *pMainWin = static_cast<AHRSMainWin *>( parentWidget()->parentWidget() );

    pMainWin->menu();
    update();
}


void AHRSCanvas::swipeRight()
{
    update();
}


void AHRSCanvas::swipeUp()
{
    zoomOut();
    update();
}


void AHRSCanvas::swipeDown()
{
    zoomIn();
    update();
}


void AHRSCanvas::drawDirectOrFromTo( QPainter *pAhrs, CanvasConstants *pC )
{
    QPen coursePen( Qt::yellow, 12, Qt::SolidLine, Qt::RoundCap );

    if( m_directAP.qsID != "NULL" )
    {
        double dPxPerNM = static_cast<double>( pC->dW - 20.0 ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile
        QLineF ball;

        TrafficMath::updateAirport( &m_directAP );

        if( m_bPortrait )
        {
            ball.setP1( QPointF( pC->dW2, pC->dH - pC->dW2 - 30.0 ) );
            ball.setP2( QPointF( pC->dW2, pC->dH - pC->dW2 - 30.0 - (m_directAP.bd.dDistance * dPxPerNM) ) );
        }
        else
        {
            ball.setP1( QPointF( pC->dW + pC->dW2, pC->dH - pC->dW2 - 30.0 ) );
            ball.setP2( QPointF( pC->dW + pC->dW2, pC->dH - pC->dW2 - 30.0 - (m_directAP.bd.dDistance * dPxPerNM) ) );
        }
        if( ball.length() > (pC->dW2 - 30.0) )
            ball.setLength( pC->dW2 - 30.0 );

        // Traffic angle in reference to you (which clock position they're at)
        ball.setAngle( -(m_directAP.bd.dBearing + g_situation.dAHRSGyroHeading) + 180.0 );

        pAhrs->setPen( coursePen );
        pAhrs->drawLine( ball );
    }
    else if( m_fromAP.qsID != "NULL" )
    {
        double  dPxPerNM = static_cast<double>( pC->dW - 20.0 ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile
        QLineF  ball;
        QPointF fromPt, toPt;

        TrafficMath::updateAirport( &m_fromAP );
        TrafficMath::updateAirport( &m_toAP );

        if( m_bPortrait )
        {
            ball.setP1( QPointF( pC->dW2, pC->dH - pC->dW2 - 30.0 ) );
            ball.setP2( QPointF( pC->dW2, pC->dH - pC->dW2 - 30.0 - (m_fromAP.bd.dDistance * dPxPerNM) ) );
        }
        else
        {
            ball.setP1( QPointF( pC->dW + pC->dW2, pC->dH - pC->dW2 - 30.0 ) );
            ball.setP2( QPointF( pC->dW + pC->dW2, pC->dH - pC->dW2 - 30.0 - (m_fromAP.bd.dDistance * dPxPerNM) ) );
        }
        ball.setAngle( -(m_fromAP.bd.dBearing + g_situation.dAHRSGyroHeading) + 180.0 );
        fromPt = ball.p2();
        if( m_bPortrait )
        {
            ball.setP1( QPointF( pC->dW2, pC->dH - pC->dW2 - 30.0 ) );
            ball.setP2( QPointF( pC->dW2, pC->dH - pC->dW2 - 30.0 - (m_toAP.bd.dDistance * dPxPerNM) ) );
        }
        else
        {
            ball.setP1( QPointF( pC->dW + pC->dW2, pC->dH - pC->dW2 - 30.0 ) );
            ball.setP2( QPointF( pC->dW + pC->dW2, pC->dH - pC->dW2 - 30.0 - (m_toAP.bd.dDistance * dPxPerNM) ) );
        }
        ball.setAngle( -(m_toAP.bd.dBearing + g_situation.dAHRSGyroHeading) + 180.0 );
        toPt = ball.p2();
        ball.setP1( fromPt );
        ball.setP2( toPt );

        pAhrs->setPen( coursePen );
        QPainterPath maskPath;
        maskPath.addEllipse( 20.0 + (m_bPortrait ? 0 : pC->dW),
                             pC->dH - pC->dW,
                             pC->dW - 40.0, pC->dW - 40.0 );
        pAhrs->setClipPath( maskPath );
        pAhrs->drawLine( ball );
        pAhrs->setClipping( false );
    }
}


void AHRSCanvas::setSwitchableTanks( bool bSwitchable )
{
    m_tanks.bDualTanks = bSwitchable;
    if( !bSwitchable )
    {
        m_tanks.dRightCapacity = 0.0;
        m_tanks.dRightRemaining = 0.0;
    }
}
