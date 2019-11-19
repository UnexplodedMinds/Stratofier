/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/


#include "DetailsDialog.h"


DetailsDialog::DetailsDialog( QWidget *pParent, CanvasConstants *pC, Airport *pA )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    QString   qsHtml( "<html><body>\n" );
    Frequency f;
    int       r;
    int       i = 0;

    setupUi( this );

    m_pCancelButton->setMinimumHeight( static_cast<int>( pC->dH10 ) );

    qsHtml.append( QString( "<p align=\"center\" style=\"font: 24pt Helvetica; color: darkRed;\"><b>&nbsp;%1</b></p>\n"
                            "<p align=\"center\" style=\"font: 12pt Helvetica; color: darkRed;\"><b>&nbsp;%2</b></p>\n"
                            "<table><tr><td style=\"font: 14pt Helvetica;\"><b>&nbsp;Elevation:</b><td width=\"40\"></td></td><td style=\"font: 18pt Helvetica; color: blue;\"><b>%3</b></td></tr></table><br />\n"
                            "<div style=\"font: 14pt Helvetica;\"><b>&nbsp;Frequencies:</b><br />\n"
                            "<table style=\"font: 18pt Helvetica;\">\n" )
                    .arg( pA->qsID )
                    .arg( pA->qsName )
                    .arg( pA->dElev ) );
    foreach( f, pA->frequencies )
        qsHtml.append( QString( "<tr><td>&nbsp;%1</td><td width=\"40\"></td><td style=\"color: blue;\"><b>%2</b></td></tr>\n" ).arg( f.qsDescription ).arg( f.dFreq ) );
    qsHtml.append( "</table></div>\n<br />\n<div style=\"font: 14pt Helvetica;\"><b>&nbsp;Runways:</b><br />"
                   "<table style=\"font: 18pt Helvetica;\">\n<tr>\n" );
    foreach( r, pA->runways )
    {
        i++;
        qsHtml.append( QString( "<td style=\"font: 18pt Helvetica; color: blue;\">&nbsp;<b>%1%2</td><td width=\"20\">" ).arg( r / 10 ).arg( (i == pA->runways.count()) ? " </b>" : ",</b>" ) );
    }
    qsHtml.append( "\n</tr>\n</table></div>\n" );
    qsHtml.append( "</body></html>\n" );

    m_pDetailsEdit->setHtml( qsHtml );
}


DetailsDialog::~DetailsDialog()
{
}


