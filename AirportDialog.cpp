/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QScroller>
#include <QtDebug>
#include <QTableWidget>

#include "AirportDialog.h"
#include "TrafficMath.h"
#include "StratuxStreams.h"


extern QList<Airport>   g_airportCache;
extern StratuxSituation g_situation;


AirportDialog::AirportDialog( QWidget *pParent, CanvasConstants *pC, const QString &qsTitle )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_pC( pC ),
      m_bAllAirports( false ),
      m_dDist( 25.0 )
{
    setupUi( this );
    m_pTitleLabel->setText( qsTitle );

    QScroller::grabGesture( m_pAirportsTable, QScroller::LeftMouseButtonGesture );

    m_pCancelButton->setMinimumHeight( static_cast<int>( pC->dH10 ) );

    connect( m_pRange25, SIGNAL( clicked() ), this, SLOT( within() ) );
    connect( m_pRange50, SIGNAL( clicked() ), this, SLOT( within() ) );
    connect( m_pRange100, SIGNAL( clicked() ), this, SLOT( within() ) );
    connect( m_pRange200, SIGNAL( clicked() ), this, SLOT( within() ) );
    connect( m_pAirportsButton, SIGNAL( clicked() ), this, SLOT( airportsType() ) );

    connect( m_pAirportsTable, SIGNAL( itemPressed( QTableWidgetItem*) ), this, SLOT( airportSelected( QTableWidgetItem* ) ) );

    updateAirports();
}


AirportDialog::~AirportDialog()
{
}


void AirportDialog::within()
{
    QObject *pObj = sender();

    // Should never happen
    if( pObj == nullptr )
        return;

    QString qsObj = pObj->objectName();

    // Should also never happen
    if( qsObj.isEmpty() )
        return;

    if( qsObj == "m_pRange25" )
        m_dDist = 25.0;
    else if( qsObj == "m_pRange50" )
        m_dDist = 50.0;
    else if( qsObj == "m_pRange100" )
        m_dDist = 100.0;
    else
        m_dDist = 200.0;

    toggle( pObj );
    updateAirports();
}


void AirportDialog::toggle( QObject *pObj )
{
    QString      qsOn = "QPushButton { border: none; background-color: CornflowerBlue; }";
    QString      qsOff = "QPushButton { border: none; background-color: DimGray; }";
    QPushButton *pBtn = qobject_cast<QPushButton *>( pObj );

    m_pRange25->setStyleSheet( qsOff );
    m_pRange50->setStyleSheet( qsOff );
    m_pRange100->setStyleSheet( qsOff );
    m_pRange200->setStyleSheet( qsOff );
    pBtn->setStyleSheet( qsOn );
}


void AirportDialog::updateAirports()
{
    Airport      ap;
    QFontMetrics lineMetric( m_pAirportsTable->font() );
    int          iRowHeight = static_cast<int>( static_cast<double>( lineMetric.boundingRect( "K" ).height() * 1.5 ) );

    // Clear the table
    while( m_pAirportsTable->rowCount() > 0 )
        m_pAirportsTable->removeRow( 0 );

    // Populate the table
    foreach( ap, g_airportCache )
    {
        ap.bd = TrafficMath::haversine( g_situation.dGPSlat, g_situation.dGPSlong, ap.dLat, ap.dLong );

        if( ap.bd.dDistance <= m_dDist )
        {
            if( m_bAllAirports || ((!m_bAllAirports) && (ap.qsID != "R")) )
            {
                m_pAirportsTable->setRowCount( m_pAirportsTable->rowCount() + 1 );
                m_pAirportsTable->setRowHeight( m_pAirportsTable->rowCount() - 1, iRowHeight );
                m_pAirportsTable->setItem( m_pAirportsTable->rowCount() - 1, 0, new QTableWidgetItem( ap.qsName ) );
                m_pAirportsTable->setItem( m_pAirportsTable->rowCount() - 1, 1, new QTableWidgetItem( "  " + ap.qsID ) );
                m_pAirportsTable->setItem( m_pAirportsTable->rowCount() - 1, 2, new QTableWidgetItem( QString( "  %1" ).arg( static_cast<int>( ap.bd.dDistance ), 3, 10, QChar( '0' ) ) ) );
            }
        }
    }
    m_pAirportsTable->resizeColumnsToContents();
    if( m_pC->bPortrait )
        m_pAirportsTable->setColumnWidth( 0, static_cast<int>( m_pC->dW2 + m_pC->dW10 ) );
    else
        m_pAirportsTable->setColumnWidth( 0, static_cast<int>( m_pC->dW2 ) );

    // Sort by distance from you
    m_pAirportsTable->sortByColumn( 2, Qt::AscendingOrder );
}


void AirportDialog::airportsType()
{
    m_bAllAirports = (!m_bAllAirports);

    if( m_bAllAirports )
        m_pAirportsButton->setText( "ALL" );
    else
        m_pAirportsButton->setText( "PUBLIC" );

    updateAirports();
}


void AirportDialog::airportSelected( QTableWidgetItem *pItem )
{
    m_qsSel = m_pAirportsTable->item( pItem->row(), 0 )->text();
    accept();
}

