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

    QScroller::grabGesture( m_pCountryList, QScroller::LeftMouseButtonGesture );

    populateCountries();

    connect( m_pOKButton, SIGNAL( clicked() ), this, SLOT( ok() ) );
}


CountryDialog::~CountryDialog()
{
}


void CountryDialog::populateCountries()
{
    QVariantList     countries = g_pSet->value( "CountryAirports", QVariantList() ).toList();
    QVariant         country;
    QListWidgetItem *pCountryItem;

    foreach( country, countries )
    {
        pCountryItem = m_pCountryList->item( country.toInt() );

        if( pCountryItem != nullptr )
            pCountryItem->setSelected( true );
    }
}


void CountryDialog::ok()
{
    QListWidgetItem *pCountry;
    int              iC;

    for( iC = 0; iC < m_pCountryList->count(); iC++ )
    {
        pCountry = m_pCountryList->item( iC );
        if( pCountry != nullptr )
        {
            if( pCountry->isSelected() )
                m_countryList.append( static_cast<Canvas::CountryCode>( iC ) );
            else
                m_deleteCountryList.append( static_cast<Canvas::CountryCode>( iC ) );
        }
    }

    accept();
}
