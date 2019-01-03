/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QPainter>
#include <QtDebug>
#include <QMouseEvent>
#include <QTimer>
#include <QFont>
#include <QLinearGradient>
#include <QLineF>

#include <math.h>

#include "AHRSCanvas.h"
#include "BugSelector.h"
#include "Keypad.h"
#include "AHRSMainWin.h"
#include "StreamReader.h"
#include "Builder.h"


extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;

StratuxSituation          g_situation;
QMap<int, StratuxTraffic> g_trafficMap;


AHRSCanvas::AHRSCanvas( QWidget *parent )
    : QWidget( parent ),
      m_pCanvas( 0 ),
      m_bInitialized( false ),
      m_iHeadBugAngle( -1 ),
      m_iWindBugAngle( -1 ),
      m_pRollIndicator( 0 ),
      m_pHeadIndicator( 0 ),
      m_pAltTape( 0 ),
      m_pSpeedTape( 0 ),
      m_pVertSpeedTape( 0 ),
      m_bUpdated( false ),
      m_bShowGPSDetails( false ),
      m_dZoomNM( 20.0 ),
      m_pZoomInPixmap( 0 ),
      m_pZoomOutPixmap( 0 )
{
    // Initialize AHRS settings
    // No need to init the traffic because it starts out as an empty QMap.
    StreamReader::initSituation( g_situation );

    // Preload the fancier icons that are impractical to paint programmatically
    m_planeIcon.load( ":/graphics/resources/Plane.png" );
    m_headIcon.load( ":/icons/resources/HeadingIcon.png" );
    m_windIcon.load( ":/icons/resources/WindIcon.png" );
    m_headIcon = m_headIcon.scaled( 32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    m_windIcon = m_windIcon.scaled( 32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    m_pZoomInPixmap = new QPixmap( ":/icons/resources/add.png" );
    m_pZoomOutPixmap = new QPixmap( ":/icons/resources/sub.png" );

    // Quick and dirty way to ensure we're shown full screen before any calculations happen
    QTimer::singleShot( 500, this, SLOT( init() ) );
}


// Delete everything that needs deleting
AHRSCanvas::~AHRSCanvas()
{
    if( m_pRollIndicator != 0 )
    {
        delete m_pRollIndicator;
        m_pRollIndicator = 0;
    }
    if( m_pHeadIndicator != 0 )
    {
        delete m_pHeadIndicator;
        m_pHeadIndicator = 0;
    }
    if( m_pAltTape != 0 )
    {
        delete m_pAltTape;
        m_pAltTape = 0;
    }
    if( m_pSpeedTape != 0 )
    {
        delete m_pSpeedTape;
        m_pSpeedTape = 0;
    }
    if( m_pVertSpeedTape != 0 )
    {
        delete m_pVertSpeedTape;
        m_pVertSpeedTape = 0;
    }
    if( m_pCanvas != 0 )
    {
        delete m_pCanvas;
        m_pCanvas = 0;
    }
    if( m_pZoomInPixmap != 0 )
    {
        delete m_pZoomInPixmap;
        m_pZoomInPixmap = 0;
    }
    if( m_pZoomOutPixmap != 0 )
    {
        delete m_pZoomOutPixmap;
        m_pZoomOutPixmap = 0;
    }
}


// Create the canvas utility instance, create the various pixmaps that are built up for fast painting
// and start the update timer.
void AHRSCanvas::init()
{
//  Testing traffic for when there isn't anything nearby
/*
    StratuxTraffic t;

	t.bHasADSB = true;
	t.dAge = 0.0;
	t.dBearing = 40.0;
	t.dDist = 19.5;
	t.dTrack = 45.0;
	t.dAlt = 3000.0;
	t.qsTail = "N450FL";
	g_trafficMap.insert( 12345, t );
	t.dBearing = 120.0;
	t.dDist = 13.0;
	t.dTrack = 250.0;
	t.dAlt = 5000.0;
	t.qsTail = "N1234K";
	g_trafficMap.insert( 54321, t );
*/
    m_pCanvas = new Canvas( width(), height() );

    CanvasConstants c = m_pCanvas->contants();

    m_pRollIndicator = new QPixmap( static_cast<int>( c.dW2 ), static_cast<int>( c.dH2 / c.dAspectP * 0.9 ) );	// The 90% scale factor is somewhat arbitrary but scales correctly so when it's rotated the angles line up
    m_pHeadIndicator = new QPixmap( static_cast<int>( c.dW - c.dW20 ), static_cast<int>( c.dW - c.dW20 ) );
    m_pAltTape = new QPixmap( static_cast<int>( c.dW5 ) - 50, c.iTinyFontHeight * 300 );						// 20000 ft / 100 x 1.5x font height
    m_pVertSpeedTape = new QPixmap( 40, c.dH2 );
    m_pSpeedTape = new QPixmap( static_cast<int>( c.dW5 ) - 50, c.iTinyFontHeight * 60 );						// 300 Knots x 2x font height
    m_pRollIndicator->fill( Qt::transparent );
    m_pHeadIndicator->fill( Qt::transparent );
    m_pAltTape->fill( Qt::transparent );
    m_pSpeedTape->fill( Qt::transparent );
    m_pVertSpeedTape->fill( Qt::transparent );
    Builder::buildRollIndicator( m_pRollIndicator, m_pCanvas );
    Builder::buildHeadingIndicator( m_pHeadIndicator, m_pCanvas );
    Builder::buildAltTape( m_pAltTape, m_pCanvas );
    Builder::buildSpeedTape( m_pSpeedTape, m_pCanvas );
    Builder::buildVertSpeedTape( m_pVertSpeedTape, m_pCanvas );
    m_iDispTimer = startTimer( 1000 );																			// Just drives updating the canvas if we're not currently receiving anything new.
    m_bInitialized = true;
}


// Resize event - on Android typically happens once at init
// Current android manifest locks the display to portait so it won't fire on rotating the device.
// This needs some thought though since I'm not sure it's really necessary to lock it into portrait mode.
void AHRSCanvas::resizeEvent( QResizeEvent *pEvent )
{
    if( pEvent == 0 )
        return;

    if( m_bInitialized )
    {
        if( m_iDispTimer != 0 )
            killTimer( m_iDispTimer );
        m_iDispTimer = 0;
        m_bInitialized = false;
        delete m_pRollIndicator;
        delete m_pHeadIndicator;
        delete m_pAltTape;
        delete m_pSpeedTape;
        delete m_pVertSpeedTape;
        init();
    }
}


// Just a utility timer that periodically updates the display when it's not being driven by the streams
// coming from the Stratux.
void AHRSCanvas::timerEvent( QTimerEvent *pEvent )
{
    if( pEvent == 0 )
        return;

    if( !m_bUpdated )
        update();

    cullTrafficMap();

    m_bUpdated = false;
}


// Where all the magic happens
void AHRSCanvas::paintEvent( QPaintEvent *pEvent )
{
    if( (!m_bInitialized) || (pEvent == 0) )
        return;

    QPainter        ahrs( this );
    CanvasConstants c = m_pCanvas->contants();
    double          dPitchH = c.dH4 + (g_situation.dAHRSpitch / 22.5 * c.dH4);     // The visible portion is only 1/4 of the 90 deg range
    QString         qsHead( QString( "%1" ).arg( static_cast<int>( g_situation.dAHRSGyroHeading ), 3, 10, QChar( '0' ) ) );
    QPolygon        shape;
    QPen            linePen( Qt::black );
    double          dSlipSkid = c.dW2 - ((g_situation.dAHRSSlipSkid / 100.0) * c.dW2);
	double          dPxPerVSpeed = c.dH2 / 40.0;
	QFont           itsy( wee );

	itsy.setPointSize( 6 );

    if( dSlipSkid < (c.dW4 + 25.0) )
        dSlipSkid = c.dW4 + 25.0;
    else if( dSlipSkid > (c.dW2 + c.dW4 - 25.0) )
        dSlipSkid = c.dW2 + c.dW4 - 25.0;

    linePen.setWidth( 2 );

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

    // Draw the top roll indicator
    ahrs.translate( c.dW2, (static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 );
    ahrs.rotate( -g_situation.dAHRSroll );
    ahrs.translate( -c.dW2, -((static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 )  );
    ahrs.drawPixmap( c.dW2 - c.dW4, c.dH40, *m_pRollIndicator );
    ahrs.resetTransform();

    QPolygonF arrow;

    arrow.append( QPointF( c.dW2, c.dH40 + 50.0 ) );
    arrow.append( QPointF( c.dW2 + 15.0, c.dH40 + 65.0 ) );
    arrow.append( QPointF( c.dW2 - 15, c.dH40 + 65.0 ) );
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
    ahrs.setPen( QPen( Qt::white, 2 ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW2 - c.dW10 + 10.0, c.dH - m_pHeadIndicator->height() - 45.0 - c.iMedFontHeight, c.dW5 - 20.0, c.iMedFontHeight );
    ahrs.setPen( Qt::white );
    ahrs.setFont( med );
    ahrs.drawText( c.dW2 - (m_pCanvas->medWidth( qsHead ) / 2), c.dH - m_pHeadIndicator->height() - 53.0, qsHead );

    // Arrow for heading position above heading dial
    arrow.clear();
    arrow.append( QPointF( c.dW2, c.dH - m_pHeadIndicator->height() - 15.0 ) );
    arrow.append( QPointF( c.dW2 + 15.0, c.dH - m_pHeadIndicator->height() - 35.0 ) );
    arrow.append( QPointF( c.dW2 - 15.0, c.dH - m_pHeadIndicator->height() - 35.0 ) );
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
        ahrs.drawPixmap( c.dW2 - 16.0, c.dH - m_pHeadIndicator->height() - 21.0, m_headIcon );
        ahrs.resetTransform();
    }

    // Draw the wind bug
    if( m_iWindBugAngle >= 0 )
    {
        ahrs.translate( c.dW2, c.dH - (m_pHeadIndicator->height() / 2) - 10.0 );
        ahrs.rotate( m_iWindBugAngle - g_situation.dAHRSGyroHeading );
        ahrs.translate( -c.dW2, -(c.dH - (m_pHeadIndicator->height() / 2) - 10.0) );
        ahrs.drawPixmap( c.dW2 - 16.0, c.dH - m_pHeadIndicator->height() - 21.0, m_windIcon );
        ahrs.resetTransform();
    }

    // Draw the Altitude tape
    ahrs.setClipRect( c.dW - c.dW5 + 1.0, 2.0, c.dW5 - 4.0, c.dH2 - 50 );
    ahrs.drawPixmap( c.dW - c.dW5 + 5.0, c.dH4 - (static_cast<double>( c.iTinyFontHeight ) * 1.5) - (((20000.0 - g_situation.dBaroPressAlt) / 20000.0) * m_pAltTape->height()), *m_pAltTape );
    ahrs.setClipping( false );

    // Draw the vertical speed static pixmap
    ahrs.drawPixmap( c.dW - 40.0, 0.0, *m_pVertSpeedTape );

    // Draw the vertical speed indicator
    ahrs.translate( 0.0, c.dH4 - (dPxPerVSpeed * g_situation.dGPSVertSpeed / 100.0) );
    arrow.clear();
    arrow.append( QPoint( c.dW - 30.0, 0.0 ) );
	arrow.append( QPoint( c.dW - 20.0, -7.0 ) );
    arrow.append( QPoint( c.dW, -15.0 ) );
    arrow.append( QPoint( c.dW, 15.0 ) );
	arrow.append( QPoint( c.dW - 20.0, 7.0 ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.drawPolygon( arrow );

	QString qsFullVspeed = QString::number( g_situation.dGPSVertSpeed / 100.0, 'f', 1 );
	QString qsFracVspeed = qsFullVspeed.right( 1 );
	QString qsIntVspeed = qsFullVspeed.left( qsFullVspeed.length() - 2 );
	QFontMetrics weeMetrics( wee );

	ahrs.setFont( wee );
	QRect intRect( weeMetrics.boundingRect( qsIntVspeed ) );
	ahrs.drawText( c.dW - 20.0, 4.0, qsIntVspeed );
	ahrs.setFont( itsy );
	ahrs.drawText( c.dW - 20.0 + intRect.width() + 2, 4.0, qsFracVspeed );
	ahrs.resetTransform();

    // Draw the current altitude
    ahrs.setPen( Qt::white );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW - c.dW5, c.dH4 - (c.iSmallFontHeight / 2), c.dW5 - 40.0, c.iSmallFontHeight + 1 );
    ahrs.setPen( Qt::white );
    ahrs.setFont( small );
    ahrs.drawText( c.dW - c.dW5 + 5, c.dH4 + (c.iSmallFontHeight / 2) - 4.0, QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );

    // Draw the Speed tape
    ahrs.setPen( Qt::white );
    ahrs.setClipRect( 2.0, 2.0, c.dW5 - 4.0, c.dH2 - 4.0 );
    ahrs.drawPixmap( 4, c.dH4 - c.iSmallFontHeight - (((300.0 - g_situation.dGPSGroundSpeed) / 300.0) * m_pSpeedTape->height()), *m_pSpeedTape );
    ahrs.setClipping( false );

    // Draw the current speed
	ahrs.setPen( Qt::white );
	ahrs.setBrush( Qt::black );
	ahrs.drawRect( 0, c.dH4 - (c.iSmallFontHeight / 2), c.dW5 - 40.0, c.iSmallFontHeight + 1 );
    ahrs.setFont( small );
    ahrs.drawText( 5, c.dH4 + (c.iSmallFontHeight / 2) - 4.0, QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );

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
    arrow.append( QPoint( 1, c.dH - c.iTinyFontHeight - 10.0 ) );
    arrow.append( QPoint( -14.0, c.dH - c.iTinyFontHeight - 25.0 ) );
    arrow.append( QPoint( 16.0, c.dH - c.iTinyFontHeight - 25.0 ) );
    ahrs.setPen( Qt::black );
    ahrs.setBrush( Qt::white );
    ahrs.translate( c.dW10 - 10.0 + ((g_situation.dAHRSGLoad - 1.0) * (c.dW10 - 10.0)), 0.0 );
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

    updateTraffic( &ahrs, &c );

    ahrs.drawPixmap( 10, static_cast<int>( c.dH ) - m_pHeadIndicator->height() + 25, *m_pZoomInPixmap );
    ahrs.drawPixmap( 10, static_cast<int>( c.dH ) - 120, *m_pZoomOutPixmap );

    if( m_bShowGPSDetails )
    {
        QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, c.dH - 50.0 );

        cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
        cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

        linePen.setColor( Qt::black );
        linePen.setWidth( 3 );
        ahrs.setPen( linePen );
        ahrs.setBrush( cloudyGradient );
        ahrs.drawRect( 50, 50, c.dW - 100, c.dH - 100 );
        ahrs.setFont( med );
        ahrs.drawText( 100, 100, "GPS Status" );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 3),  QString( "GPS Satellites Seen: %1" ).arg( g_situation.iGPSSatsSeen ) );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 5),  QString( "GPS Satellites Tracked: %1" ).arg( g_situation.iGPSSatsTracked ) );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 7),  QString( "GPS Satellites Locked: %1" ).arg( g_situation.iGPSSats ) );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 9),  QString( "GPS Fix Quality: %1" ).arg( g_situation.iGPSFixQuality ) );
    }
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

    pAhrs->setFont( tiny );

    // Draw a large dot for each aircraft; the outer edge of the heading indicator is calibrated to be 20 NM out from your position
	pAhrs->setFont( wee );
	foreach( traffic, trafficList )
    {
        // If bearing and distance were able to be calculated then show relative position
        if( traffic.bHasADSB )
        {
			double dTrafficDist = traffic.dDist * dPxPerNM;
			
			ball.setP1( QPointF( c->dW2, c->dH - (m_pHeadIndicator->height() / 2) - 10.0 ) );
			ball.setP2( QPointF( c->dW2, c->dH - (m_pHeadIndicator->height() / 2) - 10.0 - dTrafficDist ) );

			// Traffic angle in reference to you (which clock position they're at)
			ball.setAngle( -(traffic.dBearing + g_situation.dAHRSGyroHeading) + 90.0 );

			// Draw the black part of the track line
			planePen.setWidth( 5 );
            planePen.setColor( Qt::black );
			track.setP1( ball.p2() );
			track.setP2( QPointF( ball.p2().x(), ball.p2().y() + 30.0 ) );
            track.setAngle( -(traffic.dTrack - g_situation.dAHRSGyroHeading) + 90.0 );
            pAhrs->setPen( planePen );
			pAhrs->drawLine( track );

            // Draw the dot
            planePen.setWidth( 15 );
            planePen.setColor( Qt::black );
            pAhrs->setPen( planePen );
            pAhrs->drawPoint( ball.p2() );
            planePen.setColor( Qt::green );
            pAhrs->setPen( planePen );
            pAhrs->drawPoint( ball.p2().x() - 2, ball.p2().y() - 2 );

            // Draw the green part of the track line
            planePen.setWidth( 5 );
            planePen.setColor( Qt::green );
            track.setP1( QPointF( ball.p2().x() - 2, ball.p2().y() - 2 ) );
            track.setP2( QPointF( ball.p2().x() - 2, ball.p2().y() + 28.0 ) );
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
			pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0, traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
			pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + c->iWeeFontHeight, QString::number( static_cast<int>( traffic.dTrack ) ) );
			pAhrs->drawText( ball.p2().x() + 10.0, ball.p2().y() + 10.0 + (c->iWeeFontHeight * 2), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
			pAhrs->setPen( Qt::green );
			pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0, traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
			pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + c->iWeeFontHeight, QString::number( static_cast<int>( traffic.dTrack ) ) );
			pAhrs->drawText( ball.p2().x() + 9.0, ball.p2().y() + 9.0 + (c->iWeeFontHeight * 2), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
		}
    }

    // Draw the zoom level
    pAhrs->setFont( small );
    pAhrs->setPen( Qt::black );
    pAhrs->drawText( 6.0, c->dH - 62.0, QString( "%1 NM" ).arg( static_cast<int>( m_dZoomNM ) ) );
    pAhrs->setPen( Qt::green );
    pAhrs->drawText( 4.0, c->dH - 64.0, QString( "%1 NM" ).arg( static_cast<int>( m_dZoomNM ) ) );
}


// Situation (mostly AHRS data) update
void AHRSCanvas::situation( StratuxSituation s )
{
    g_situation = s;
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
            // Anything older than 30 seconds discard
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
void AHRSCanvas::mousePressEvent( QMouseEvent *pEvent )
{
    if( pEvent == 0 )
        return;

    if( m_bShowGPSDetails )
    {
        m_bShowGPSDetails = false;
        update();
        return;
    }

    // Otherwise we're looking for specific spots
    CanvasConstants c = m_pCanvas->contants();
    QPoint          pressPt( pEvent->pos() );
    QRect           headRect( c.dW2 - (m_pHeadIndicator->width() / 4), c.dH - m_pHeadIndicator->height() + (m_pHeadIndicator->height() / 4) - 10.0, m_pHeadIndicator->width() / 2, m_pHeadIndicator->height() / 2 );
    QRect           gpsRect( c.dW - c.dW5, c.dH - (c.iLargeFontHeight * 2.0), c.dW5, c.iLargeFontHeight * 2.0 );
    QRect           zoomInRect( 0.0, c.dH - m_pHeadIndicator->height() + 5, 75, 75 );
    QRect           zoomOutRect( 0.0, c.dH - 130.0, 75, 75 );
    int             iXoff = parentWidget()->parentWidget()->geometry().x();	// Likely 0 on a dedicated screen
	int             iYoff = parentWidget()->parentWidget()->geometry().y();	// Ditto

    // User pressed the GPS Lat/long area. This needs to be before the test for the heading indicator since it's within that area's rectangle
    if( gpsRect.contains( pressPt ) )
    {
        m_bShowGPSDetails = (!m_bShowGPSDetails);
    }
    // User pressed the zoom in button
    else if( zoomInRect.contains( pressPt ) )
        zoomIn();
    // User pressed the zoom out button
    else if( zoomOutRect.contains( pressPt ) )
        zoomOut();
    // User pressed on the heading indicator
    else if( headRect.contains( pressPt ) )
    {
        int         iButton = -1;
        BugSelector bugSel( this );

        bugSel.setGeometry( iXoff + c.dW2 - 100.0, iYoff + c.dH - (m_pHeadIndicator->height() / 2) - 85.0, 200.0, 150.0 );

        iButton = bugSel.exec();

        // Back was pressed
		if (iButton == QDialog::Rejected)
		{
			m_iHeadBugAngle = -1;
			m_iWindBugAngle = -1;
			return;
		}

        Keypad keypad( this );

		keypad.setGeometry( iXoff + c.dW2 - 200.0, iYoff + c.dH - (m_pHeadIndicator->height() / 2) - 170.0, 400.0, 320.0 );

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
                m_iWindBugAngle = iAngle;
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

    m_bUpdated = true;
    update();
}


void AHRSCanvas::zoomIn()
{
    m_dZoomNM -= 5.0;
    if( m_dZoomNM < 5.0 )
        m_dZoomNM = 5.0;
}


void AHRSCanvas::zoomOut()
{
    m_dZoomNM += 5.0;
    if( m_dZoomNM > 100.0 )
        m_dZoomNM = 100.0;
}