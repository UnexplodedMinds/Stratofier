/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QPixmap>
#include <QPainter>

#include "Builder.h"
#include "Canvas.h"


extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;


Builder::Builder()
{
}


// Build the altitude tape pixmap
// Note that there are pixmap size limits on Android that appear to be smaller than Windows and X11
// so there are font size limitations in order not to exceed them.
void Builder::buildAltTape( QPixmap *pAltTape, Canvas *pCanvas )
{
    QPainter        ahrs( pAltTape );
    int             iAlt, iV = 1, iY;
    CanvasConstants c = pCanvas->constants();
    QString         qsAlt;

    ahrs.setFont( tiny );
	for( iAlt = 20000; iAlt >= 0; iAlt -= 100 )
    {
        iY = static_cast<int>( static_cast<double>( iV ) * static_cast<double>( c.iTinyFontHeight ) * 1.5 );
        qsAlt = QString::number( iAlt );
        ahrs.setPen( Qt::black );
        ahrs.drawText( 1, iY + 1, qsAlt );
        ahrs.setPen( Qt::white );
        ahrs.drawText( 0, iY, qsAlt );
        iY = iY - (c.iTinyFontHeight / 2) + 2;
        iV++;
    }
}


// Build the vertical speed tape pixmap
void Builder::buildVertSpeedTape( QPixmap *pVertTape, Canvas *pCanvas, bool bPortrait )
{
    QPainter        ahrs( pVertTape );
    int             iVert, iV = 1, iY;
    CanvasConstants c = pCanvas->constants();
    double          dLineHeight;
    QString         qsVSpeed;

    if( bPortrait )
        dLineHeight = (c.dH2 - 30.0) / 40.0;
    else
        dLineHeight = (c.dH - 30.0) / 40.0;

    ahrs.setFont( tiny );
	for( iVert = 10; iVert >= -10; iVert-- )
    {
        iY = static_cast<int>( static_cast<double>( iV ) * dLineHeight * 2.0 );
        if( (iVert % 2) == 0 )
        {
            qsVSpeed = QString::number( abs( iVert * 2 ) );
            ahrs.setPen( Qt::black );
            ahrs.drawText( 6, iY + 1, qsVSpeed );
            ahrs.setPen( Qt::white );
            ahrs.drawText( 5, iY, qsVSpeed );
        }
        iY -= 3;
		ahrs.drawLine( pVertTape->width() - 10, iY - 1, pVertTape->width() - 5, iY - 1 );
        iV++;
    }
}


// Build the air speed (ground speed actually since this is GPS based without any pitot-static system)
void Builder::buildSpeedTape( QPixmap *pSpeedTape, Canvas *pCanvas )
{
    QPainter        ahrs( pSpeedTape );
    int             iSpeed, iV = 1, iY;
    CanvasConstants c = pCanvas->constants();
    QString         qsSpeed;
    double          dDoubleTinyFontHeight = static_cast<double>( c.iTinyFontHeight ) * 2.0;

	for( iSpeed = 300; iSpeed >= 0; iSpeed -= 10 )
    {
		ahrs.setFont( tiny );
        iY = static_cast<int>( static_cast<double>( iV ) * dDoubleTinyFontHeight );
        qsSpeed = QString::number( iSpeed );
        ahrs.setPen( Qt::black );
        ahrs.drawText( 11, iY + 1, qsSpeed );
        ahrs.setPen( Qt::white );
		ahrs.drawText( 10, iY, qsSpeed );
#if defined( Q_OS_ANDROID )
        iY = iY - (c.iTinyFontHeight / 4);
#else
        iY = iY - (c.iTinyFontHeight / 2) + 5;
#endif
        ahrs.setPen( QPen( Qt::white, 2 ) );
        ahrs.drawLine( 0, iY, 5, iY );
		if( iSpeed >= 10 )
		{
			ahrs.setFont( wee );
            iY = static_cast<int>( (static_cast<double>( iV ) * dDoubleTinyFontHeight) + c.iTinyFontHeight );
            qsSpeed = QString::number( iSpeed - 5 );
            ahrs.setPen( Qt::black );
            ahrs.drawText( 11, iY + 1, qsSpeed );
            ahrs.setPen( Qt::white );
            ahrs.drawText( 10, iY, qsSpeed );
        }
		iV++;
    }
}


// Build the roll indicator (the arc scale at the top)
void Builder::buildRollIndicator( QPixmap *pRollInd, Canvas *pCanvas, bool bPortrait )
{
    QPainter        ahrs( pRollInd );
    double          dW = pRollInd->width();
    double          dH = pRollInd->height();
    double          dW2 = dW / 2.0;
    double          dH2 = dH / 2.0;
    QPen            linePen( Qt::white, 3 );
    QFontMetrics    rollMetrics( bPortrait ? tiny : wee );
    QString         qsRoll;
    QRect           rollRect;
    QLineF          clipLine( dW / 2.0, dH / 2.0, dW / 2.0, 20.0 );
    QFont           tinyBold( bPortrait ? tiny : wee );
    CanvasConstants c = pCanvas->constants();

    tinyBold.setBold( true );

    clipLine.setAngle( 29.0 );
    ahrs.setClipRect( 0.0, 0.0, dW, clipLine.p2().y() );

    // Draw the black base arc surround but make it white as well in Android; the higher resolution makes it look "funny"
#if defined( Q_OS_ANDROID )
    linePen.setColor( Qt::white );
#else
    linePen.setColor( Qt::black );
#endif
    linePen.setWidth( c.iFatPen );
    ahrs.setPen( linePen );
    ahrs.drawEllipse( 21.0, 21.0, dW - 42.0, dH - 42.0 );
    linePen.setColor( Qt::white );
    linePen.setWidth( c.iThickPen );
    ahrs.setPen( linePen );

    ahrs.setFont( tinyBold );
    ahrs.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing, true );

    // -30 to 30 with black lines on -30, 0 and 30
    for( int i = 0; i < 7; i++ )
    {
        ahrs.translate( dW2, dH2 );
        ahrs.rotate( -30 + (i * 10) );
        ahrs.translate( -dW2, -dH2 );
        if( (i == 0) || (i == 3) || (i == 6) )
            linePen.setColor( QColor( 0xFF, 0xA5, 0x00, 0xFF ) );   // Orange
        else
            linePen.setColor( Qt::white );
        ahrs.setPen( linePen );
        ahrs.drawLine( dW2, (c.dH * (c.bPortrait ? 0.0125 : 0.02083)), dW2, (c.dH * (c.bPortrait ? 0.025 : 0.04167)) );
        qsRoll = QString::number( abs( -30 + (i * 10) ) );
        qsRoll.chop( 1 );
        rollRect = rollMetrics.boundingRect( qsRoll );
#if defined( Q_OS_ANDROID )
        ahrs.drawText( dW2 - (static_cast<double>( rollRect.width() ) * 1.5), (c.dH * (c.bPortrait ? 0.04 : 0.06)) + rollRect.height(), qsRoll );
#else
        ahrs.drawText( dW2 - (rollRect.width() / 2.0), 22.0 + rollRect.height(), qsRoll );
#endif
        ahrs.resetTransform();
    }
    // Red lines on -60, -45, 45 and 60
    linePen.setColor( Qt::darkRed );
    ahrs.setPen( linePen );
    // -45
    qsRoll = "45";
    ahrs.translate( dW2, dH2 );
    ahrs.rotate( -45 );
    ahrs.translate( -dW2, -dH2 );
    ahrs.drawLine( dW2, (c.dH * (c.bPortrait ? 0.0125 : 0.02083)), dW2, (c.dH * (c.bPortrait ? 0.025 : 0.04167)) );
    rollRect = rollMetrics.boundingRect( qsRoll );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( dW2 - (static_cast<double>( rollRect.width() ) * 1.5), (c.dH * (c.bPortrait ? 0.04 : 0.06)) + rollRect.height(), qsRoll );
#else
    ahrs.drawText( dW2 - (rollRect.width() / 2.0), 22.0 + rollRect.height(), qsRoll );
#endif
    ahrs.resetTransform();
    // 45
    ahrs.translate( dW2, dH2 );
    ahrs.rotate( 45 );
    ahrs.translate( -dW2, -dH2 );
    ahrs.drawLine( dW2, (c.dH * (c.bPortrait ? 0.0125 : 0.02083)), dW2, (c.dH * (c.bPortrait ? 0.025 : 0.04167)) );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( dW2 - (static_cast<double>( rollRect.width() ) * 1.5), (c.dH * (c.bPortrait ? 0.04 : 0.06)) + rollRect.height(), qsRoll );
#else
    ahrs.drawText( dW2 - (rollRect.width() / 2.0), 22.0 + rollRect.height(), qsRoll );
#endif
    ahrs.resetTransform();

    // We need to turn it off so the 60s won't get clipped
    ahrs.setClipping( false );

    // -60
    ahrs.translate( dW2, dH2 );
    ahrs.rotate( -60 );
    ahrs.translate( -dW2, -dH2 );
    ahrs.drawLine( dW2, (c.dH * (c.bPortrait ? 0.0125 : 0.02083)), dW2, (c.dH * (c.bPortrait ? 0.025 : 0.04167)) );
    qsRoll = "60";
    rollRect = rollMetrics.boundingRect( qsRoll );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( dW2 - (static_cast<double>( rollRect.width() ) * 1.5), (c.dH * (c.bPortrait ? 0.04 : 0.06)) + rollRect.height(), qsRoll );
#else
    ahrs.drawText( dW2 - (rollRect.width() / 2.0), 22.0 + rollRect.height(), qsRoll );
#endif
    ahrs.resetTransform();
    // 60
    ahrs.translate( dW2, dH2 );
    ahrs.rotate( 60 );
    ahrs.translate( -dW2, -dH2 );
    ahrs.drawLine( dW2, (c.dH * (c.bPortrait ? 0.0125 : 0.02083)), dW2, (c.dH * (c.bPortrait ? 0.025 : 0.04167)) );
#if defined( Q_OS_ANDROID )
    ahrs.drawText( dW2 - (static_cast<double>( rollRect.width() ) * 1.5), (c.dH * (c.bPortrait ? 0.04 : 0.06)) + rollRect.height(), qsRoll );
#else
    ahrs.drawText( dW2 - (rollRect.width() / 2.0), 22.0 + rollRect.height(), qsRoll );
#endif
    ahrs.resetTransform();

    // Turn the clipping rect back on
    ahrs.setClipRect( 0.0, 0.0, dW, clipLine.p2().y() );

    // Draw the thinner white base arc overtop of the pegs and black shadow
    linePen.setColor( Qt::white );
    linePen.setWidth( 3 );
    ahrs.setPen( linePen );
    ahrs.drawEllipse( 21.0, 21.0, dW - 42.0, dH - 42.0 );
}


// Build the round heading indicator
void Builder::buildHeadingIndicator( QPixmap *pHeadInd, Canvas *pCanvas )
{
    Q_UNUSED( pCanvas )

    CanvasConstants c = pCanvas->constants();
    QPainter     ahrs( pHeadInd );
    double       dW = pHeadInd->width() - c.iFatPen;
    double       dH = pHeadInd->height() - c.iFatPen;
    double       dW2 = dW / 2.0;
    double       dH2 = dH / 2.0;
    QPen         linePen( Qt::white, c.iFatPen );
    bool         bThis = true;
    QColor       orange( 255, 165, 0 );
    QFontMetrics headMetrics( large );
    QString      qsHead;
    QRect        headRect;

    ahrs.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing, true );
    // Heading background
	ahrs.setPen( linePen );
	ahrs.setBrush( Qt::black );
    ahrs.drawEllipse( static_cast<double>( c.iFatPen ) / 2.0, static_cast<double>( c.iFatPen ) / 2.0,
                      dW - static_cast<double>( c.iFatPen ) / 2.0, dH - static_cast<double>( c.iFatPen ) / 2.0 );
    // Tens of degrees
    for( int i = 0; i < 360; i += 10 )
    {
        ahrs.translate( dW2, dH2 );
        ahrs.rotate( i );
        ahrs.translate( -dW2, -dH2 );
        ahrs.drawLine( dW2, c.iFatPen / 2, dW2, 20 - (c.iFatPen / 2) );
        ahrs.resetTransform();
    }
    // Every five degrees
    linePen.setWidth( 2 );
    linePen.setColor( Qt::gray );
    ahrs.setPen( linePen );
    for( int i = 5; i < 360; i += 5 )
    {
        if( bThis )
        {
            ahrs.translate( dW2, dH2 );
            ahrs.rotate( i );
            ahrs.translate( -dW2, -dH2 );
            ahrs.drawLine( dW2, 0, dW2, 10 - (c.iFatPen / 2) );
            ahrs.resetTransform();
            bThis = false;
        }
        else
            bThis = true;
    }
    ahrs.resetTransform();
    // Number labels
    ahrs.setFont( large );
    for( int i = 0; i < 36; i += 3 )
    {
        ahrs.translate( dW2, dH2 );
        ahrs.rotate( i * 10 );
        ahrs.translate( -dW2, -dH2 );
        if( i == 0 )
        {
            ahrs.setPen( orange );
            qsHead = "N";
        }
        else if( i == 9 )
        {
            ahrs.setPen( orange );
            qsHead = "E";
        }
        else if( i == 18 )
        {
            ahrs.setPen( orange );
            qsHead = "S";
        }
        else if( i == 27 )
        {
            ahrs.setPen( orange );
            qsHead = "W";
        }
        else
        {
            ahrs.setPen( Qt::white );
            qsHead = QString::number( abs( i ) );
        }
        headRect = headMetrics.boundingRect( qsHead );
#if defined( Q_OS_ANDROID )
        ahrs.drawText( dW2 - (headRect.width() / 2.0), headRect.height() + c.dH40, qsHead );
#else
        ahrs.drawText( dW2 - (headRect.width() / 2.0), headRect.height() + c.dH80, qsHead );
#endif
        ahrs.resetTransform();
    }
}

