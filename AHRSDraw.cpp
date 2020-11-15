/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QPainter>
#include <QtDebug>
#include <QFont>
#include <QLineF>
#include <QTransform>
#include <QVariant>
#include <QSettings>
#include <QBitmap>
#include <QPainterPath>


#include <math.h>

#include "AHRSDraw.h"
#include "StratuxStreams.h"
#include "TrafficMath.h"
#include "Builder.h"


extern QFont itsy;
extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;

extern StratuxSituation      g_situation;
extern QSettings            *g_pSet;
extern QList<StratuxTraffic> g_trafficList;
extern QString               g_qsStratofierVersion;


AHRSDraw::AHRSDraw( QPainter *pAHRS,
                    CanvasConstants *pC,
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
                    QPixmap *trafficOrange )
    : m_pAHRS( pAHRS ),
      m_pC( pC ),
      m_pCanvas( pCanvas ),
      m_pDirectAP( pDirectAP ),
      m_pFromAP( pFromAP ),
      m_pToAP( pToAP ),
      m_pAirports( pAirports ),
      m_pAirspaces( pAirspaces ),
      m_dZoomNM( dZoomNM ),
      m_pSettings( pSettings ),
      m_iMagDev( iMagDev ),
      m_trafficRed( trafficRed ),
      m_trafficYellow( trafficYellow ),
      m_trafficGreen( trafficGreen ),
      m_trafficCyan( trafficCyan ),
      m_trafficOrange( trafficOrange )
{
}


// Delete everything that needs deleting
AHRSDraw::~AHRSDraw()
{
}


void AHRSDraw::drawSlipSkid( double dSlipSkid )
{
    m_pAHRS->setPen( QPen( Qt::white, 2 ) );
    m_pAHRS->setBrush( Qt::black );
    m_pAHRS->drawRect( m_pC->dW2 - m_pC->dW4, 1, m_pC->dW2, m_pC->dH40 );
    m_pAHRS->drawRect( m_pC->dW2 - 15.0, 1.0, 30.0, m_pC->dH40 );
    m_pAHRS->setPen( Qt::NoPen );
    m_pAHRS->setBrush( Qt::green );
    m_pAHRS->drawEllipse( dSlipSkid - 7.0,
                          1.0,
                          20.0,
                          m_pC->dH40 );
}


void AHRSDraw::drawCurrAlt( QPixmap *pNum )
{
    if( m_pC->bPortrait )
        m_pAHRS->translate( 0.0, -m_pC->dH4 );

    m_pAHRS->setPen( QPen( Qt::white, m_pC->iThinPen ) );
    m_pAHRS->setBrush( QColor( 0, 0, 0, 175 ) );
    m_pAHRS->drawRect( m_pC->dW - m_pC->dW5 - m_pC->dW40, m_pC->dH2 - (m_pC->dHNum / 2.0) - (m_pC->dH * 0.0075), m_pC->dW5 + m_pC->dW40, m_pC->dHNum + (m_pC->dH * 0.015) );
    m_pAHRS->setPen( Qt::white );
    m_pAHRS->drawPixmap( m_pC->dW - m_pC->dW5, m_pC->dH2 - (m_pC->dHNum / 2.0), *pNum );

    if( m_pC->bPortrait )
        m_pAHRS->resetTransform();
}


void AHRSDraw::drawCurrSpeed( QPixmap *pNum, bool bGS )
{
    if( m_pC->bPortrait )
        m_pAHRS->translate( 0.0, -m_pC->dH4 );

    if( !bGS )
    {
        m_pAHRS->setPen( QPen( Qt::white, m_pC->iThinPen ) );
        m_pAHRS->setBrush( QColor( 0, 0, 0, 175 ) );
        m_pAHRS->drawRect( 0, m_pC->dH2 - (m_pC->dHNum / 2.0) - (m_pC->dH * 0.0075), m_pC->dW5 + m_pC->dW80, m_pC->dHNum + (m_pC->dH * 0.015) );
        m_pAHRS->drawPixmap( m_pC->dW * 0.0125, m_pC->dH2 - (m_pC->dHNum / 2.0), *pNum );
    }
    else
        m_pAHRS->drawPixmap( m_pC->dW10, m_pC->dH2 + m_pC->dH40, *pNum );

    if( m_pC->bPortrait )
        m_pAHRS->resetTransform();
}


void AHRSDraw::drawDirectOrFromTo()
{
    if( ((m_pDirectAP->qsID == "NULL") && (m_pFromAP->qsID == "NULL")) || (m_pAirports->count() == 0) )
        return;

    QPen coursePen( Qt::yellow, 8, Qt::SolidLine, Qt::RoundCap );

    maskHeading();

    if( m_pDirectAP->qsID != "NULL" )
    {
        QLineF  ball;
        int     iAP = TrafficMath::findAirport( m_pDirectAP, m_pAirports );

        if( iAP >= m_pAirports->count() )
            return;

        Airport ap = m_pAirports->at( iAP );

        if( m_pC->bPortrait )
        {
            ball.setP1( QPointF( m_pC->dW2, m_pC->dH - m_pC->dW2 - 10.0 ) );
            ball.setP2( ap.logicalPt );
        }
        else
        {
            ball.setP1( QPointF( m_pC->dW + m_pC->dW2, m_pC->dH - m_pC->dW2 - 10.0 ) );
            ball.setP2( ap.logicalPt );
        }
        if( ball.length() > (m_pC->dW2 - 30.0) )
            ball.setLength( m_pC->dW2 - 30.0 );

        m_pAHRS->setPen( coursePen );
        m_pAHRS->drawLine( ball );

        double  dDispBearing = ap.bd.dBearing;
        QPixmap num( 320, 84 );

        if( dDispBearing < 0.0 )
            dDispBearing += 360.0;

        Builder::buildNumber( &num, m_pC, dDispBearing, 0 );
        m_pAHRS->drawPixmap( m_pC->dW10 + m_pC->dW80, m_pC->dH80 + (m_pC->bPortrait ? 0.0 : m_pC->dH40), num );
        Builder::buildNumber( &num, m_pC, ap.bd.dDistance, 1 );
        m_pAHRS->drawPixmap( m_pC->dW10 + m_pC->dW80, m_pC->dH80 + m_pC->dH20 + (m_pC->bPortrait ? 0.0 : m_pC->dH40), num );
    }
    else if( m_pFromAP->qsID != "NULL" )
    {
        int iFromAP = TrafficMath::findAirport( m_pFromAP, m_pAirports );
        int iToAP = TrafficMath::findAirport( m_pToAP, m_pAirports );

        if( (iFromAP >= m_pAirports->count()) || (iToAP >= m_pAirports->count() ) )
            return;

        Airport apFrom = m_pAirports->at( iFromAP );
        Airport apTo = m_pAirports->at( iToAP );

        m_pAHRS->setPen( coursePen );
        m_pAHRS->drawLine( apFrom.logicalPt, apTo.logicalPt );

        double dDispBearing = apTo.bd.dBearing;
        QPixmap num( 320, 84 );

        if( dDispBearing < 0.0 )
            dDispBearing += 360.0;

        Builder::buildNumber( &num, m_pC, dDispBearing, 0 );
        m_pAHRS->drawPixmap( m_pC->dW10 + m_pC->dW80, m_pC->dH80 + (m_pC->bPortrait ? 0.0 : m_pC->dH40), num );
        Builder::buildNumber( &num, m_pC, apTo.bd.dDistance, 1 );
        m_pAHRS->drawPixmap( m_pC->dW10 + m_pC->dW80, m_pC->dH80 + m_pC->dH20 + (m_pC->bPortrait ? 0.0 : m_pC->dH40), num );
    }

    m_pAHRS->setClipping( false );
}


void AHRSDraw::updateAirports()
{
    Airport      ap;
    QLineF       ball;
    double	     dPxPerNM = static_cast<double>( m_pC->dW - 30.0 ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
    double       dAPDist;
    QPen         apPen( Qt::magenta );
    QLineF       runwayLine;
    int          iRunway, iAPRunway;
    double       dAirportDiam = m_pC->dWa * (m_pC->bPortrait ? 0.03125 : 0.01875);
    QRect        apRect;
    double       deltaY;
    double       dHead = g_situation.dAHRSGyroHeading;
    int          iAP = 0;

    if( g_situation.bHaveWTData )
        dHead = g_situation.dAHRSMagHeading;

    QFontMetrics apMetrics( tiny );

    m_pAHRS->setFont( tiny );

    maskHeading();

    apPen.setWidth( m_pC->iThinPen );
    m_pAHRS->setBrush( Qt::NoBrush );

    foreach( ap, *m_pAirports )
    {
        if( ap.bGrass && (m_pSettings->eShowAirports == Canvas::ShowPavedAirports) )
            continue;
        else if( (!ap.bGrass) && (m_pSettings->eShowAirports == Canvas::ShowGrassAirports) )
            continue;
        else if( (ap.qsID == "R") && (!m_pSettings->bShowPrivate) )
            continue;

        apRect = apMetrics.boundingRect( ap.qsID );

        dAPDist = ap.bd.dDistance * dPxPerNM;

        ball.setP1( QPointF( (m_pC->bPortrait ? 0.0 : m_pC->dW) + m_pC->dW2, m_pC->dH - m_pC->dHeadDiam2 - 10.0 ) );
        ball.setP2( QPointF( (m_pC->bPortrait ? 0.0 : m_pC->dW) + m_pC->dW2, m_pC->dH - m_pC->dHeadDiam2 - 10.0 + dAPDist ) );

        // Airport angle in reference to you (which clock position it's at)
        ball.setAngle( ap.bd.dBearing - dHead - 90.0 );
        // Qt Y coords are backward
        deltaY = ball.p2().y() - (m_pC->dH - m_pC->dHeadDiam2 - 10.0);
        ball.setP2( QPointF( ball.p2().x(), m_pC->dH - m_pC->dHeadDiam2 - 10.0 - deltaY ) );

        apPen.setWidth( m_pC->iThinPen );
        apPen.setColor( Qt::black );
        m_pAHRS->setPen( apPen );
        ap.logicalPt.setX( ball.p2().x() );
        ap.logicalPt.setY( ball.p2().y() );
        m_pAHRS->drawEllipse( ap.logicalPt.x() - (dAirportDiam / 2.0) + 1.0, ap.logicalPt.y()- (dAirportDiam / 2.0) + 1.0, dAirportDiam, dAirportDiam );
        apPen.setColor( Qt::magenta );
        m_pAHRS->setPen( apPen );
        m_pAHRS->drawEllipse( ap.logicalPt.x() - (dAirportDiam / 2.0), ap.logicalPt.y() - (dAirportDiam / 2.0), dAirportDiam, dAirportDiam );
        apPen.setColor( Qt::black );
        m_pAHRS->setPen( apPen );
        m_pAHRS->drawText( ball.p2().x() - (dAirportDiam / 2.0) - (apRect.width() / 2) + 2, ball.p2().y() - (dAirportDiam / 2.0) + apRect.height() - 1, ap.qsID );
        // Draw the runways and tiny headings after the black ID shadow but before the yellow ID text
        if( (m_dZoomNM <= 30) && m_pSettings->bShowRunways )
        {
            for( iRunway = 0; iRunway < ap.runways.count(); iRunway++ )
            {
                iAPRunway = ap.runways.at( iRunway );
                runwayLine.setP1( QPointF( ball.p2().x(), ball.p2().y() ) );
                runwayLine.setP2( QPointF( ball.p2().x(), ball.p2().y() + (dAirportDiam * 2.0) ) );
                runwayLine.setAngle( 270.0 - static_cast<double>( iAPRunway ) );
                apPen.setColor( Qt::magenta );
                apPen.setWidth( m_pC->iThickPen );
                m_pAHRS->setPen( apPen );
                m_pAHRS->drawLine( runwayLine );
                if( ((iAPRunway - dHead) > 90) && ((iAPRunway - dHead) < 270) )
                    runwayLine.setLength( runwayLine.length() + m_pC->dW80 );
                QPixmap num( 128, 84 ); // It would need to be resized back anyway so creating a new one each time is faster
                num.fill( Qt::transparent );
                Builder::buildNumber( &num, m_pC, iAPRunway / 10, 2 );
                num = num.scaledToWidth( m_pC->dW20, Qt::SmoothTransformation );
                m_pAHRS->drawPixmap( runwayLine.p2(), num );
            }
        }
        apPen.setColor( Qt::yellow );
        m_pAHRS->setPen( apPen );
        m_pAHRS->drawText( ball.p2().x() - (dAirportDiam / 2.0) - (apRect.width() / 2) + 1, ball.p2().y() - (dAirportDiam / 2.0) + apRect.height() - 2, ap.qsID );

        m_pAirports->replace( iAP, ap );    // Update airports with logical coords for painting DirectTo and FromTo
        iAP++;
    }

    m_pAHRS->setClipping( false );
}


void AHRSDraw::updateAirspaces()
{
    if( !m_pSettings->bShowAirspaces )
        return;

    Airspace     as;
    QLineF       ball;
    double	     dPxPerNM = static_cast<double>( m_pC->dHeadDiam ) / (m_dZoomNM * 2.0);	// Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
    double       dASDist;
    QPen         asPen( Qt::yellow );
    BearingDist  bd;
    QPolygonF    airspacePoly;
    double       deltaY;

    maskHeading();

    asPen.setWidth( m_pC->iThinPen );

    foreach( as, *m_pAirspaces )
    {
        airspacePoly.clear();
        foreach( bd, as.shapeHav )
        {
            dASDist = bd.dDistance * dPxPerNM;

            ball.setP1( QPointF( (m_pC->bPortrait ? 0.0 : m_pC->dW) + m_pC->dW2, m_pC->dH - 10.0 - m_pC->dHeadDiam2 ) );
            ball.setP2( QPointF( (m_pC->bPortrait ? 0.0 : m_pC->dW) + m_pC->dW2, m_pC->dH - 10.0 - m_pC->dHeadDiam2 - dASDist ) );

            // Airspace polygon point angle in reference to you (which clock position it's at)
            if( g_situation.bHaveWTData )
                ball.setAngle( bd.dBearing - g_situation.dAHRSMagHeading - 90.0 );
            else
                ball.setAngle( bd.dBearing - g_situation.dAHRSGyroHeading - 90.0 );
            // Qt Y coords are backward
            deltaY = ball.p2().y() - (m_pC->dH - 10.0 - m_pC->dHeadDiam2);
            ball.setP2( QPointF( ball.p2().x(), m_pC->dH - 10.0 - m_pC->dHeadDiam2 - deltaY ) );
            airspacePoly.append( ball.p2() );
        }
        m_pAHRS->setBrush( Qt::NoBrush );
        switch( as.eType )
        {
            case Canvas::Airspace_Class_B:
                asPen.setColor( Qt::blue );
                break;
            case Canvas::Airspace_Class_C:
                asPen.setColor( Qt::darkBlue );
                break;
            case Canvas::Airspace_Class_D:
                asPen.setColor( Qt::green );
                break;
            case Canvas::Airspace_Class_E:
                asPen.setColor( Qt::darkGreen );
                break;
            case Canvas::Airspace_Class_G:
                asPen.setColor( Qt::gray );
                break;
            case Canvas::Airspace_MOA:
                asPen.setColor( Qt::darkMagenta );
                break;
            case Canvas::Airspace_TFR:
                asPen.setColor( Qt::darkYellow );
                m_pAHRS->setBrush( QColor( 255, 255, 0, 50 ) );
                break;
            case Canvas::Airspace_SFRA:
                asPen.setColor( Qt::red );
                m_pAHRS->setBrush( QColor( 255, 0, 0, 50 ) );
                break;
            case Canvas::Airspace_Prohibited:
                asPen.setColor( Qt::darkCyan );
                m_pAHRS->setBrush( QColor( 0, 255, 255, 50 ) );
                break;
            case Canvas::Airspace_Restricted:
                asPen.setColor( QColor( 0xFF, 0xA5, 0x00 ) );
                m_pAHRS->setBrush( QColor( 0xFF, 0xA5, 0x00, 50 ) );
                break;
            default:
                asPen.setColor( Qt::transparent );
                break;
        }
        m_pAHRS->setPen( asPen );
        m_pAHRS->drawPolygon( airspacePoly );
        if( (as.iAltTop > 0) && m_pSettings->bShowAltitudes )
        {
            QRectF asBound = airspacePoly.boundingRect();
            QLineF asLine( asBound.topRight(), asBound.bottomLeft() );
            while( !airspacePoly.containsPoint( asLine.p2(), Qt::OddEvenFill ) )
                asLine.setLength( asLine.length() - 1.0 );
            asLine.setLength( asLine.length() - 2.0 );
            m_pAHRS->setPen( Qt::darkGray );
            m_pAHRS->setFont( itsy );
            m_pAHRS->drawText( QPointF( asLine.p2().x(), asLine.p2().y() - m_pC->iTinyFontHeight ), QString::number( as.iAltTop / 100 ) );
            if( as.iAltBottom <= 0 )
                m_pAHRS->drawText( asLine.p2(), "GND" );
            else
                m_pAHRS->drawText( asLine.p2(), QString::number( as.iAltBottom / 100 ) );
        }
    }
    m_pAHRS->setClipping( false );
}


// Draw the traffic onto the heading indicator and the tail numbers on the side
void AHRSDraw::updateTraffic()
{
    StratuxTraffic traffic;
    QPen           stickPen( Qt::green, 2 );
    double		   dPxPerNM = m_pC->dHeadDiam / (m_dZoomNM * 2.0);     // Pixels per nautical mile; the outer limit of the heading indicator is calibrated to the zoom level in NM
    QLineF		   ball, info, stick;
    double         dAlt;
    QString        qsSign;
    QFontMetrics   smallMetrics( small );
    QColor         closenessColor( Qt::green );
    QRectF         trafficRect( 0.0, 0.0, m_pC->dW20, m_pC->dW20 );
    QPointF        unBall;
    double         dHead = g_situation.dAHRSGyroHeading;

    if( g_situation.bHaveWTData )
        dHead = g_situation.dAHRSMagHeading;

    maskHeading();

    // Draw a chevron for each aircraft; the outer edge of the heading indicator is calibrated to be 20 NM out from your position
    foreach( traffic, g_trafficList )
    {
        // If bearing and distance were able to be calculated then show relative position
        if( traffic.bHasADSB && (traffic.qsTail != m_pSettings->qsOwnshipID) )
        {
            double dTrafficDist = traffic.dDist * dPxPerNM;
            double dAltDist = traffic.dAlt - g_situation.dBaroPressAlt;
            double dAltDistAbs = fabs( dAltDist );

            if( m_pSettings->bShowAllTraffic || (dAltDistAbs < 5000) )
            {
                closenessColor = Qt::green;

                ball.setP1( QPointF( (m_pC->bPortrait ? 0 : m_pC->dW) + m_pC->dW2, m_pC->dH - 10.0 - m_pC->dHeadDiam2 ) );
                ball.setP2( QPointF( (m_pC->bPortrait ? 0 : m_pC->dW) + m_pC->dW2, m_pC->dH - 10.0 - m_pC->dHeadDiam2 - dTrafficDist ) );

                // Traffic angle in reference to you (which clock position they're at regardless of their own course)
                ball.setAngle( -(traffic.dBearing - dHead - 90.0) );

                // Draw the arrow
                trafficRect.moveCenter( ball.p2() );
                m_pAHRS->translate( ball.p2() );
                m_pAHRS->rotate( traffic.dTrack - 90.0 + g_situation.dAHRSMagHeading - static_cast<double>( m_iMagDev * 2.0 ) );
                unBall.setX( -ball.p2().x() );
                unBall.setY( -ball.p2().y() );
                m_pAHRS->translate( unBall );

                if( traffic.bOnGround )
                {
                    m_pAHRS->drawPixmap( trafficRect.toRect(), *m_trafficCyan );
                    stickPen.setColor( Qt::cyan );
                    closenessColor = Qt::cyan;
                }
                else if( dAltDistAbs > 2000 )
                {
                    m_pAHRS->drawPixmap( trafficRect.toRect(), *m_trafficGreen );
                    stickPen.setColor( Qt::green );
                    closenessColor = Qt::green;
                }
                else if( (dAltDistAbs <= 2000) && (dAltDistAbs > 1000) )
                {
                    m_pAHRS->drawPixmap( trafficRect.toRect(), *m_trafficYellow );
                    stickPen.setColor( Qt::yellow );
                    closenessColor = Qt::yellow;
                }
                else if( (dAltDistAbs <= 1000) && (dAltDistAbs > 500) )
                {
                    m_pAHRS->drawPixmap( trafficRect.toRect(), *m_trafficOrange );
                    stickPen.setColor( QColor( 0xFF, 0xA5, 0x00 ) );
                    closenessColor = QColor( 0xFF, 0xA5, 0x00 );
                }
                else
                {
                    m_pAHRS->drawPixmap( trafficRect.toRect(), *m_trafficRed );
                    stickPen.setColor( Qt::red );
                    closenessColor = Qt::red;
                }
                // Create the stick
                stick.setP1( ball.p2() );
                stick.setP2( QPointF( ball.p2().x(), ball.p2().y() - m_pC->dW20 ) );
                m_pAHRS->setPen( stickPen );
                m_pAHRS->drawLine( stick );
                m_pAHRS->resetTransform();

                // Draw the ID, numerical track heading and altitude delta
                dAlt = (traffic.dAlt - g_situation.dBaroPressAlt) / 100.0;
                if( dAlt > 0 )
                    qsSign = "+";
                else if( dAlt < 0 )
                    qsSign = "-";
                m_pAHRS->setPen( Qt::black );
                m_pAHRS->setFont( wee );
                info.setP1( ball.p2() );
                info.setP2( QPointF( ball.p2().x() + m_pC->dW40, ball.p2().y() ) );
                info.setAngle( 30.0 );
                m_pAHRS->drawText( info.p2(), traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
                info.setAngle( -50.0 );
                m_pAHRS->drawText( info.p2(), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
                m_pAHRS->setPen( closenessColor );
                info.translate( 2.0, 2.0 );
                info.setAngle( 30.0 );
                m_pAHRS->drawText( info.p2(), traffic.qsTail.isEmpty() ? "UNKWN" : traffic.qsTail );
                info.setAngle( -50.0 );
                m_pAHRS->drawText( info.p2(), QString( "%1%2" ).arg( qsSign ).arg( static_cast<int>( fabs( dAlt ) ) ) );
            }
        }
    }

    m_pAHRS->setClipping( false );

    QString qsZoom = QString( "%1nm" ).arg( static_cast<int>( m_dZoomNM ) );
    QString qsMagDev = QString( "%1%2" ).arg( m_iMagDev ).arg( QChar( 0xB0 ) );

    if( m_iMagDev > 0 )
        qsMagDev.prepend( "+" );

    // Draw the zoom level
    m_pAHRS->setFont( tiny );
    if( m_pC->bPortrait )
    {
        m_pAHRS->setPen( Qt::black );
        m_pAHRS->drawText( m_pC->dW - m_pC->dW10, m_pC->dH - m_pC->dH20 - (m_pC->iTinyFontHeight * 2), qsZoom );
        m_pAHRS->setPen( QColor( 80, 255, 80 ) );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW10 + 2.0, m_pC->dH - m_pC->dH20 - (m_pC->iTinyFontHeight * 2) + 2.0, qsZoom );

        // Draw the magnetic deviation
        m_pAHRS->setPen( Qt::black );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW10 - m_pC->dW40 - m_pC->dW80, m_pC->dH - m_pC->dH20 - m_pC->iTinyFontHeight, qsMagDev );
        m_pAHRS->setPen( Qt::yellow );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW10 - m_pC->dW40 - m_pC->dW80 + 2.0, m_pC->dH - m_pC->dH20 - m_pC->iTinyFontHeight + 2.0, qsMagDev );
    }
    else
    {
        m_pAHRS->setPen( Qt::black );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW5, m_pC->iTinyFontHeight, qsZoom );
        m_pAHRS->setPen( QColor( 80, 255, 80 ) );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW5 + 2.0, m_pC->iTinyFontHeight + 2.0, qsZoom );

        // Draw the magnetic deviation
        m_pAHRS->setPen( Qt::black );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW5, (m_pC->iTinyFontHeight * 2.0), qsMagDev );
        m_pAHRS->setPen( Qt::yellow );
        m_pAHRS->drawText( m_pC->dWa - m_pC->dW5 + 2.0, (m_pC->iTinyFontHeight * 2.0) + 2.0, qsMagDev );
    }
}


void AHRSDraw::paintTemp()
{
    // Draw the outside temp if we have it
    if( g_situation.bHaveWTData )
    {
        m_pAHRS->setPen( Qt::yellow );
        m_pAHRS->setFont( small );
        m_pAHRS->drawText( m_pC->dW - m_pC->dW5 - m_pC->dW5, m_pC->dH20 + m_pC->dH80,
                           QString( "%1%2" ).arg( g_situation.dBaroTemp, 0, 'f', 1 ).arg( QChar( 0xB0 ) ) );
    }
}


void AHRSDraw::paintSwitchNotice( FuelTanks *pTanks )
{
    QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, m_pC->dH - 50.0 );
    QPen            linePen( Qt::black );

    cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
    cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

    linePen.setWidth( m_pC->iThickPen );
    m_pAHRS->setPen( linePen );
    m_pAHRS->setBrush( cloudyGradient );

    m_pAHRS->drawRect( m_pC->dW10, m_pC->dH10, (m_pC->bPortrait ? m_pC->dW : m_pC->dWa) - m_pC->dW5, m_pC->dH - m_pC->dH5 );
    m_pAHRS->setFont( large );
    m_pAHRS->drawText( m_pC->dW5, m_pC->dH5 + m_pC->iLargeFontHeight, "TANK SWITCH" );
    m_pAHRS->setFont( med );
    m_pAHRS->drawText( m_pC->dW5, m_pC->dH5 + (m_pC->iMedFontHeight * 3),  QString( "Switch to  %1  tank" ).arg( pTanks->bOnLeftTank ? "RIGHT" : "LEFT" ) );
    m_pAHRS->drawText( m_pC->dW5, m_pC->dH5 + (m_pC->iMedFontHeight * 5),  QString( "ADJUST MIXTURE" ) );
    m_pAHRS->drawText( m_pC->dW5, m_pC->dH5 + (m_pC->iMedFontHeight * 7),  QString( "PRESS TO CONFIRM" ) );
}


void AHRSDraw::paintTimer( int iTimerMin, int iTimerSec )
{
    QString      qsTimer = QString( "%1:%2" ).arg( iTimerMin, 2, 10, QChar( '0' ) ).arg( iTimerSec, 2, 10, QChar( '0' ) );
    QFontMetrics largeMetrics( large );
    QPen         linePen( Qt::white );
    QPixmap      Num( m_pC->dW5, m_pC->dH20 );
    QPixmap      timerNum( m_pC->dW5, m_pC->dH20 );

    linePen.setWidth( m_pC->iThinPen );

    m_pAHRS->setPen( linePen );
    m_pAHRS->setBrush( Qt::black );

    Builder::buildNumber( &Num, m_pC, qsTimer );
    timerNum.fill( Qt::cyan );
    timerNum.setMask( Num.createMaskFromColor( Qt::transparent ) );

    if( m_pC->bPortrait )
    {
        m_pAHRS->drawRect( m_pC->dW2 - m_pC->dW10 - m_pC->dW40, m_pC->dH - m_pC->dH20, m_pC->dW5 + m_pC->dW20, m_pC->dH20 );
        m_pAHRS->drawPixmap( m_pC->dW2 - m_pC->dW10, m_pC->dH - m_pC->dH20 + m_pC->dH100, timerNum );
    }
    else
    {
        m_pAHRS->drawRect( m_pC->dW2 - m_pC->dW5, m_pC->dH - m_pC->dH5, m_pC->dW5 + m_pC->dW10, m_pC->dH20 );
        m_pAHRS->drawPixmap( m_pC->dW2 - m_pC->dW10 + m_pC->dW20 - (timerNum.width() / 2), m_pC->dH - m_pC->dH5 + m_pC->dH100, timerNum );
    }
}


void AHRSDraw::paintInfo()
{
    QLinearGradient cloudyGradient( 0.0, 50.0, 0.0, m_pC->dH - 50.0 );
    QFont           med_bu( med );
    QPen            linePen( Qt::black );
    int             iMedFontHeight = m_pC->iMedFontHeight;
    int             iSmallFontHeight = m_pC->iSmallFontHeight;

    med_bu.setUnderline( true );
    med_bu.setBold( true );

    cloudyGradient.setColorAt( 0, QColor( 255, 255, 255, 225 ) );
    cloudyGradient.setColorAt( 1, QColor( 175, 175, 255, 225 ) );

    linePen.setWidth( m_pC->iThickPen );
    m_pAHRS->setPen( linePen );
    m_pAHRS->setBrush( cloudyGradient );
    m_pAHRS->drawRect( 50, 50, (m_pC->bPortrait ? m_pC->dW : m_pC->dWa) - 100, m_pC->dH - 100 );
    m_pAHRS->setFont( med_bu );

    m_pAHRS->drawText( 75, 95, "GPS Status" );
    m_pAHRS->setFont( small );
    m_pAHRS->drawText( 75, 95 + iMedFontHeight,  QString( "GPS Satellites Seen: %1" ).arg( g_situation.iGPSSatsSeen ) );
    m_pAHRS->drawText( 75, 95 + (iMedFontHeight * 2),  QString( "GPS Satellites Tracked: %1" ).arg( g_situation.iGPSSatsTracked ) );
    m_pAHRS->drawText( 75, 95 + (iMedFontHeight * 3),  QString( "GPS Satellites Locked: %1" ).arg( g_situation.iGPSSats ) );
    m_pAHRS->drawText( 75, 95 + (iMedFontHeight * 4),  QString( "GPS Fix Quality: %1" ).arg( g_situation.iGPSFixQuality ) );

    StratuxTraffic traffic;
    int            iY = 0;
    int            iLine;

    m_pAHRS->setFont( med_bu );
    m_pAHRS->drawText( m_pC->bPortrait ? 75 : m_pC->dW, m_pC->bPortrait ? m_pC->dH2 : 95, "Non-ADS-B Traffic" );
    m_pAHRS->setFont( small );
    foreach( traffic, g_trafficList )
    {
        // If bearing and distance were able to be calculated then show relative position
        if( !traffic.bHasADSB && (!traffic.qsTail.isEmpty()) )
        {
            iLine = (m_pC->bPortrait ? static_cast<int>( m_pC->dH2 ) : 95) + iMedFontHeight + (iY * iSmallFontHeight);
            m_pAHRS->drawText( m_pC->bPortrait ? 75 : m_pC->dW, iLine, traffic.qsTail );
            m_pAHRS->drawText( m_pC->bPortrait ? m_pCanvas->scaledH( 200.0 ) : m_pCanvas->scaledH( 500.0 ), iLine, QString( "%1 ft" ).arg( static_cast<int>( traffic.dAlt ) ) );
            if( traffic.iSquawk > 0 )
                m_pAHRS->drawText( m_pC->bPortrait ? m_pCanvas->scaledH( 325 ) : m_pCanvas->scaledH( 600 ), iLine, QString::number( traffic.iSquawk ) );
            iY++;
        }
        if( iY > 10 )
            break;
    }

    m_pAHRS->setFont( med );
    m_pAHRS->setPen( Qt::blue );
    m_pAHRS->drawText( 75, m_pC->bPortrait ? m_pCanvas->scaledV( 700.0 ) : m_pCanvas->scaledV( 390.0 ), QString( "Version: %1" ).arg( g_qsStratofierVersion ) );
    if( g_situation.bHaveWTData )
        m_pAHRS->drawText( 75, m_pC->bPortrait ? m_pCanvas->scaledV( 750.0 ) : m_pCanvas->scaledV( 440.0 ), QString( "BADASP: %1" ).arg( g_situation.qsBADASPversion ) );
}


void AHRSDraw::maskHeading()
{
    QPainterPath maskPath;

    maskPath.addEllipse( QPointF( (m_pC->bPortrait ? 0 : m_pC->dW) + m_pC->dW2, m_pC->dH - 10.0 - m_pC->dHeadDiam2 ),
                         m_pC->dHeadDiam2, m_pC->dHeadDiam2 );
    m_pAHRS->setClipPath( maskPath );
}

