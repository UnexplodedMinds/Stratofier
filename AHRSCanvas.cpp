/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
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

#include <math.h>

#include "AHRSCanvas.h"
#include "BugSelector.h"
#include "Keypad.h"
#include "AHRSMainWin.h"
#include "StreamReader.h"
#include "Builder.h"
#include "RoscoPiDefs.h"
#include "TimerDialog.h"


extern QFont itsy;
extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;
extern QFont huge;

StratuxSituation          g_situation;
QMap<int, StratuxTraffic> g_trafficMap;
QSettings                *g_pSet;

QString g_qsRoscoPiVersion( "1.0.3" );


/*
IMPORTANT NOTE:

Wherever the dH constant is used for scaling, even for width or x offset, was deliberate, since
that constant is always the same regardless of whether we're in landscape or portrait mode.  The
dWa constant is used differently which is why the more convenient dH is used instead.
*/


AHRSCanvas::AHRSCanvas( QWidget *parent )
    : QWidget( parent ),
      m_pHeadIndicator( Q_NULLPTR ),
      m_pCanvas( Q_NULLPTR ),
      m_bInitialized( false ),
      m_iHeadBugAngle( -1 ),
      m_iWindBugAngle( -1 ),
      m_iWindBugSpeed( 0 ),
      m_pRollIndicator( Q_NULLPTR ),
      m_pAltTape( Q_NULLPTR ),
      m_pSpeedTape( Q_NULLPTR ),
      m_pVertSpeedTape( Q_NULLPTR ),
      m_pZoomInPixmap( Q_NULLPTR ),
      m_pZoomOutPixmap( Q_NULLPTR ),
      m_pMagHeadOffLessPixmap( Q_NULLPTR ),
      m_pMagHeadOffMorePixmap( Q_NULLPTR ),
      m_iUpdateCount( 0 ),
      m_bUpdated( false ),
      m_bShowGPSDetails( false ),
      m_bPortrait( true ),
      m_bLongPress( false ),
      m_longPressStart( QDateTime::currentDateTime() ),
      m_bShowCrosswind( false ),
      m_iTimerMin( -1 ),
      m_iTimerSec( -1 ),
      m_iMagDev( 0 ),
      m_bDisplayTanksSwitchNotice( false ),
      m_tanks( { 0.0, 0.0, 0.0, 0.0, 9.0, 10.0, 8.0, 5.0, 30, true, true, QDateTime::currentDateTime() } )
{
#ifndef ANDROID
    g_pSet = new QSettings( "./config.ini", QSettings::IniFormat );
#else
    g_pSet = new QSettings;
#endif

    // Initialize AHRS settings
    // No need to init the traffic because it starts out as an empty QMap.
    StreamReader::initSituation( g_situation );

    // Preload the fancier icons that are impractical to paint programmatically
    m_planeIcon.load( ":/graphics/resources/Plane.png" );
    m_headIcon.load( ":/icons/resources/HeadingIcon.png" );
    m_windIcon.load( ":/icons/resources/WindIcon.png" );

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
    if( m_pRollIndicator != Q_NULLPTR )
    {
        delete m_pRollIndicator;
        m_pRollIndicator = Q_NULLPTR;
    }
    if( m_pHeadIndicator != Q_NULLPTR )
    {
        delete m_pHeadIndicator;
        m_pHeadIndicator = Q_NULLPTR;
    }
    if( m_pAltTape != Q_NULLPTR )
    {
        delete m_pAltTape;
        m_pAltTape = Q_NULLPTR;
    }
    if( m_pSpeedTape != Q_NULLPTR )
    {
        delete m_pSpeedTape;
        m_pSpeedTape = Q_NULLPTR;
    }
    if( m_pVertSpeedTape != Q_NULLPTR )
    {
        delete m_pVertSpeedTape;
        m_pVertSpeedTape = Q_NULLPTR;
    }
    if( m_pCanvas != Q_NULLPTR )
    {
        delete m_pCanvas;
        m_pCanvas = Q_NULLPTR;
    }
    if( m_pZoomInPixmap != Q_NULLPTR )
    {
        delete m_pZoomInPixmap;
        m_pZoomInPixmap = Q_NULLPTR;
    }
    if( m_pZoomOutPixmap != Q_NULLPTR )
    {
        delete m_pZoomOutPixmap;
        m_pZoomOutPixmap = Q_NULLPTR;
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
    int             iZoomBtnSize = c.dH * (m_bPortrait ? 0.06 : 0.1 );
    int             iBugSize = static_cast<int>( c.dWa * (m_bPortrait ? 0.1333 : 0.08) );

    m_headIcon = m_headIcon.scaled( iBugSize, iBugSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    m_windIcon = m_windIcon.scaled( iBugSize, iBugSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    if( m_pRollIndicator != Q_NULLPTR )
        delete m_pRollIndicator;
    if( m_pVertSpeedTape != Q_NULLPTR )
        delete m_pVertSpeedTape;
    if( m_pHeadIndicator != Q_NULLPTR )
        delete m_pHeadIndicator;
    if( m_pAltTape != Q_NULLPTR )
        delete m_pAltTape;
    if( m_pSpeedTape != Q_NULLPTR )
        delete m_pSpeedTape;
    if( m_pZoomInPixmap != Q_NULLPTR )
        delete m_pZoomInPixmap;
    if( m_pZoomOutPixmap != Q_NULLPTR )
        delete m_pZoomOutPixmap;

    if( m_bPortrait )
    {
        m_pRollIndicator = new QPixmap( static_cast<int>( c.dW - c.dW5 ), static_cast<int>( c.dW - c.dW5 ) );
#if defined( Q_OS_ANDROID )
        m_pVertSpeedTape = new QPixmap( c.dWa * 0.08333, c.dH2 + m_pCanvas->scaledV( 20.0 ) );
#else
        m_pVertSpeedTape = new QPixmap( c.dWa * 0.08333, c.dH2 );
#endif
    }
    else
    {
        m_pRollIndicator = new QPixmap( static_cast<int>( c.dW2 + c.dW5 ), static_cast<int>( c.dW2 + c.dW5 ) );
        m_pVertSpeedTape = new QPixmap( c.dWa * 0.05, c.dH );
    }

    m_pHeadIndicator = new QPixmap( static_cast<int>( c.dW - c.dW20 ), static_cast<int>( c.dW - c.dW20 ) );
    m_pAltTape = new QPixmap( static_cast<int>( c.dW5 ), c.iTinyFontHeight * 300 );                         // 20000 ft / 100 x 1.5x font height
    m_pSpeedTape = new QPixmap( static_cast<int>( c.dW5 ) + (c.dWa * 0.0521), c.iTinyFontHeight * 60 );     // 300 Knots x 2x font height
    m_pRollIndicator->fill( Qt::transparent );
    m_pHeadIndicator->fill( Qt::transparent );
    m_pAltTape->fill( Qt::transparent );
    m_pSpeedTape->fill( Qt::transparent );
    m_pVertSpeedTape->fill( QColor( 0, 0, 0, 160 ) );
    Builder::buildRollIndicator( m_pRollIndicator, m_pCanvas, m_bPortrait );
    Builder::buildHeadingIndicator( m_pHeadIndicator, m_pCanvas );
    Builder::buildAltTape( m_pAltTape, m_pCanvas );
    Builder::buildSpeedTape( m_pSpeedTape, m_pCanvas );
    Builder::buildVertSpeedTape( m_pVertSpeedTape, m_pCanvas, m_bPortrait );

    QPixmap zIn( ":/graphics/resources/ZoomIn.png" );
    QPixmap zOut( ":/graphics/resources/ZoomOut.png" );
    QTransform trans;

    trans.rotate( -90.0 );
    m_pZoomInPixmap = new QPixmap( zIn.scaled( iZoomBtnSize, iZoomBtnSize ) );
    m_pZoomOutPixmap = new QPixmap( zOut.scaled( iZoomBtnSize, iZoomBtnSize ) );
    m_pMagHeadOffLessPixmap = new QPixmap( zIn.transformed( trans ).scaled( c.iMedFontHeight, c.iMedFontHeight ) );
    trans.rotate( 360.0 );
    m_pMagHeadOffMorePixmap = new QPixmap( zOut.transformed( trans ).scaled( c.iMedFontHeight, c.iMedFontHeight ) );

    m_iDispTimer = startTimer( 5000 );     // Update the in-memory airspace objects every 15 seconds
    m_bInitialized = true;
}


void AHRSCanvas::loadSettings()
{
    m_dZoomNM = g_pSet->value( "ZoomNM", 10.0 ).toDouble();
    m_bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();
    m_bShowOutsideHeading = g_pSet->value( "ShowOutsideHeading", true ).toBool();
    m_eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", 1 ).toInt() );
    m_iMagDev = g_pSet->value( "MagDev", 0.0 ).toInt();

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

    if( !m_bUpdated )
        update();

    cullTrafficMap();

    // If we have a valid GPS position, run the list of airports within range by threading it so it doesn't interfere with the display update
    if( (g_situation.dGPSlat != 0.0) && (g_situation.dGPSlong != 0.0) )
    {
        if( m_iUpdateCount < 6 )
            QtConcurrent::run( TrafficMath::updateNearbyAirports, &m_airports, m_dZoomNM, true );
        else
        {
            QtConcurrent::run( TrafficMath::updateNearbyAirports, &m_airports, m_dZoomNM, false );
            m_iUpdateCount = 0;
        }
        m_iUpdateCount++;
    }

    double dInterval = 0.00416666666667;   // 15 seconds / 3600 seconds (1 hour)

    // If the aircraft is moving and there is virtually no vertical movement then we are taxiing
    if( (g_situation.dGPSGroundSpeed > 5.0) && (g_situation.dBaroVertSpeed < 5.0) )
    {
        if( m_tanks.bOnLeftTank )
            m_tanks.dLeftRemaining -= (m_tanks.dFuelRateTaxi * dInterval);
        else
            m_tanks.dRightRemaining -= (m_tanks.dFuelRateTaxi * dInterval);
    }

    if( (m_tanks.lastSwitch.secsTo( qdtNow ) > (m_tanks.iSwitchIntervalMins * 60)) && m_tanks.bDualTanks )
        m_bDisplayTanksSwitchNotice = true;

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
	double				  dPxPerNM = static_cast<double>( m_pHeadIndicator->height() ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
	QLineF				  track, ball;
	double                dAlt;
	QString               qsSign;
    QFontMetrics          tinyMetrics( tiny );
    int                   iBallPenWidth = static_cast<int>( c->dH * (m_bPortrait ? 0.01875 : 0.03125) );
    int                   iCourseLinePenWidth = static_cast<int>( c->dH * (m_bPortrait ? 0.00625 : 0.010417) );
    int                   iCourseLineLength = static_cast<int>( c->dH * (m_bPortrait ? 0.0375 : 0.0625) );

    if( !m_bShowOutsideHeading )
    {
        QPainterPath maskPath;

        maskPath.addEllipse( (m_bPortrait ? 0 : c->dW) + c->dW2 - (m_pHeadIndicator->width() / 2),
                             c->dH - m_pHeadIndicator->height() - 10.0,
                             m_pHeadIndicator->width(), m_pHeadIndicator->height() );
        pAhrs->setClipPath( maskPath );
    }

    // Draw a large dot for each aircraft; the outer edge of the heading indicator is calibrated to be 20 NM out from your position
	foreach( traffic, trafficList )
    {
        // If bearing and distance were able to be calculated then show relative position
        if( traffic.bHasADSB )
        {
			double dTrafficDist = traffic.dDist * dPxPerNM;
            double dAltDist = traffic.dAlt - g_situation.dBaroPressAlt;

            if( m_bShowAllTraffic || (fabs( dAltDist ) < 5000) )
            {
			    ball.setP1( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - (m_pHeadIndicator->height() / 2) - 10.0 ) );
			    ball.setP2( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - (m_pHeadIndicator->height() / 2) - 10.0 - dTrafficDist ) );

			    // Traffic angle in reference to you (which clock position they're at)
			    ball.setAngle( -(traffic.dBearing + g_situation.dAHRSGyroHeading) + 180.0 );

			    // Draw the black part of the track line
                planePen.setWidth( static_cast<int>( c->dH * (m_bPortrait ? 0.00625 : 0.010417) ) );
                planePen.setColor( Qt::black );
			    track.setP1( ball.p2() );
                track.setP2( QPointF( ball.p2().x(), ball.p2().y() + iCourseLineLength ) );
                track.setAngle( -(traffic.dTrack - g_situation.dAHRSGyroHeading) + 90.0 );
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
                track.setAngle( -(traffic.dTrack - g_situation.dAHRSGyroHeading) + 90.0 );
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
			    pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + c->iWeeFontHeight, QString::number( static_cast<int>( traffic.dTrack ) ) );
			    pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + (c->iWeeFontHeight * 2), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
			    pAhrs->setPen( Qt::green );
                pAhrs->setFont( wee );
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0, traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
                pAhrs->setFont( tiny );
                pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + c->iWeeFontHeight, QString::number( static_cast<int>( traffic.dTrack ) ) );
			    pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + (c->iWeeFontHeight * 2), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
            }
		}
    }

    pAhrs->setClipping( false );

    // TODO: Move these out to their own painters

    QString qsZoom = QString( "%1 NM" ).arg( static_cast<int>( m_dZoomNM ) );
    QString qsMagDev = QString( "%1%2" ).arg( m_iMagDev ).arg( QChar( 0xB0 ) );
    QRect   zoomRect = tinyMetrics.boundingRect( qsZoom );

    if( m_iMagDev < 0 )
        qsMagDev.prepend( "   -" );
    else if( m_iMagDev > 0 )
        qsMagDev.prepend( "   +" );
    else
        qsMagDev.prepend( "   " );

#if defined( Q_OS_ANDROID )
    zoomRect.setWidth( zoomRect.width() * 4 );
    qsMagDev.prepend( "   " );
#endif

    // Draw the zoom level
    pAhrs->setFont( tiny );
    pAhrs->setPen( Qt::black );
    pAhrs->drawText( (c->dW * (m_bPortrait ? 1.0 : 2.0)) - zoomRect.width() - (m_bPortrait ? 5.0 : 2.0),
                     c->dH - (c->dH * (m_bPortrait ? 0.0775 : 0.1292)) + (m_bPortrait ? 0 : 5),
                     qsZoom );
    pAhrs->setPen( QColor( 80, 255, 80 ) );
    pAhrs->drawText( (c->dW * (m_bPortrait ? 1.0 : 2.0)) - zoomRect.width() - (m_bPortrait ? 7.0 : 4.0),
                     c->dH - (c->dH * (m_bPortrait ? 0.0775 : 0.1292)) - 2 + (m_bPortrait ? 0 : 5),
                     qsZoom );

    // Draw the magnetic deviation
    pAhrs->setPen( Qt::black );
    pAhrs->drawText( (c->dW * (m_bPortrait ? 1.0 : 2.0)) - zoomRect.width() - (m_bPortrait ? 5.0 : 2.0),
                     c->dH - (c->dH * (m_bPortrait ? 0.0775 : 0.1292)) + (m_bPortrait ? 0 : 5) - c->iTinyFontHeight - (m_bPortrait ? 10.0 : 0),
                     qsMagDev );
    pAhrs->setPen( Qt::yellow );
    pAhrs->drawText( (c->dW * (m_bPortrait ? 1.0 : 2.0)) - zoomRect.width() - (m_bPortrait ? 7.0 : 4.0),
                     c->dH - (c->dH * (m_bPortrait ? 0.0775 : 0.1292)) - 2 + (m_bPortrait ? 0 : 5) - c->iTinyFontHeight - (m_bPortrait ? 10.0 : 0),
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
        handleScreenPress( pEvent->pos() );

        m_bUpdated = true;
        update();
    }
}


void AHRSCanvas::handleScreenPress( const QPoint &pressPt )
{
    // Otherwise we're looking for specific spots
    CanvasConstants c = m_pCanvas->constants();
    QRect           headRect( (m_bPortrait ? c.dW2 : c.dW + c.dW2) - (m_pHeadIndicator->width() / 4), c.dH - m_pHeadIndicator->height() + (m_pHeadIndicator->height() / 4) - 10.0, m_pHeadIndicator->width() / 2, m_pHeadIndicator->height() / 2 );
    QRect           gpsRect( (m_bPortrait ? c.dW : c.dWa) - c.dW5, c.dH - (c.iLargeFontHeight * 2.0), c.dW5, c.iLargeFontHeight * 2.0 );
    QRectF          zoomInRect;
    QRectF          zoomOutRect;
    QRectF          magHeadLessRect;
    QRectF          magHeadMoreRect;
    int             iXoff = parentWidget()->parentWidget()->geometry().x();	// Likely 0 on a dedicated screen (mostly useful emulating on a PC)
    int             iYoff = parentWidget()->parentWidget()->geometry().y();	// Ditto
    double          dZoomBtnWidth = m_pCanvas->scaledH( 64.0 );
    double          dZoomBtnHeight = m_pCanvas->scaledV( 64.0 );

    if( m_bPortrait )
    {
        zoomInRect.setRect( 0.0, c.dH2 - m_pCanvas->scaledV( 50.0 ), dZoomBtnWidth, dZoomBtnHeight );
        zoomOutRect.setRect( 0.0, c.dH - m_pCanvas->scaledV( 50.0 ) - dZoomBtnHeight, dZoomBtnWidth, dZoomBtnHeight );

        magHeadLessRect.setRect( c.dW2 - c.dW10 - m_pMagHeadOffLessPixmap->width(),
                                 c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.06625 : 0.1104167)) - c.iMedFontHeight,
                                 m_pMagHeadOffLessPixmap->width(),
                                 m_pMagHeadOffLessPixmap->height() );
        magHeadMoreRect.setRect( c.dW2 - c.dW10 + c.dW5,
                                 c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.06625 : 0.1104167)) - c.iMedFontHeight,
                                 m_pMagHeadOffLessPixmap->width(),
                                 m_pMagHeadOffLessPixmap->height() );
    }
    else
    {
        zoomInRect.setRect( c.dW, 10.0, dZoomBtnWidth, dZoomBtnHeight );
        zoomOutRect.setRect( c.dWa - dZoomBtnWidth - 20.0, 10.0, dZoomBtnWidth, dZoomBtnHeight );

#if defined( Q_OS_ANDROID )
        magHeadMoreRect.setRect( c.dW + c.dW5,
                                 c.dH - c.iMedFontHeight - 10.0,
                                 m_pMagHeadOffMorePixmap->width(),
                                 m_pMagHeadOffMorePixmap->height() );
        magHeadLessRect.setRect( c.dW - m_pMagHeadOffLessPixmap->width(),
                                 c.dH - c.iMedFontHeight - 10.0,
                                 m_pMagHeadOffLessPixmap->width(),
                                 m_pMagHeadOffLessPixmap->height() );
#else
        magHeadLessRect.setRect( c.dW + c.dW2 - c.dW10 - m_pMagHeadOffLessPixmap->width(),
                                 c.dH - m_pHeadIndicator->height() - 36.0 - c.iMedFontHeight,
                                 m_pMagHeadOffLessPixmap->width(),
                                 m_pMagHeadOffLessPixmap->height() );
        magHeadMoreRect.setRect( c.dW + c.dW2 - c.dW10 + c.dW5,
                                 c.dH - m_pHeadIndicator->height() - 36.0 - c.iMedFontHeight,
                                 m_pMagHeadOffMorePixmap->width(),
                                 m_pMagHeadOffMorePixmap->height() );
#endif
    }

    // User pressed the GPS Lat/long area. This needs to be before the test for the heading indicator since it's within that area's rectangle
    if( gpsRect.contains( pressPt ) )
        m_bShowGPSDetails = (!m_bShowGPSDetails);
    // User pressed the zoom in button
    else if( zoomInRect.contains( pressPt ) )
    {
        zoomIn();
        update();
    }
    // User pressed the zoom out button
    else if( zoomOutRect.contains( pressPt ) )
    {
        zoomOut();
        update();
    }
    else if( magHeadLessRect.contains( pressPt ) )
    {
        m_iMagDev--;
        if( m_iMagDev < -30.0 )
            m_iMagDev++;
        g_pSet->setValue( "MagDev", m_iMagDev );
        g_pSet->sync();
    }
    else if( magHeadMoreRect.contains( pressPt ) )
    {
        m_iMagDev++;
        if( m_iMagDev > 30.0 )
            m_iMagDev--;
        g_pSet->setValue( "MagDev", m_iMagDev );
        g_pSet->sync();
    }
    // User pressed on the heading indicator
    else if( headRect.contains( pressPt ) )
    {
        int         iButton = -1;
        BugSelector bugSel( this );

        bugSel.setGeometry( iXoff + (m_bPortrait ? c.dW2 : c.dW + c.dW2) - static_cast<int>( c.dWa * (m_bPortrait ? 0.2083 : 0.125) ),
                            iYoff + c.dH - (m_pHeadIndicator->height() / 2) - 10 - static_cast<int>( c.dH * (m_bPortrait ? 0.125 : 0.2083) ),
                            static_cast<int>( c.dWa * 0.4167 ),
                            static_cast<int>( c.dH * 0.25 ) );
        bugSel.setMinimumSize( static_cast<int>( c.dWa * (m_bPortrait? 0.4167 : 0.25) ),
                               static_cast<int>( c.dH * (m_bPortrait ? 0.25 : 0.4167) ) );

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

        keypad.setGeometry( iXoff + (m_bPortrait ? c.dW2 : c.dW + c.dW2) - (m_bPortrait ? c.dWa * 0.4167 : c.dWa * 0.25),
                            iYoff + c.dH - (m_pHeadIndicator->height() / 2) - 10 - static_cast<int>( m_bPortrait ? c.dH * 0.2 : c.dH * 0.3333 ),
                            m_bPortrait ? c.dH2 : c.dH * 0.8333, m_bPortrait ? c.dH * 0.4 : c.dH * 0.6667 );
        keypad.setMinimumSize( m_bPortrait ? c.dH2 : c.dH * 0.8333, m_bPortrait ? c.dH * 0.4 : c.dH * 0.6667 );

        if( iButton == static_cast<int>( BugSelector::WindBug ) )
            keypad.setTitle( "WIND FROM HEADING" );

        if( keypad.exec() == QDialog::Accepted )
        {
            int iAngle = keypad.value();

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
}


void AHRSCanvas::mousePressEvent( QMouseEvent *pEvent )
{
    CanvasConstants c = m_pCanvas->constants();
    QRect           headRect( (m_bPortrait ? c.dW2 : c.dW + c.dW2) - (m_pHeadIndicator->width() / 4), c.dH - m_pHeadIndicator->height() + (m_pHeadIndicator->height() / 4) - 10.0, m_pHeadIndicator->width() / 2, m_pHeadIndicator->height() / 2 );

    if( headRect.contains( pEvent->pos() ) )
    {
        m_longPressStart = QDateTime::currentDateTime();
        m_bLongPress = true;
    }
}


void AHRSCanvas::zoomIn()
{
    m_dZoomNM -= 5.0;
    if( m_dZoomNM < 5.0 )
        m_dZoomNM = 5.0;
    g_pSet->setValue( "ZoomNM", m_dZoomNM );
    g_pSet->sync();
    TrafficMath::updateNearbyAirports( &m_airports, m_dZoomNM, false );
}


void AHRSCanvas::zoomOut()
{
    m_dZoomNM += 5.0;
    if( m_dZoomNM > 100.0 )
        m_dZoomNM = 100.0;
    g_pSet->setValue( "ZoomNM", m_dZoomNM );
    g_pSet->sync();
    TrafficMath::updateNearbyAirports( &m_airports, m_dZoomNM, false );
}


void AHRSCanvas::showAllTraffic( bool bAll )
{
    m_bShowAllTraffic = bAll;
    g_pSet->setValue( "ShowAllTraffic", bAll );
    g_pSet->sync();
    update();
}


void AHRSCanvas::showOutside( bool bOut )
{
    m_bShowOutsideHeading = bOut;
    g_pSet->setValue( "ShowOutsideHeading", bOut );
    g_pSet->sync();
    update();
}


void AHRSCanvas::showAirports( Canvas::ShowAirports eShow )
{
    m_eShowAirports = eShow;
    g_pSet->setValue( "ShowAirports", static_cast<int>( eShow ) );
    g_pSet->sync();
    update();
}


void AHRSCanvas::paintPortrait()
{
    QPainter        ahrs( this );
    CanvasConstants c = m_pCanvas->constants();
    double          dPitchH = c.dH4 + (g_situation.dAHRSpitch / 22.5 * c.dH4);     // The visible portion is only 1/4 of the 90 deg range
    QString         qsHead( QString( "%1" ).arg( static_cast<int>( g_situation.dAHRSGyroHeading ), 3, 10, QChar( '0' ) ) );
    QPolygon        shape;
    QPen            linePen( Qt::black );
    double          dSlipSkid = c.dW2 - ((g_situation.dAHRSSlipSkid / 100.0) * c.dW2);
    double          dPxPerVSpeed = c.dH2 / 40.0;

    if( dSlipSkid < (c.dW4 + 25.0) )
        dSlipSkid = c.dW4 + 25.0;
    else if( dSlipSkid > (c.dW2 + c.dW4 - 25.0) )
        dSlipSkid = c.dW2 + c.dW4 - 25.0;

    linePen.setWidth( c.iThinPen );

    ahrs.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing, true );

    // Tank indicators background
    ahrs.setPen( linePen );
    ahrs.setBrush( QColor( 0x2F, 0x2F, 0x2F ) );
    ahrs.drawRect( 0.0, c.dH2 + c.dH40, c.dW10, c.dH2 - c.dH5 );
    ahrs.drawRect( c.dW - c.dW10 - 1, c.dH2 + c.dH40, c.dW10, c.dH2 - c.dH5 );

    // Tank indicators level
    ahrs.setBrush( Qt::darkGreen );
    ahrs.drawRect( 0.0,
                   c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)),
                   c.dW10,
                   c.dH2 - c.dH5 - ((c.dH2 - c.dH5) * ((m_tanks.dLeftCapacity - m_tanks.dLeftRemaining) / m_tanks.dLeftCapacity)) );
    ahrs.drawRect( c.dW - c.dW10 - 1,
                   c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)),
                   c.dW10,
                   c.dH2 - c.dH5 - ((c.dH2 - c.dH5) * ((m_tanks.dRightCapacity - m_tanks.dRightRemaining) / m_tanks.dRightCapacity)) );

    // Tank indicators ticks
    for( int iTankLine = 0; iTankLine < 8; iTankLine++ )
    {
        ahrs.drawLine( 0.0, c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * static_cast<double>( iTankLine ) / 8.0),
                       (c.dW40 / 2.0), c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * static_cast<double>( iTankLine ) / 8.0) );
        ahrs.drawLine( c.dW - (c.dW40 / 2.0), c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * static_cast<double>( iTankLine ) / 8.0),
                       c.dW, c.dH2 + c.dH40 + ((c.dH2 - c.dH5) * static_cast<double>( iTankLine ) / 8.0) );
    }

    // Tank indicator labels
    ahrs.setFont( large );
    if( !m_tanks.bDualTanks )
        ahrs.setPen( Qt::white );
    else
    {
        if( m_tanks.bOnLeftTank )
            ahrs.setPen( Qt::cyan );
        else
            ahrs.setPen( Qt::white );
    }
    ahrs.drawText( (c.dW40 / 2.0) + 2, c.dH2 + (c.dH40 / 2.0) + c.iLargeFontHeight, "L" );
    if( !m_tanks.bDualTanks )
        ahrs.setPen( Qt::white );
    else
    {
        if( !m_tanks.bOnLeftTank )
            ahrs.setPen( Qt::cyan );
        else
            ahrs.setPen( Qt::white );
    }
    ahrs.drawText( c.dW - c.dW20 - (c.dW40 / 2.0), c.dH2 + (c.dH40 / 2.0) + c.iLargeFontHeight, "R" );

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

    ahrs.fillRect( -400.0, dPitchH, c.dW + 800.0, c.dH4, groundGradient );
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

    // Reset rotation
    ahrs.resetTransform();

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
    ahrs.translate( c.dW2, (static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -c.dW2, -((static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 )  );
    ahrs.drawPixmap( c.dW10, c.dH40, *m_pRollIndicator );
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

    // Draw the heading value over the indicator
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW2 - c.dW10 + 10.0, c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.06625 : 0.1104167)) - c.iMedFontHeight, c.dW5 - 20.0, c.iMedFontHeight );
#if defined( Q_OS_ANDROID )
    ahrs.setFont( large );
    ahrs.drawText( c.dW2 - (m_pCanvas->largeWidth( qsHead ) / 2) + 20, c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.075 : 0.125)), qsHead );
#else
    ahrs.setFont( med );
    ahrs.drawText( c.dW2 - (m_pCanvas->medWidth( qsHead ) / 2), c.dH - m_pHeadIndicator->height() - 58.0, qsHead );
#endif
    // Draw the magnetic deviation less/more controls
    ahrs.drawPixmap( c.dW2 - c.dW10 - m_pMagHeadOffLessPixmap->width(),
                     c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.06625 : 0.1104167)) - c.iMedFontHeight,
                     *m_pMagHeadOffLessPixmap );
    ahrs.drawPixmap( c.dW2 - c.dW10 + c.dW5,
                     c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.06625 : 0.1104167)) - c.iMedFontHeight,
                    *m_pMagHeadOffMorePixmap );

    // Arrow for heading position above heading dial
    arrow.clear();
    arrow.append( QPointF( c.dW2, c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.01875 : 0.03125)) ) );
    arrow.append( QPointF( c.dW2 + (c.dWa * (m_bPortrait ? 0.03125 : 0.01875)), c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.04375 : 0.04375)) ) );
    arrow.append( QPointF( c.dW2 - (c.dWa * (m_bPortrait ? 0.03125 : 0.01875)), c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.04375 : 0.04375)) ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the heading pixmap and rotate it to the current heading
    ahrs.translate( c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
    ahrs.rotate( -g_situation.dAHRSGyroHeading );
    ahrs.translate( -c.dW2, -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
    ahrs.drawPixmap( c.dW2 - (m_pHeadIndicator->width() / 2), c.dH - m_pHeadIndicator->height() - 10.0, *m_pHeadIndicator );
    ahrs.resetTransform();

    // Draw the central airplane
    ahrs.drawPixmap( QRect( c.dW2 - c.dW20, c.dH - 10 - (m_pHeadIndicator->height() / 2) - c.dH20, c.dW10, c.dH10 ), m_planeIcon );

    // Draw the heading bug
    if( m_iHeadBugAngle >= 0 )
    {
        ahrs.translate( c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
        ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -c.dW2, -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
        ahrs.drawPixmap( c.dW2 - (m_headIcon.width() / 2), c.dH - m_pHeadIndicator->height() - (m_headIcon.height() / 2) - 5.0, m_headIcon );

        // If long press triggered crosswind component display and the wind bug is set
        if( m_bShowCrosswind && (m_iWindBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( QColor( 0xFF, 0x90, 0x01 ) );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW2, c.dH - m_pHeadIndicator->height() + 20, c.dW2, c.dH - 10.0 - (m_pHeadIndicator->height() / 2) );
        }

        ahrs.resetTransform();
    }

    // Draw the wind bug
    if( m_iWindBugAngle >= 0 )
    {
        ahrs.translate( c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
        ahrs.rotate( m_iWindBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -c.dW2, -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
        ahrs.drawPixmap( c.dW2 - (m_windIcon.width() / 2), c.dH - m_pHeadIndicator->height() - (m_windIcon.height() / 2) - 5, m_windIcon );

        QString      qsWind = QString::number( m_iWindBugSpeed );
        QFontMetrics windMetrics( med );
        QRect        windRect = windMetrics.boundingRect( qsWind );

#if defined( Q_OS_ANDROID )
        windRect.setWidth( windRect.width() * 4 );
#endif

        ahrs.setFont( med );
        ahrs.setPen( Qt::black );
        ahrs.drawText( c.dW2 - (windRect.width() / 2), c.dH - m_pHeadIndicator->height() - 3, qsWind );
        ahrs.setPen( Qt::white );
        ahrs.drawText( c.dW2 - (windRect.width() / 2) - 1, c.dH - m_pHeadIndicator->height() - 4, qsWind );

        // If long press triggered crosswind component display and the heading bug is set
        if( m_bShowCrosswind && (m_iHeadBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( Qt::cyan );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW2, c.dH - m_pHeadIndicator->height() + 20, c.dW2, c.dH - 10.0 - (m_pHeadIndicator->height() / 2) ); 

            // Draw the crosswind component calculated from heading vs wind
            double dAng = fabs( static_cast<double>( m_iWindBugAngle ) - static_cast<double>( m_iHeadBugAngle ) );
            while( dAng > 180.0 )
                dAng -= 360.0;
            dAng = fabs( dAng );
            double dCrossComp = fabs( static_cast<double>( m_iWindBugSpeed ) * sin( dAng * ToRad ) );
            double dCrossPos = c.dH - (static_cast<double>( m_pHeadIndicator->height() ) / 1.3) - 10.0;
            QString qsCrossAng = QString( "%1%2" ).arg( static_cast<int>( dAng ) ).arg( QChar( 176 ) );

            ahrs.resetTransform();
            ahrs.translate( c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
            ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
            ahrs.translate( -c.dW2, -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
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
    ahrs.drawPixmap( c.dW - c.dW5 + 5.0, c.dH4 - (static_cast<double>( c.iTinyFontHeight ) * 1.5) - (((20000.0 - g_situation.dBaroPressAlt) / 20000.0) * m_pAltTape->height()) + 7.0, *m_pAltTape );
    ahrs.setClipping( false );

    // Draw the vertical speed static pixmap
    ahrs.drawPixmap( c.dW - (c.dWa * (m_bPortrait ? 0.08333 : 0.05)), 0.0, *m_pVertSpeedTape );

    // Draw the vertical speed indicator
    ahrs.translate( 0.0, c.dH4 - (dPxPerVSpeed * g_situation.dGPSVertSpeed / 100.0) );
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
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW - c.dW5, c.dH4 - (c.iSmallFontHeight / 2), c.dW5 - m_pCanvas->scaledH( 40.0 ), c.iSmallFontHeight + 1 );
    ahrs.setPen( Qt::white );
    if( g_situation.dBaroPressAlt < 10000.0 )
        ahrs.setFont( tiny );
    else
    {
        QFont weeBold( wee );
        weeBold.setBold( true );
        ahrs.setFont( weeBold );   // 5 digits won't quite fit
    }
#if defined( Q_OS_ANDROID )
    ahrs.drawText( c.dW - c.dW5 + m_pCanvas->scaledH( 4 ), c.dH4 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 9.0 ), QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );
#else
    ahrs.drawText( c.dW - c.dW5 + m_pCanvas->scaledH( 4 ), c.dH4 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 6.0 ), QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );
#endif
    // Draw the Speed tape
    ahrs.fillRect( 0, 0, c.dW10 + 5.0, c.dH2, QColor( 0, 0, 0, 100 ) );
    ahrs.setClipRect( 2.0, 2.0, c.dW5 - 4.0, c.dH2 - 4.0 );
    ahrs.drawPixmap( 4, c.dH4 - c.iSmallFontHeight - (((300.0 - g_situation.dGPSGroundSpeed) / 300.0) * m_pSpeedTape->height()), *m_pSpeedTape );
    ahrs.setClipping( false );

    // Draw the current speed
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( 0, c.dH4 - (c.iSmallFontHeight / 2), c.dW5 - m_pCanvas->scaledH( 40.0 ), c.iSmallFontHeight + 1 );
    ahrs.setFont( small );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( 4, c.dH4 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 6.0 ), QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );
#else
    ahrs.drawText( 4, c.dH4 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 4.0 ), QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );
#endif

    // Draw the G-Force indicator box and scale
    ahrs.setFont( tiny );
    ahrs.setPen( Qt::black );
    ahrs.drawText( 10, c.dH - 15, "2" );
    ahrs.drawText( c.dW10 - (c.iTinyFontWidth / 2), c.dH - 15, "0" );
    ahrs.drawText( c.dW5 - c.iTinyFontWidth - 10, c.dH - 15, "2" );
    ahrs.setPen( Qt::white );
    ahrs.drawText( 9, c.dH - 16, "2" );
    ahrs.drawText( c.dW10 - (c.iTinyFontWidth / 2), c.dH - 16, "0" );
    ahrs.drawText( c.dW5 - c.iTinyFontWidth - 11, c.dH - 16, "2" );

    // Arrow for G-Force indicator
    arrow.clear();
    arrow.append( QPoint( 1, c.dH - c.iTinyFontHeight - m_pCanvas->scaledV( 10.0 ) ) );
    arrow.append( QPoint( m_pCanvas->scaledH( -14.0 ), c.dH - c.iTinyFontHeight - m_pCanvas->scaledV( 25.0 ) ) );
    arrow.append( QPoint( m_pCanvas->scaledH( 16.0 ), c.dH - c.iTinyFontHeight - m_pCanvas->scaledV( 25.0 ) ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.translate( c.dW10 - m_pCanvas->scaledH( 10.0 ) + ((g_situation.dAHRSGLoad - 1.0) * (c.dW10 - m_pCanvas->scaledH( 10.0 ))), 0.0 );
    ahrs.drawPolygon( arrow );
    ahrs.resetTransform();

    // GPS Lat/Long
    QString qsLat = QString( "%1 %2" )
        .arg( fabs( g_situation.dGPSlat ), 0, 'f', 3, QChar( '0' ) )
        .arg( (g_situation.dGPSlat < 0.0) ? "E" : "W" );
    QString qsLong = QString( "%1 %2" )
        .arg( fabs( g_situation.dGPSlong ), 0, 'f', 3, QChar( '0') )
        .arg( (g_situation.dGPSlong < 0.0) ? "N" : "S" );

    ahrs.setPen( Qt::black );
    ahrs.setFont( tiny );
    ahrs.drawText( c.dW - c.dW5 + 10.0, c.dH - c.iTinyFontHeight - 15, qsLat );
    ahrs.drawText( c.dW - c.dW5 + 10.0, c.dH - 15, qsLong );
    ahrs.setPen( Qt::white );
    ahrs.drawText( c.dW - c.dW5 + 9.0, c.dH - c.iTinyFontHeight - 16, qsLat );
    ahrs.drawText( c.dW - c.dW5 + 9.0, c.dH - 16, qsLong );

    // Update the airport positions
    if( m_eShowAirports != Canvas::ShowNoAirports )
        updateAirports( &ahrs, &c );

    // Update the traffic positions
    updateTraffic( &ahrs, &c );

    // Draw the zoom in/out buttons
    ahrs.drawPixmap( 0, c.dH2 - m_pCanvas->scaledV( 50.0 ), *m_pZoomInPixmap );
    ahrs.drawPixmap( 0, c.dH - m_pCanvas->scaledV( 50.0 ) - m_pZoomOutPixmap->height(), *m_pZoomOutPixmap );
    
    if( m_bShowGPSDetails )
        paintInfo( &ahrs, &c );
    else if( m_bDisplayTanksSwitchNotice )
        paintSwitchNotice( &ahrs, &c );

    if( (m_iTimerMin >= 0) && (m_iTimerSec >= 0) )
        paintTimer( &ahrs, &c );
}


void AHRSCanvas::paintSwitchNotice( QPainter *pAhrs, CanvasConstants *c )
{
    QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, c->dH - 50.0 );
    QPen            linePen( Qt::black );
    int             iVoff = 0;

    cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
    cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

    linePen.setWidth( c->iThickPen );
    pAhrs->setPen( linePen );
    pAhrs->setBrush( cloudyGradient );

    pAhrs->drawRect( c->dW10, c->dH10, (m_bPortrait ? c->dW : c->dWa) - c->dW5, c->dH - c->dH5 );
    pAhrs->setFont( med );
    pAhrs->drawText( 75, 95 + iVoff + c->iMedFontHeight, "Tank Switch" );
    pAhrs->setFont( small );
    pAhrs->drawText( 75, 95 + iVoff + (c->iMedFontHeight * 3),  QString( "Switch to %1 tank" ).arg( m_tanks.bOnLeftTank ? "RIGHT" : "LEFT" ) );
    pAhrs->drawText( 75, 95 + iVoff + (c->iMedFontHeight * 5),  QString( "Press anywhere when done." ) );
}


void AHRSCanvas::paintTimer( QPainter *pAhrs, CanvasConstants *c )
{
    QString      qsTimer = QString( "%1:%2" ).arg( m_iTimerMin, 2, 10, QChar( '0' ) ).arg( m_iTimerSec, 2, 10, QChar( '0' ) );
    QFontMetrics largeMetrics( large );
    QRect        timerRect = largeMetrics.boundingRect( qsTimer );
    QPen         linePen( Qt::white );
    int          iFudge = 0;

#if defined( Q_OS_ANDROID )
    timerRect.setWidth( timerRect.width() * 4 );
    iFudge = 20;
#endif

    linePen.setWidth( c->iThinPen );

    pAhrs->setPen( linePen );
    pAhrs->setBrush( Qt::black );
    pAhrs->drawRect( c->dW2 - (m_bPortrait ? m_pCanvas->scaledH( 50.0 ) : m_pCanvas->scaledH( 70.0 )),
                     c->dH - (m_bPortrait ? m_pCanvas->scaledV( 70.0 ) : m_pCanvas->scaledV( 100.0 )),
                     m_pCanvas->scaledH( 100.0 ), m_pCanvas->scaledV( 35.0 ) );
    pAhrs->setPen( Qt::cyan );
    pAhrs->setFont( large );
    pAhrs->drawText( c->dW2 - (m_bPortrait ? 0 : m_pCanvas->scaledH( 20.0 )) - (timerRect.width() / 2) + iFudge, c->dH - (m_bPortrait ? m_pCanvas->scaledV( 41.0 ) : m_pCanvas->scaledV( 71.0 )), qsTimer );
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
    pAhrs->drawText( 75, m_bPortrait ? m_pCanvas->scaledV( 700.0 ) : m_pCanvas->scaledV( 390.0 ), QString( "Version: %1" ).arg( g_qsRoscoPiVersion ) );
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
    double          dPxPerVSpeed = (c.dH - 30.0) / 40.0;
    QFontMetrics    tinyMetrics( tiny );
    int             iGPSFudge = 0;

#if defined( Q_OS_ANDROID )
    iGPSFudge = 10;
#endif

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
    ahrs.translate( c.dW2 - c.dW20, (static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -(c.dW2 - c.dW20), -((static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 ) );
    ahrs.drawPixmap( c.dW4 - c.dW10 - c.dW20, c.dH40, *m_pRollIndicator );
    ahrs.resetTransform();

    ahrs.translate( -c.dW20, 0.0 );

    QPolygonF arrow;

    arrow.append( QPointF( c.dW2, c.dH40 + 50.0 ) );
    arrow.append( QPointF( c.dW2 + 15.0, c.dH40 + 65.0 ) );
    arrow.append( QPointF( c.dW2 - 15, c.dH40 + 65.0 ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the yellow pitch indicators
    ahrs.setBrush( Qt::yellow );
    shape.append( QPoint( c.dW5 + c.dW20, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 - c.dW10, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 - c.dW10 + 20, c.dH2 ) );
    shape.append( QPoint( c.dW2 - c.dW10, c.dH2 + c.dH160 + 4.0 ) );
    shape.append( QPoint( c.dW5 + c.dW20, c.dH2 + c.dH160 + 4.0 ) );
    ahrs.drawPolygon( shape );
    shape.clear();
    shape.append( QPoint( c.dW - c.dW5 - c.dW20, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 + c.dW10, c.dH2 - c.dH160 - 4.0 ) );
    shape.append( QPoint( c.dW2 + c.dW10 - 20, c.dH2 ) );
    shape.append( QPoint( c.dW2 + c.dW10, c.dH2 + c.dH160 + 4.0 ) );
    shape.append( QPoint( c.dW - c.dW5 - c.dW20, c.dH2 + c.dH160 + 4.0 ) );
    ahrs.drawPolygon( shape );
    shape.clear();
    shape.append( QPoint( c.dW2, c.dH2 ) );
    shape.append( QPoint( c.dW2 - c.dW20, c.dH2 + 20 ) );
    shape.append( QPoint( c.dW2 + c.dW20, c.dH2 + 20 ) );
    ahrs.drawPolygon( shape );

    ahrs.translate( c.dW20, 0.0 );

    // Arrow for heading position above heading dial
    arrow.clear();
    arrow.append( QPointF( c.dW + c.dW2, c.dH - m_pHeadIndicator->height() - (c.dH * (m_bPortrait ? 0.01875 : 0.03125)) ) );
    arrow.append( QPointF( c.dW + c.dW2 + (c.dWa * (m_bPortrait ? 0.03125 : 0.01875)), c.dH - m_pHeadIndicator->height() - 10.0 - (c.dH * (m_bPortrait ? 0.04375 : 0.04375)) ) );
    arrow.append( QPointF( c.dW + c.dW2 - (c.dWa * (m_bPortrait ? 0.03125 : 0.01875)), c.dH - m_pHeadIndicator->height() - 10.0 - (c.dH * (m_bPortrait ? 0.04375 : 0.04375)) ) );
    ahrs.setBrush( Qt::white );
    ahrs.setPen( Qt::black );
    ahrs.drawPolygon( arrow );

    // Draw the heading pixmap and rotate it to the current heading
    ahrs.translate( c.dW + c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
    ahrs.rotate( -g_situation.dAHRSGyroHeading );
    ahrs.translate( -(c.dW + c.dW2), -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
    ahrs.drawPixmap( c.dW + c.dW2 - (m_pHeadIndicator->width() / 2), c.dH - m_pHeadIndicator->height() - 10.0, *m_pHeadIndicator );
    ahrs.resetTransform();

    // Draw the central airplane
    ahrs.drawPixmap( QRect( c.dW + c.dW2 - c.dW20, c.dH - 10 - (m_pHeadIndicator->height() / 2) - c.dH20, c.dW10, c.dH10 ), m_planeIcon );

    // Draw the heading bug
    if( m_iHeadBugAngle >= 0 )
    {
        ahrs.translate( c.dW + c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
        ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -(c.dW + c.dW2), -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
        ahrs.drawPixmap( c.dW + c.dW2 - (m_headIcon.width() / 2), c.dH - m_pHeadIndicator->height() - 10 - m_pCanvas->scaledV( 20.0 ), m_headIcon );

        // If long press triggered crosswind component display and the wind bug is set
        if( m_bShowCrosswind && (m_iWindBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( QColor( 0xFF, 0x90, 0x01 ) );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW + c.dW2, c.dH - m_pHeadIndicator->height() + 20, c.dW + c.dW2, c.dH - 10.0 - (m_pHeadIndicator->height() / 2) );
        }

        ahrs.resetTransform();
    }

    // Draw the wind bug
    if( m_iWindBugAngle >= 0 )
    {
        ahrs.translate( c.dW + c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
        ahrs.rotate( m_iWindBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -(c.dW + c.dW2), -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
        ahrs.drawPixmap( c.dW + c.dW2 - (m_headIcon.width() / 2), c.dH - m_pHeadIndicator->height() - 10 - m_pCanvas->scaledV( 20.0 ), m_windIcon );

        QString      qsWind = QString::number( m_iWindBugSpeed );
        QFontMetrics windMetrics( med );
        QRect        windRect = windMetrics.boundingRect( qsWind );

#if defined( Q_OS_ANDROID )
        windRect.setWidth( windRect.width() * 4 );
#endif

        ahrs.setFont( med );
        ahrs.setPen( Qt::black );
        ahrs.drawText( c.dW + c.dW2 - (windRect.width() / 2), c.dH - m_pHeadIndicator->height() + m_pCanvas->scaledV( 9.0 ), qsWind );
        ahrs.setPen( Qt::white );
        ahrs.drawText( c.dW + c.dW2 - (windRect.width() / 2) - 1, c.dH - m_pHeadIndicator->height() + m_pCanvas->scaledV( 10.0 ), qsWind );

        // If long press triggered crosswind component display and the heading bug is set
        if( m_bShowCrosswind && (m_iHeadBugAngle >= 0) )
        {
            linePen.setWidth( c.iThinPen );
            linePen.setColor( Qt::cyan );
            ahrs.setPen( linePen );
            ahrs.drawLine( c.dW + c.dW2, c.dH - m_pHeadIndicator->height() + 20, c.dW + c.dW2, c.dH - 10.0 - (m_pHeadIndicator->height() / 2) ); 

            // Draw the crosswind component calculated from heading vs wind
            double dAng = fabs( static_cast<double>( m_iWindBugAngle ) - static_cast<double>( m_iHeadBugAngle ) );
            while( dAng > 180.0 )
                dAng -= 360.0;
            dAng = fabs( dAng );
            double dCrossComp = fabs( static_cast<double>( m_iWindBugSpeed ) * sin( dAng * ToRad ) );
            double dCrossPos = c.dH - (static_cast<double>( m_pHeadIndicator->height() ) / 1.4) - 10.0;
            QString qsCrossAng = QString( "%1%2" ).arg( static_cast<int>( dAng ) ).arg( QChar( 176 ) );

            ahrs.resetTransform();
            ahrs.translate( c.dW + c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
            ahrs.rotate( m_iHeadBugAngle - g_situation.dAHRSGyroHeading );
            ahrs.translate( -(c.dW + c.dW2), -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
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

    // Draw zoom in/out buttons
    ahrs.drawPixmap( c.dW + 10.0, 10, *m_pZoomInPixmap );
    ahrs.drawPixmap( c.dWa - m_pZoomOutPixmap->width() - 20, 10, *m_pZoomOutPixmap );

    // Draw the Altitude tape
#if defined( Q_OS_ANDROID )
    ahrs.fillRect( QRectF( c.dW - c.dW5 - m_pCanvas->scaledH( 10.0 ), 0.0, c.dW5 - m_pCanvas->scaledH( 30.0 ), c.dH - m_pMagHeadOffLessPixmap->height() - 10.0 ), QColor( 0, 0, 0, 100 ) );
#else
    ahrs.fillRect( QRectF( c.dW - c.dW5 - m_pCanvas->scaledH( 10.0 ), 0.0, c.dW5 - m_pCanvas->scaledH( 30.0 ), c.dH ), QColor( 0, 0, 0, 100 ) );
#endif
    ahrs.setClipRect( c.dW - c.dW5 - m_pCanvas->scaledH( 10.0 ), 2.0, c.dW5 + m_pCanvas->scaledH( 10.0 ), c.dH - 5 );
    ahrs.drawPixmap( c.dW - c.dW5 - m_pCanvas->scaledH( 10.0 ), c.dH2 - (static_cast<double>( c.iTinyFontHeight ) * 1.5) - (((20000.0 - g_situation.dBaroPressAlt) / 20000.0) * m_pAltTape->height()) + m_pCanvas->scaledV( 7 ), *m_pAltTape );
    ahrs.setClipping( false );

    // Draw the vertical speed static pixmap
    ahrs.drawPixmap( c.dW - m_pVertSpeedTape->width(), 0.0, *m_pVertSpeedTape );

    // Android has a thin indicator strip along the top that would obscure the heading so for the android version in landscape, put the heading in the bottom left middle corner
    ahrs.setBrush( Qt::black );
#if defined( Q_OS_ANDROID )
    ahrs.setPen( QPen( Qt::white, 3 ) );
    ahrs.drawRect( c.dW + 10.0, c.dH - c.iMedFontHeight - 10.0, c.dW5 - 20.0, c.iMedFontHeight );

    // Draw the magnetic deviation more control. For Android draw the less button further down so it's not obscured by the vertical speed pixmap
    ahrs.drawPixmap( c.dW + c.dW5,
                     c.dH - c.iMedFontHeight - 10.0,
                    *m_pMagHeadOffMorePixmap );
    ahrs.drawPixmap( c.dW - m_pMagHeadOffLessPixmap->width(),
                     c.dH - c.iMedFontHeight - 10.0,
                    *m_pMagHeadOffLessPixmap );
#else
    ahrs.setPen( QPen( Qt::white, 2 ) );
    ahrs.drawRect( c.dW + c.dW2 - c.dW10 + 10.0, c.dH - m_pHeadIndicator->height() - 36.0 - c.iMedFontHeight, c.dW5 - 20.0, c.iMedFontHeight );

    // Draw the magnetic deviation less/more controls
    ahrs.drawPixmap( c.dW + c.dW2 - c.dW10 - m_pMagHeadOffLessPixmap->width(),
                     c.dH - m_pHeadIndicator->height() - 36.0 - c.iMedFontHeight,
                    *m_pMagHeadOffLessPixmap );
    ahrs.drawPixmap( c.dW + c.dW2 - c.dW10 + c.dW5,
                     c.dH - m_pHeadIndicator->height() - 36.0 - c.iMedFontHeight,
                    *m_pMagHeadOffMorePixmap );
#endif

    // Draw the heading value over the indicator
    ahrs.setPen( Qt::white );
    ahrs.setFont( med );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( c.dW + c.dW10 - (m_pCanvas->medWidth( qsHead ) / 2) + 10.0, c.dH - m_pCanvas->scaledV( 15.0 ), qsHead );
#else
    ahrs.drawText( c.dW + c.dW2 - (m_pCanvas->medWidth( qsHead ) / 2), c.dH - m_pHeadIndicator->height() - 42.0, qsHead );
#endif

    // Draw the vertical speed indicator
    ahrs.translate( 0.0, c.dH2 - (dPxPerVSpeed * g_situation.dGPSVertSpeed / 100.0) );
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
    ahrs.drawText( c.dW - m_pCanvas->scaledH( 20.0 ) + intRect.width() + m_pCanvas->scaledH( 2.0 ), m_pCanvas->scaledV( 4.0 ), qsFracVspeed );
    ahrs.resetTransform();

    // Draw the current altitude
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW - c.dW5 - m_pCanvas->scaledH( 10.0 ), c.dH2 - (c.iSmallFontHeight / 2), c.dW5 - m_pCanvas->scaledH( 23.0 ), c.iSmallFontHeight + 1 );
    if( g_situation.dBaroPressAlt < 10000.0 )
        ahrs.setFont( tiny );
    else
    {
        QFont weeBold( wee );
        weeBold.setBold( true );
        ahrs.setFont( weeBold );   // 5 digits won't quite fit
    }
#if defined( Q_OS_ANDROID )
    ahrs.drawText( c.dW - c.dW5 - m_pCanvas->scaledH( 8.0 ), c.dH2 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 11.0 ), QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );
#else
    ahrs.drawText( c.dW - c.dW5 - m_pCanvas->scaledH( 8.0 ), c.dH2 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 6.0 ), QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );
#endif
    // Draw the Speed tape
    ahrs.fillRect( QRectF( 0.0, 0.0, c.dW5 - m_pCanvas->scaledH( 25.0 ), c.dH ), QColor( 0, 0, 0, 100 ) );
    ahrs.drawPixmap( 4, c.dH2 - c.iSmallFontHeight - (((300.0 - g_situation.dGPSGroundSpeed) / 300.0) * m_pSpeedTape->height()), *m_pSpeedTape );

    // Draw the current speed
    ahrs.setPen( QPen( Qt::white, c.iThinPen ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( 0, c.dH2 - (c.iSmallFontHeight / 2), c.dW5 - 25, c.iSmallFontHeight + 1 );
    ahrs.setFont( small );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( m_pCanvas->scaledH( 5 ), c.dH2 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 9.0 ), QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );
#else
    ahrs.drawText( m_pCanvas->scaledH( 5 ), c.dH2 + (c.iSmallFontHeight / 2) - m_pCanvas->scaledV( 4.0 ), QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );
#endif
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

    // GPS Lat/Long
    QString qsLat = QString( "%1 %2" )
        .arg( fabs( g_situation.dGPSlat ), 0, 'f', 3, QChar( '0' ) )
        .arg( (g_situation.dGPSlat < 0.0) ? "E" : "W" );
    QString qsLong = QString( "%1 %2" )
        .arg( fabs( g_situation.dGPSlong ), 0, 'f', 3, QChar( '0') )
        .arg( (g_situation.dGPSlong < 0.0) ? "N" : "S" );

    ahrs.setPen( Qt::black );
    ahrs.setFont( tiny );

    ahrs.drawText( (c.dW * 2.0) - c.dW5 + iGPSFudge, c.dH - c.iTinyFontHeight - 10, qsLat );
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + iGPSFudge, c.dH - 10, qsLong );
    ahrs.setPen( Qt::white );
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + iGPSFudge - 1, c.dH - c.iTinyFontHeight - 11, qsLat );
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + iGPSFudge - 1, c.dH - 11, qsLong );

    // Update the airport positions
    if( m_eShowAirports != Canvas::ShowNoAirports )
        updateAirports( &ahrs, &c );

    // Update the traffic positions
    updateTraffic( &ahrs, &c );

    if( m_bShowGPSDetails )
        paintInfo( &ahrs, &c );

    if( (m_iTimerMin >= 0) && (m_iTimerSec >= 0) )
        paintTimer( &ahrs, &c );
}


void AHRSCanvas::timerReminder( int iMinutes, int iSeconds )
{
    m_iTimerMin = iMinutes;
    m_iTimerSec = iSeconds;

    if( (iMinutes == 0) && (iSeconds == 0) )
    {
        TimerDialog  dlg( this );
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
    double	     dPxPerNM = static_cast<double>( m_pHeadIndicator->height() ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
    double       dAPDist;
    QPen         apPen( Qt::magenta );
    QLineF       runwayLine;
    int          iRunway, iAPRunway;
    double       dAirportDiam = c->dWa * (m_bPortrait ? 0.03125 : 0.01875);
#if defined( Q_OS_ANDROID )
    QFontMetrics itsyMetrics( wee );
#else
    QFontMetrics itsyMetrics( itsy );
#endif
    QRect        itsyRect = itsyMetrics.boundingRect( "00" );
    QString      qsRunway;
    QPainterPath maskPath;

    maskPath.addEllipse( (m_bPortrait ? 0 : c->dW) + c->dW2 - (m_pHeadIndicator->width() / 2),
                         c->dH - m_pHeadIndicator->height() - 10.0,
                         m_pHeadIndicator->width(), m_pHeadIndicator->height() );
    pAhrs->setClipPath( maskPath );

    apPen.setWidth( c->iThinPen );
    pAhrs->setBrush( Qt::NoBrush );
    foreach( ap, m_airports )
    {
        if( (!ap.bPublic) && (m_eShowAirports == Canvas::ShowPublicAirports) )
            continue;

        dAPDist = ap.bd.dDistance * dPxPerNM;

        ball.setP1( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - (m_pHeadIndicator->height() / 2) - 10.0 ) );
        ball.setP2( QPointF( (m_bPortrait ? 0 : c->dW) + c->dW2, c->dH - (m_pHeadIndicator->height() / 2) - 10.0 - dAPDist ) );

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
#if defined( Q_OS_ANDROID )
        pAhrs->setFont( tiny );
#else
        pAhrs->setFont( wee );
#endif
        pAhrs->drawText( ball.p2().x() - (dAirportDiam / 2.0) + 2, ball.p2().y() - (dAirportDiam / 2.0) - 1, ap.qsID );
        // Draw the runways and tiny headings after the black ID shadow but before the yellow ID text
        if( m_dZoomNM <= 30 )
        {
            for( iRunway = 0; iRunway < ap.runways.count(); iRunway++ )
            {
                iAPRunway = ap.runways.at( iRunway );
                runwayLine.setP1( QPointF( ball.p2().x(), ball.p2().y() ) );
                runwayLine.setP2( QPointF( ball.p2().x(), ball.p2().y() + (dAirportDiam * 2.0) ) );
                runwayLine.setAngle( -(iAPRunway - g_situation.dAHRSGyroHeading) + 90.0 );
                apPen.setColor( Qt::magenta );
                apPen.setWidth( c->iThickPen );
                pAhrs->setPen( apPen );
                pAhrs->drawLine( runwayLine );
#if defined( Q_OS_ANDROID )
                pAhrs->setFont( wee );
#else
                pAhrs->setFont( itsy );
#endif
                runwayLine.setLength( runwayLine.length() + 5 );
                if( ((iAPRunway - g_situation.dAHRSGyroHeading) > 90) && ((iAPRunway - g_situation.dAHRSGyroHeading) < 270) )
                    runwayLine.setLength( runwayLine.length() + itsyRect.height() - 5 );
                qsRunway = QString::number( iAPRunway );
                qsRunway.chop( 1 );
                pAhrs->drawText( runwayLine.p2().x() - (itsyRect.width() / 2), runwayLine.p2().y(), qsRunway );
            }
        }
#if defined( Q_OS_ANDROID )
        pAhrs->setFont( tiny );
#else
        pAhrs->setFont( wee );
#endif
        apPen.setColor( Qt::yellow );
        pAhrs->setPen( apPen );
        pAhrs->drawText( ball.p2().x() - (dAirportDiam / 2.0) + 1, ball.p2().y() - (dAirportDiam / 2.0) - 2, ap.qsID );
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

