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
#include <QSettings>

#include <math.h>

#include "AHRSCanvas.h"
#include "BugSelector.h"
#include "Keypad.h"
#include "AHRSMainWin.h"
#include "StreamReader.h"
#include "Builder.h"
#include "RoscoPiDefs.h"


extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;

StratuxSituation          g_situation;
QMap<int, StratuxTraffic> g_trafficMap;
QSettings                *g_pSet;

QString g_qsRoscoPiVersion( "0.0.5" );


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
      m_pZoomInPixmap( 0 ),
      m_pZoomOutPixmap( 0 ),
      m_iWindBugSpeed( 0 ),
      m_bPortrait( true ),
      m_bLongPress( false ),
      m_longPressStart( QDateTime::currentDateTime() ),
      m_bShowCrosswind( false )
{
    g_pSet = new QSettings( "/home/pi/RoscoPi/config.ini", QSettings::IniFormat );

    // Initialize AHRS settings
    // No need to init the traffic because it starts out as an empty QMap.
    StreamReader::initSituation( g_situation );

    // Preload the fancier icons that are impractical to paint programmatically
    m_planeIcon.load( ":/graphics/resources/Plane.png" );
    m_headIcon.load( ":/icons/resources/HeadingIcon.png" );
    m_windIcon.load( ":/icons/resources/WindIcon.png" );
    m_headIcon = m_headIcon.scaled( 64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    m_windIcon = m_windIcon.scaled( 64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    m_pZoomInPixmap = new QPixmap( ":/icons/resources/add.png" );
    m_pZoomOutPixmap = new QPixmap( ":/icons/resources/sub.png" );

    m_dZoomNM = g_pSet->value( "ZoomNM", 10.0 ).toDouble();
    m_bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();

    // Quick and dirty way to ensure we're shown full screen before any calculations happen
    QTimer::singleShot( 1500, this, SLOT( init() ) );
}


// Delete everything that needs deleting
AHRSCanvas::~AHRSCanvas()
{
    if( g_pSet != 0 )
    {
        g_pSet->sync();
        delete g_pSet;
        g_pSet = 0;
    }
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
	t.dAlt = 4500.0;
	t.qsTail = "N5432L";
	g_trafficMap.insert( 12345, t );
	t.dBearing = 120.0;
	t.dDist = 13.0;
	t.dTrack = 250.0;
	t.dAlt = 15000.0;
	t.qsTail = "N1234K";
	g_trafficMap.insert( 54321, t );
*/
    m_pCanvas = new Canvas( width(), height(), m_bPortrait );

    CanvasConstants c = m_pCanvas->contants();

    if( m_bPortrait )
    {
        m_pRollIndicator = new QPixmap( static_cast<int>( c.dW - c.dW5 ), static_cast<int>( c.dW - c.dW5 ) );
        m_pVertSpeedTape = new QPixmap( 40, c.dH2 );
    }
    else
    {
        m_pRollIndicator = new QPixmap( static_cast<int>( c.dW2 + c.dW5 ), static_cast<int>( c.dW2 + c.dW5 ) );
        m_pVertSpeedTape = new QPixmap( 40, c.dH );
    }
    m_pHeadIndicator = new QPixmap( static_cast<int>( c.dW - c.dW20 ), static_cast<int>( c.dW - c.dW20 ) );
    m_pAltTape = new QPixmap( static_cast<int>( c.dW5 + 50.0 ) - 50, c.iTinyFontHeight * 300 );						// 20000 ft / 100 x 1.5x font height
    m_pSpeedTape = new QPixmap( static_cast<int>( c.dW5 ) - 25, c.iTinyFontHeight * 60 );						    // 300 Knots x 2x font height
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
    m_iDispTimer = startTimer( 1000 );																			// Just drives updating the canvas if we're not currently receiving anything new.
    m_bInitialized = true;
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

    QString qsZoom = QString( "%1 NM" ).arg( static_cast<int>( m_dZoomNM ) );
    QRect   zoomRect = tinyMetrics.boundingRect( qsZoom );

    // Draw the zoom level
    pAhrs->setFont( tiny );
    pAhrs->setPen( Qt::black );
    pAhrs->drawText( (c->dW * (m_bPortrait ? 1.0 : 2.0)) - zoomRect.width() - (m_bPortrait ? 5.0 : 2.0), c->dH - 62.0 + (m_bPortrait ? 0 : 5), qsZoom );
    pAhrs->setPen( QColor( 80, 255, 80 ) );
    pAhrs->drawText( (c->dW * (m_bPortrait ? 1.0 : 2.0)) - zoomRect.width() - (m_bPortrait ? 7.0 : 4.0), c->dH - 64.0 + (m_bPortrait ? 0 : 5), qsZoom );
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
            if( abs( it.value().lastActualReport.secsTo( now ) ) > 30.0 )
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

    if( m_bLongPress && (m_longPressStart.msecsTo( qdtNow ) > 1000) )
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
    CanvasConstants c = m_pCanvas->contants();
    QRect           headRect( (m_bPortrait ? c.dW2 : c.dW + c.dW2) - (m_pHeadIndicator->width() / 4), c.dH - m_pHeadIndicator->height() + (m_pHeadIndicator->height() / 4) - 10.0, m_pHeadIndicator->width() / 2, m_pHeadIndicator->height() / 2 );
    QRect           gpsRect( (m_bPortrait ? c.dW : width()) - c.dW5, c.dH - (c.iLargeFontHeight * 2.0), c.dW5, c.iLargeFontHeight * 2.0 );
    QRect           zoomInRect;
    QRect           zoomOutRect;
    int             iXoff = parentWidget()->parentWidget()->geometry().x();	// Likely 0 on a dedicated screen (mostly useful emulating on a PC)
    int             iYoff = parentWidget()->parentWidget()->geometry().y();	// Ditto

    if( m_bPortrait )
    {
        zoomInRect.setRect( 0.0, c.dH - m_pHeadIndicator->height() + 5, 75, 75 );
        zoomOutRect.setRect( 0.0, c.dH - 130.0, 75, 75 );
    }
    else
    {
        zoomInRect.setRect( c.dW + 10, 10, 75, 75 );
        zoomOutRect.setRect( (c.dW * 2.0) - 50, 10, 75, 75 );
    }

    // User pressed the GPS Lat/long area. This needs to be before the test for the heading indicator since it's within that area's rectangle
    if( gpsRect.contains( pressPt ) )
        m_bShowGPSDetails = (!m_bShowGPSDetails);
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

        bugSel.setGeometry( iXoff + (m_bPortrait ? c.dW2 : c.dW + c.dW2) - 100.0, iYoff + c.dH - (m_pHeadIndicator->height() / 2) - 100.0, 200.0, 200.0 );

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

        keypad.setGeometry( iXoff + (m_bPortrait ? c.dW2 : c.dW + c.dW2) - 200.0, iYoff + c.dH - (m_pHeadIndicator->height() / 2) - 170.0, 400.0, 320.0 );
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
    CanvasConstants c = m_pCanvas->contants();
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
}


void AHRSCanvas::zoomOut()
{
    m_dZoomNM += 5.0;
    if( m_dZoomNM > 100.0 )
        m_dZoomNM = 100.0;
    g_pSet->setValue( "ZoomNM", m_dZoomNM );
    g_pSet->sync();
}


void AHRSCanvas::showAllTraffic( bool bAll )
{
    m_bShowAllTraffic = bAll;
    g_pSet->setValue( "ShowAllTraffic", bAll );
    update();
}


void AHRSCanvas::paintPortrait()
{
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
    ahrs.drawPixmap( c.dW10, c.dH40, *m_pRollIndicator );
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
        ahrs.drawPixmap( c.dW2 - 32.0, c.dH - m_pHeadIndicator->height() - 37.0, m_headIcon );

        // If long press triggered crosswind component display and the wind bug is set
        if( m_bShowCrosswind && (m_iWindBugAngle >= 0) )
        {
            linePen.setWidth( 2 );
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
        ahrs.drawPixmap( c.dW2 - 32.0, c.dH - m_pHeadIndicator->height() - 37.0, m_windIcon );

        QString      qsWind = QString::number( m_iWindBugSpeed );
        QFontMetrics windMetrics( med );
        QRect        windRect = windMetrics.boundingRect( qsWind );

        ahrs.setFont( med );
        ahrs.setPen( Qt::black );
        ahrs.drawText( c.dW2 - (windRect.width() / 2), c.dH - m_pHeadIndicator->height() - 3, qsWind );
        ahrs.setPen( Qt::white );
        ahrs.drawText( c.dW2 - (windRect.width() / 2) - 1, c.dH - m_pHeadIndicator->height() - 4, qsWind );

        // If long press triggered crosswind component display and the heading bug is set
        if( m_bShowCrosswind && (m_iHeadBugAngle >= 0) )
        {
            linePen.setWidth( 2 );
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

    // Draw vertical speed indicator as In thousands and hundreds of FPM in tiny text on the vertical speed arrow
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
    if( g_situation.dBaroPressAlt < 10000.0 )
        ahrs.setFont( small );
    else
    {
        QFont tinyBold( tiny );
        tinyBold.setBold( true );
        ahrs.setFont( tinyBold );   // 5 digits won't quite fit
    }
    ahrs.drawText( c.dW - c.dW5 + 4, c.dH4 + (c.iSmallFontHeight / 2) - 5.0, QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );

    // Draw the Speed tape
    ahrs.fillRect( 0, 0, c.dW10 + 5.0, c.dH2, QColor( 0, 0, 0, 100 ) );
    ahrs.setPen( Qt::white );
    ahrs.setClipRect( 2.0, 2.0, c.dW5 - 4.0, c.dH2 - 4.0 );
    ahrs.drawPixmap( 4, c.dH4 - c.iSmallFontHeight - (((300.0 - g_situation.dGPSGroundSpeed) / 300.0) * m_pSpeedTape->height()), *m_pSpeedTape );
    ahrs.setClipping( false );

    // Draw the current speed
    ahrs.setPen( Qt::white );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( 0, c.dH4 - (c.iSmallFontHeight / 2), c.dW5 - 40.0, c.iSmallFontHeight + 1 );
    ahrs.setFont( small );
    ahrs.drawText( 4, c.dH4 + (c.iSmallFontHeight / 2) - 5.0, QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );

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

    // Draw the zoom in/out buttons
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
        ahrs.setPen( Qt::blue );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 12), QString( "Version: %1" ).arg( g_qsRoscoPiVersion ) );
    }
}


void AHRSCanvas::paintLandscape()
{
    QPainter        ahrs( this );
    CanvasConstants c = m_pCanvas->contants();
    double          dPitchH = c.dH2 + (g_situation.dAHRSpitch / 22.5 * c.dH2);     // The visible portion is only 1/4 of the 90 deg range
    QString         qsHead( QString( "%1" ).arg( static_cast<int>( g_situation.dAHRSGyroHeading ), 3, 10, QChar( '0' ) ) );
    QPolygon        shape;
    QPen            linePen( Qt::black );
    double          dSlipSkid = c.dW2 - ((g_situation.dAHRSSlipSkid / 100.0) * c.dW2);
    double          dPxPerVSpeed = (c.dH - 30.0) / 40.0;
    QFont           itsy( wee );
    QFontMetrics    tinyMetrics( tiny );

    itsy.setPointSize( 6 );

    if( dSlipSkid < (c.dW4 + 25.0) )
        dSlipSkid = c.dW4 + 25.0;
    else if( dSlipSkid > (c.dW2 + c.dW4 - 25.0) )
        dSlipSkid = c.dW2 + c.dW4 - 25.0;

    linePen.setWidth( 2 );

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
    ahrs.translate( -(c.dW2 - c.dW20), -((static_cast<double>( m_pRollIndicator->height() ) / 2.0) + c.dH40 )  );
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

    // Draw the heading value over the indicator
    ahrs.setPen( QPen( Qt::white, 2 ) );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW + c.dW2 - c.dW10 + 10.0, c.dH - m_pHeadIndicator->height() - 36.0 - c.iMedFontHeight, c.dW5 - 20.0, c.iMedFontHeight );
    ahrs.setPen( Qt::white );
    ahrs.setFont( med );
    ahrs.drawText( c.dW + c.dW2 - (m_pCanvas->medWidth( qsHead ) / 2), c.dH - m_pHeadIndicator->height() - 45.0, qsHead );

    // Arrow for heading position above heading dial
    arrow.clear();
    arrow.append( QPointF( c.dW + c.dW2, c.dH - m_pHeadIndicator->height() - 14.0 ) );
    arrow.append( QPointF( c.dW + c.dW2 + 15.0, c.dH - m_pHeadIndicator->height() - 34.0 ) );
    arrow.append( QPointF( c.dW + c.dW2 - 15.0, c.dH - m_pHeadIndicator->height() - 34.0 ) );
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
        ahrs.drawPixmap( c.dW + c.dW2 - 32.0, c.dH - m_pHeadIndicator->height() - 37.0, m_headIcon );

        // If long press triggered crosswind component display and the wind bug is set
        if( m_bShowCrosswind && (m_iWindBugAngle >= 0) )
        {
            linePen.setWidth( 2 );
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
        ahrs.drawPixmap( c.dW + c.dW2 - 32.0, c.dH - m_pHeadIndicator->height() - 37.0, m_windIcon );

        QString      qsWind = QString::number( m_iWindBugSpeed );
        QFontMetrics windMetrics( med );
        QRect        windRect = windMetrics.boundingRect( qsWind );

        ahrs.setFont( med );
        ahrs.setPen( Qt::black );
        ahrs.drawText( c.dW + c.dW2 - (windRect.width() / 2), c.dH - m_pHeadIndicator->height() - 3, qsWind );
        ahrs.setPen( Qt::white );
        ahrs.drawText( c.dW + c.dW2 - (windRect.width() / 2) - 1, c.dH - m_pHeadIndicator->height() - 4, qsWind );

        // If long press triggered crosswind component display and the heading bug is set
        if( m_bShowCrosswind && (m_iHeadBugAngle >= 0) )
        {
            linePen.setWidth( 2 );
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
    ahrs.drawPixmap( c.dW + 10, 10, *m_pZoomInPixmap );
    ahrs.drawPixmap( (c.dW * 2.0) - 50, 10, *m_pZoomOutPixmap );

    // Draw the Altitude tape
    ahrs.fillRect( QRectF( c.dW - c.dW5 - 10.0, 0.0, c.dW5 - 30.0, c.dH ), QColor( 0, 0, 0, 100 ) );
    ahrs.setClipRect( c.dW - c.dW5 - 10.0, 2.0, c.dW5 + 10.0, c.dH - 5 );
    ahrs.drawPixmap( c.dW - c.dW5 - 10.0, c.dH2 - (static_cast<double>( c.iTinyFontHeight ) * 1.5) - (((20000.0 - g_situation.dBaroPressAlt) / 20000.0) * m_pAltTape->height()) + 7, *m_pAltTape );
    ahrs.setClipping( false );

    // Draw the vertical speed static pixmap
    ahrs.drawPixmap( c.dW - 40.0, 0.0, *m_pVertSpeedTape );

    // Draw the vertical speed indicator
    ahrs.translate( 0.0, c.dH2 - (dPxPerVSpeed * g_situation.dGPSVertSpeed / 100.0) );
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

    // Draw vertical speed indicator as In thousands and hundreds of FPM in tiny text on the vertical speed arrow
    ahrs.setFont( wee );
    QRect intRect( weeMetrics.boundingRect( qsIntVspeed ) );
    ahrs.drawText( c.dW - 20.0, 4.0, qsIntVspeed );
    ahrs.setFont( itsy );
    ahrs.drawText( c.dW - 20.0 + intRect.width() + 2, 4.0, qsFracVspeed );
    ahrs.resetTransform();

    // Draw the current altitude
    ahrs.setPen( Qt::white );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( c.dW - c.dW5 - 10.0, c.dH2 - (c.iTinyFontHeight / 2) - 2.0, c.dW5 - 28.0, c.iTinyFontHeight + 1 );
    ahrs.setPen( Qt::white );
    if( g_situation.dBaroPressAlt < 10000.0 )
        ahrs.setFont( small );
    else
    {
        QFont tinyBold( tiny );
        tinyBold.setBold( true );
        ahrs.setFont( tinyBold );   // 5 digits won't quite fit
    }
    ahrs.drawText( c.dW - c.dW5 - 8.0, c.dH2 + (c.iTinyFontHeight / 2) - 5.0, QString::number( static_cast<int>( g_situation.dBaroPressAlt ) ) );

    // Draw the Speed tape
    ahrs.fillRect( QRectF( 0.0, 0.0, c.dW5 - 25.0, c.dH ), QColor( 0, 0, 0, 100 ) );
    ahrs.drawPixmap( 4, c.dH2 - c.iSmallFontHeight - (((300.0 - g_situation.dGPSGroundSpeed) / 300.0) * m_pSpeedTape->height()), *m_pSpeedTape );

    // Draw the current speed
    ahrs.setPen( Qt::white );
    ahrs.setBrush( Qt::black );
    ahrs.drawRect( 0, c.dH2 - (c.iSmallFontHeight / 2), c.dW5 - 25, c.iSmallFontHeight + 1 );
    ahrs.setFont( small );
    ahrs.drawText( 5, c.dH2 + (c.iSmallFontHeight / 2) - 5.0, QString::number( static_cast<int>( g_situation.dGPSGroundSpeed ) ) );

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
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + 10.0, c.dH - c.iTinyFontHeight - 10, qsLat );
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + 10.0, c.dH - 10, qsLong );
    ahrs.setPen( Qt::white );
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + 9.0, c.dH - c.iTinyFontHeight - 11, qsLat );
    ahrs.drawText( (c.dW * 2.0) - c.dW5 + 9.0, c.dH - 11, qsLong );

    updateTraffic( &ahrs, &c );

    if( m_bShowGPSDetails )
    {
        QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, c.dH - 50.0 );

        cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
        cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

        linePen.setColor( Qt::black );
        linePen.setWidth( 3 );
        ahrs.setPen( linePen );
        ahrs.setBrush( cloudyGradient );
        ahrs.drawRect( 50, 50, width() - 100, c.dH - 100 );
        ahrs.setFont( med );
        ahrs.drawText( 100, 100, "GPS Status" );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 2),  QString( "GPS Satellites Seen: %1" ).arg( g_situation.iGPSSatsSeen ) );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 4),  QString( "GPS Satellites Tracked: %1" ).arg( g_situation.iGPSSatsTracked ) );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 6),  QString( "GPS Satellites Locked: %1" ).arg( g_situation.iGPSSats ) );
        ahrs.drawText( 100, 100 + (c.iMedFontHeight * 8),  QString( "GPS Fix Quality: %1" ).arg( g_situation.iGPSFixQuality ) );
        ahrs.setPen( Qt::blue );
        ahrs.drawText( c.dW + 100, 100, QString( "Version: %1" ).arg( g_qsRoscoPiVersion ) );
    }
}