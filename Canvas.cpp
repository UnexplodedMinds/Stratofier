/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QFont>
#include <QFontMetrics>
#include <QRect>
#include <QSettings>

#include "Canvas.h"


extern QFont wee;
extern QFont tiny;
extern QFont small;
extern QFont med;
extern QFont large;


Canvas::Canvas( double dWidth, double dHeight, bool bPortrait )
{
	QFontMetrics weeMetrics( wee );
	QRect        weeRect( weeMetrics.boundingRect( "0" ) );
	QFontMetrics tinyMetrics( tiny );
    QRect        tinyRect( tinyMetrics.boundingRect( "0" ) );
    QFontMetrics smallMetrics( small );
    QRect        smallRect( smallMetrics.boundingRect( "0" ) );
    QFontMetrics medMetrics( med );
    QRect        medRect( medMetrics.boundingRect( "0" ) );
    QFontMetrics largeMetrics( large );
    QRect        largeRect( largeMetrics.boundingRect( "0" ) );

    m_preCalc.bPortrait = (dWidth < dHeight);

    // Actual width regardless
    m_preCalc.dWa = dWidth;

    // All calculations for either the heading indicator or the attitude indicator will only occupy half the physical width in landscape mode so just halve it now.
    if( !bPortrait )
        dWidth /= 2.0;

    m_preCalc.dH = dHeight;
    m_preCalc.dH2 = dHeight / 2.0;
    m_preCalc.dH4 = dHeight / 4.0;
    m_preCalc.dH5 = dHeight / 5.0;
    m_preCalc.dH7 = dHeight / 7.0;
    m_preCalc.dH10 = dHeight / 10.0;
    m_preCalc.dH20 = dHeight / 20.0;
	m_preCalc.dH30 = dHeight / 30.0;
	m_preCalc.dH40 = dHeight / 40.0;
    m_preCalc.dH80 = dHeight / 80.0;
    m_preCalc.dH100 = dHeight / 100.0;
	m_preCalc.dH160 = dHeight / 160.0;
    m_preCalc.dW = dWidth;
    m_preCalc.dW2 = dWidth / 2.0;
    m_preCalc.dW4 = dWidth / 4.0;
    m_preCalc.dW5 = dWidth / 5.0;
    m_preCalc.dW7 = dWidth / 7.0;
    m_preCalc.dW10 = dWidth / 10.0;
    m_preCalc.dW20 = dWidth / 20.0;
	m_preCalc.dW30 = dWidth / 30.0;
	m_preCalc.dW40 = dWidth / 40.0;
    m_preCalc.dW80 = dWidth / 80.0;

	m_preCalc.iWeeFontHeight = weeRect.height();
	m_preCalc.iTinyFontHeight = tinyRect.height();
    m_preCalc.iSmallFontHeight = smallRect.height();
    m_preCalc.iMedFontHeight = medRect.height();
    m_preCalc.iLargeFontHeight = largeRect.height();
    m_preCalc.iTinyFontWidth = tinyRect.width();

// This just works out for Android font scaling (I don't know why)
#if defined( Q_OS_ANDROID )
    m_preCalc.iWeeFontHeight *= 4;
    m_preCalc.iTinyFontHeight *= 4;
    m_preCalc.iSmallFontHeight *= 4;
    m_preCalc.iMedFontHeight *= 4;
    m_preCalc.iLargeFontHeight *= 4;
    m_preCalc.iTinyFontWidth *= 4;

    m_preCalc.iThinPen = 4;
    m_preCalc.iThickPen = 6;
    m_preCalc.iFatPen = 10;
#else
    m_preCalc.iThinPen = 2;
    m_preCalc.iThickPen = 3;
    m_preCalc.iFatPen = 5;
#endif

    m_preCalc.dAspectL = m_preCalc.dW / m_preCalc.dH;
    m_preCalc.dAspectP = m_preCalc.dH / m_preCalc.dW;
}


CanvasConstants Canvas::constants()
{
    return m_preCalc;
}


double Canvas::scaledH( double d )
{
    if( m_preCalc.bPortrait )
        return m_preCalc.dWa * d / 480.0;
    else
        return m_preCalc.dWa * d / 800.0;
}


double Canvas::scaledV( double d )
{
    if( m_preCalc.bPortrait )
        return m_preCalc.dH * d / 800.0;
    else
        return m_preCalc.dH * d / 480.0;
}


int Canvas::largeWidth( const QString &qsText )
{
    QFontMetrics largeMetrics( large );
    QRect        largeRect( largeMetrics.boundingRect( qsText ) );

    return largeRect.width();
}


int Canvas::medWidth(const QString &qsText)
{
	QFontMetrics medMetrics( med );
	QRect        medRect( medMetrics.boundingRect( qsText ) );

    return medRect.width();
}
