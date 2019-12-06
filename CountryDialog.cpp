/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QScroller>
#include <QListWidget>
#include <QSettings>

#include "CountryDialog.h"


extern QSettings *g_pSet;


CountryDialog::CountryDialog( QWidget *pParent, CanvasConstants *pC )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_pC( pC )
{
    setupUi( this );

    QScroller::grabGesture( m_pAirportList, QScroller::LeftMouseButtonGesture );
    QScroller::grabGesture( m_pAirspaceList, QScroller::LeftMouseButtonGesture );

    populateCountriesAirports();
    populateCountriesAirspaces();

#if defined( Q_OS_ANDROID )
    m_pOKButton->setMinimumHeight( 100 );
#else
    m_pOKButton->setMinimumHeight( 50 );
#endif

    connect( m_pOKButton, SIGNAL( clicked() ), this, SLOT( ok() ) );
}


CountryDialog::~CountryDialog()
{
}


void CountryDialog::populateCountriesAirports()
{
    QVariantList     countries = g_pSet->value( "CountryAirports", QVariantList() ).toList();
    QVariant         country;
    QListWidgetItem *pCountryItem;

    foreach( country, countries )
    {
        pCountryItem = m_pAirportList->item( country.toInt() );

        if( pCountryItem != nullptr )
            pCountryItem->setSelected( true );
    }
}


void CountryDialog::populateCountriesAirspaces()
{
    QVariantList     countries = g_pSet->value( "CountryAirspaces", QVariantList() ).toList();
    QVariant         country;
    QListWidgetItem *pCountryItem;

    foreach( country, countries )
    {
        pCountryItem = m_pAirspaceList->item( country.toInt() );

        if( pCountryItem != nullptr )
            pCountryItem->setSelected( true );
    }
}


void CountryDialog::ok()
{
    QListWidgetItem *pCountry;
    int              iC;

    for( iC = 0; iC < m_pAirportList->count(); iC++ )
    {
        pCountry = m_pAirportList->item( iC );
        if( pCountry != nullptr )
        {
            if( pCountry->isSelected() )
                m_countryListAirports.append( static_cast<Canvas::CountryCodeAirports>( iC ) );
            else
                m_deleteCountryListAirports.append( static_cast<Canvas::CountryCodeAirports>( iC ) );
        }
    }
    for( iC = 0; iC < m_pAirspaceList->count(); iC++ )
    {
        pCountry = m_pAirspaceList->item( iC );
        if( pCountry != nullptr )
        {
            if( pCountry->isSelected() )
                m_countryListAirspaces.append( static_cast<Canvas::CountryCodeAirspace>( iC ) );
            else
                m_deleteCountryListAirspaces.append( static_cast<Canvas::CountryCodeAirspace>( iC ) );
        }
    }

    accept();
}
