/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QKeyEvent>
#include <QTimer>
#include <QSettings>
#include <QDebug>

#include "Overlays.h"


extern QSettings *g_pSet;


Overlays::Overlays( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );

    loadSettings();
    m_settings.bShowAllTraffic = (!m_settings.bShowAllTraffic);
    traffic();
    m_settings.bShowRunways = (!m_settings.bShowRunways);
    runways();
    m_settings.bShowAirspaces = (!m_settings.bShowAirspaces);
    airspaces();
    m_settings.bShowAltitudes = (!m_settings.bShowAltitudes);
    altitudes();

    // Decrement the enum so when we call the cycler it changes to the right setting
    if( m_settings.eShowAirports == Canvas::ShowNoAirports )
        m_settings.eShowAirports = Canvas::ShowAllAirports;
    else if( m_settings.eShowAirports == Canvas::ShowGrassAirports )
        m_settings.eShowAirports = Canvas::ShowNoAirports;
    else if( m_settings.eShowAirports == Canvas::ShowPavedAirports )
        m_settings.eShowAirports = Canvas::ShowGrassAirports;
    else
        m_settings.eShowAirports = Canvas::ShowPavedAirports;
    airports();     // Call the cycler to update and initialize the stored airport show selection

    connect( m_pAirportsButton, SIGNAL( clicked() ), this, SLOT( airports() ) );
    connect( m_pTrafficButton, SIGNAL( clicked() ), this, SLOT( traffic() ) );
    connect( m_pShowRunwaysButton, SIGNAL( clicked() ), this, SLOT( runways() ) );
    connect( m_pShowAirspacesButton, SIGNAL( clicked() ), this, SLOT( airspaces() ) );
    connect( m_pShowAltButton, SIGNAL( clicked() ), this, SLOT( altitudes() ) );
    connect( m_pShowPrivateButton, SIGNAL( clicked() ), this, SLOT( privateAirports() ) );

    QTimer::singleShot( 100, this, SLOT( init() ) );
}


Overlays::~Overlays()
{
    g_pSet->setValue( "ShowAllTraffic", m_settings.bShowAllTraffic );
    g_pSet->setValue( "ShowAirports", static_cast<int>( m_settings.eShowAirports ) );
    g_pSet->setValue( "ShowRunways", m_settings.bShowRunways );
    g_pSet->setValue( "ShowAirspaces", m_settings.bShowAirspaces );
    g_pSet->setValue( "ShowAltitudes", m_settings.bShowAltitudes );
    g_pSet->setValue( "ShowPrivate", m_settings.bShowPrivate );
    g_pSet->sync();
}


void Overlays::init()
{
    int   iBtnHeight = m_pShowRunwaysButton->height();    // They're all the same height
    QSize iconSize( static_cast<int>( static_cast<double>( iBtnHeight ) / 2.0 * 2.3 ), iBtnHeight / 2 );

    m_pShowRunwaysButton->setIconSize( iconSize );
    m_pShowAirspacesButton->setIconSize( iconSize );
    m_pShowAltButton->setIconSize( iconSize );
    m_pShowPrivateButton->setIconSize( iconSize );
}


void Overlays::traffic()
{
    m_settings.bShowAllTraffic = (!m_settings.bShowAllTraffic);

    if( m_settings.bShowAllTraffic )
        m_pTrafficButton->setText( "ALL TRAFFIC" );
    else
        m_pTrafficButton->setText( "CLOSE TRAFFIC" );

    emit trafficToggled( m_settings.bShowAllTraffic );
}


void Overlays::airports()
{
    if( m_settings.eShowAirports == Canvas::ShowNoAirports )
    {
        m_pAirportsButton->setStyleSheet( "QPushButton { border: none; background-color: LimeGreen; color: black; margin: 2px }" );
        m_settings.eShowAirports = Canvas::ShowGrassAirports;
        m_pAirportsButton->setText( "GRASS AIRPORTS" );
    }
    else if( m_settings.eShowAirports == Canvas::ShowGrassAirports )
    {
        m_pAirportsButton->setStyleSheet( "QPushButton { border: none; background-color: CornflowerBlue; color: black; margin: 2px }" );
        m_settings.eShowAirports = Canvas::ShowPavedAirports;
        m_pAirportsButton->setText( "PAVED AIRPORTS" );
    }
    else if( m_settings.eShowAirports == Canvas::ShowPavedAirports )
    {
        m_pAirportsButton->setStyleSheet( "QPushButton { border: none; background-color: Gainsboro; color: black; margin: 2px }" );
        m_settings.eShowAirports = Canvas::ShowAllAirports;
        m_pAirportsButton->setText( "ALL AIRPORTS" );
    }
    else
    {
        m_pAirportsButton->setStyleSheet( "QPushButton { border: none; background-color: LightCoral; color: black; margin: 2px }" );
        m_settings.eShowAirports = Canvas::ShowNoAirports;
        m_pAirportsButton->setText( "NO AIRPORTS" );
    }

    emit showAirports( m_settings.eShowAirports );
}


void Overlays::privateAirports()
{
    m_settings.bShowPrivate = (!m_settings.bShowPrivate);

    g_pSet->setValue( "ShowPrivate", m_settings.bShowPrivate );
    g_pSet->sync();

    m_pShowPrivateButton->setIcon( m_settings.bShowPrivate ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );

    emit showPrivate( m_settings.bShowPrivate );

}


void Overlays::runways()
{
    m_settings.bShowRunways = (!m_settings.bShowRunways);

    g_pSet->setValue( "ShowRunways", m_settings.bShowRunways );
    g_pSet->sync();

    m_pShowRunwaysButton->setIcon( m_settings.bShowRunways ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );

    emit showRunways( m_settings.bShowRunways );
}


void Overlays::airspaces()
{
    m_settings.bShowAirspaces = (!m_settings.bShowAirspaces);

    g_pSet->setValue( "ShowAirspaces", m_settings.bShowAirspaces );
    g_pSet->sync();

    m_pShowAirspacesButton->setIcon( m_settings.bShowAirspaces ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );

    emit showAirspaces( m_settings.bShowAirspaces );
}


void Overlays::altitudes()
{
    m_settings.bShowAltitudes = (!m_settings.bShowAltitudes);

    g_pSet->setValue( "ShowAltitudes", m_settings.bShowAltitudes );
    g_pSet->sync();

    m_pShowAltButton->setIcon( m_settings.bShowAltitudes ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );

    emit showAltitudes( m_settings.bShowAltitudes );
}


void Overlays::loadSettings()
{
    // Persistent settings
    m_settings.bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();
    m_settings.eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", static_cast<int>( Canvas::ShowPavedAirports ) ).toInt() );
    m_settings.bShowRunways = g_pSet->value( "ShowRunways", true ).toBool();
    m_settings.bShowAirspaces = g_pSet->value( "ShowAirspaces", true ).toBool();
    m_settings.bShowAltitudes = g_pSet->value( "ShowAltitudes" ).toBool();
    m_settings.bShowPrivate = g_pSet->value( "ShowPrivate", false ).toBool();
}


