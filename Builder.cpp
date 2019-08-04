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


// Build the vertical speed tape pixmap
/*
void Builder::buildVertSpeedTape( QPixmap *pVertTape, Canvas *pCanvas, bool bPortrait )
{
    QPainter        ahrs( pVertTape );
    int             iVert, iV = 1, iY;
    CanvasConstants c = pCanvas->constants();
    double          dLineHeight;
    QString         qsVSpeed;
    QFont           tinyBold( "Piboto", 36, QFont::Bold );

    if( bPortrait )
        dLineHeight = ((c.dH2 * 4.0) - 120.0) / 40.0;
    else
        dLineHeight = ((c.dH * 4.0) - 120.0) / 40.0;

    ahrs.setFont( tinyBold );
	for( iVert = 10; iVert >= -10; iVert-- )
    {
        iY = static_cast<int>( static_cast<double>( iV ) * dLineHeight * 2.0 );
        if( (iVert % 2) == 0 )
        {
            qsVSpeed = QString::number( abs( iVert * 2 ) );
            ahrs.setPen( Qt::black );
            ahrs.drawText( c.dW20 + c.dW80 + 1, iY + 1, qsVSpeed );
            ahrs.setPen( Qt::white );
            ahrs.drawText( c.dW20 + c.dW80, iY, qsVSpeed );
        }
        iY -= 3;
        ahrs.setPen( QPen( Qt::white, c.dH160 ) );
        ahrs.drawLine( 5, iY - 12, c.dW20, iY - 12 );
        iV++;
    }
}
*/


void Builder::buildNumber( QPixmap *pNumber, CanvasConstants *c, int iNum, int iFieldWidth )
{
    pNumber->fill( Qt::transparent );

    QPainter        numPainter( pNumber );
    QString         qsNum = QString( "%1" ).arg( iNum, iFieldWidth, 10, QChar( '0' ) );
    QChar           cNum;
    int             iX = 0;

    foreach( cNum, qsNum )
    {
        QPixmap num( QString( ":/num/resources/%1.png" ).arg( cNum ) );
        numPainter.drawPixmap( iX, 0, c->dWNum, c->dHNum, num );
        iX += static_cast<int>( c->dWNum );
    }
}


void Builder::buildNumber( QPixmap *pNumber, CanvasConstants *c, const QString &qsNum )
{
    pNumber->fill( Qt::transparent );

    QPainter numPainter( pNumber );
    QChar    cNum;
    int      iX = 0;
    QString  qsDigit;

    foreach( cNum, qsNum )
    {
        if( cNum == ':' )
            qsDigit = ":/num/resources/colon.png";
        else
            qsDigit = QString( ":/num/resources/%1.png" ).arg( cNum );
        QPixmap num( qsDigit );
        numPainter.drawPixmap( iX, 0, c->dWNum, c->dHNum, num );
        iX += static_cast<int>( c->dWNum );
    }
}

