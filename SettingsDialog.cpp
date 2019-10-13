/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QSettings>
#include <QtDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QString>

#include "SettingsDialog.h"
#include "Builder.h"
#include "TrafficMath.h"
#include "CountryDialog.h"


extern QSettings *g_pSet;


SettingsDialog::SettingsDialog( QWidget *pParent, CanvasConstants *pC )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_iTally( 0 ),
      m_pC( pC ),
      m_mapIt( m_mapUrls )
{
    setupUi( this );

    Builder::populateUrlMap( &m_mapUrls );

    loadSettings();
    m_settings.bShowAllTraffic = (!m_settings.bShowAllTraffic);
    traffic();
    m_settings.bShowRunways = (!m_settings.bShowRunways);
    runways();

    // Decrement the enum so when we call the cycler it changes to the right setting
    if( m_settings.eShowAirports == Canvas::ShowNoAirports )
        m_settings.eShowAirports = Canvas::ShowAllAirports;
    else if( m_settings.eShowAirports == Canvas::ShowGrassAirports )
        m_settings.eShowAirports = Canvas::ShowNoAirports;
    else
        m_settings.eShowAirports = Canvas::ShowGrassAirports;
    airports();     // Call the cycler to update and initialize the stored airport show selection

    connect( m_pAirportsButton, SIGNAL( clicked() ), this, SLOT( airports() ) );
    connect( m_pTrafficButton, SIGNAL( clicked() ), this, SLOT( traffic() ) );

    connect( m_pSwitchableButton, SIGNAL( clicked() ), this, SLOT( switchable() ) );

    connect( m_pShowRunwaysButton, SIGNAL( clicked() ), this, SLOT( runways() ) );

    connect( m_pGetDataButton, SIGNAL( clicked() ), this, SLOT( getMapData() ) );

    connect( m_pSelCountriesButton, SIGNAL( clicked() ), this, SLOT( selCountries() ) );

    m_pDataProgress->hide();
}


SettingsDialog::~SettingsDialog()
{
    g_pSet->setValue( "ShowAllTraffic", m_settings.bShowAllTraffic );
    g_pSet->setValue( "ShowAirports", static_cast<int>( m_settings.eShowAirports ) );
    g_pSet->setValue( "ShowRunways", m_settings.bShowRunways );
    g_pSet->setValue( "CurrDataSet", m_settings.iCurrDataSet );

    g_pSet->setValue( "StratuxIP", m_pIPEdit->text() );

    g_pSet->beginGroup( "FuelTanks" );
    g_pSet->setValue( "DualTanks", m_settings.bSwitchableTanks );
    g_pSet->endGroup();

    g_pSet->sync();
}


void SettingsDialog::loadSettings()
{
    QVariantList countries;
    QVariant     country;

    // Persistent settings
    m_settings.bShowAllTraffic = g_pSet->value( "ShowAllTraffic", true ).toBool();
    m_settings.eShowAirports = static_cast<Canvas::ShowAirports>( g_pSet->value( "ShowAirports", 2 ).toInt() );
    m_settings.bShowRunways = g_pSet->value( "ShowRunways", true ).toBool();
    m_settings.iCurrDataSet = g_pSet->value( "CurrDataSet", 0 ).toInt();
    m_settings.qsStratuxIP = g_pSet->value( "StratuxIP", "192.168.10.1" ).toString();
    countries = g_pSet->value( "CountryAirports", countries ).toList();
    m_settings.listCountries.clear();
    foreach( country, countries )
        m_settings.listCountries.append( static_cast<Canvas::CountryCode>( country.toInt() ) );
    g_pSet->beginGroup( "FuelTanks" );
    m_settings.bSwitchableTanks = (!g_pSet->value( "DualTanks" ).toBool());
    g_pSet->endGroup();
    switchable();

    m_pIPEdit->setText( m_settings.qsStratuxIP );

    Builder::getStorage( &m_qsInternalStoragePath );

    storage();
}


void SettingsDialog::traffic()
{
    m_settings.bShowAllTraffic = (!m_settings.bShowAllTraffic);

    if( m_settings.bShowAllTraffic )
        m_pTrafficButton->setText( "ALL TRAFFIC" );
    else
        m_pTrafficButton->setText( "CLOSE TRAFFIC" );

    emit trafficToggled( m_settings.bShowAllTraffic );
}


void SettingsDialog::airports()
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


void SettingsDialog::getMapData()
{
    QObject *pSender = sender();

    if( pSender == nullptr )
        return;

    if( m_settings.listCountries.count() == 0 )
        return;

    QString qsUrl, qsFile;

    m_pManager = new QNetworkAccessManager( this );

    m_mapIt = m_mapUrls;
    nextCountry();

    qsUrl = "http://skyfun.space/stratofier/openaip_airports_" + m_mapIt.value() + ".aip";
    qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapIt.value() + ".aip";
    m_url = qsUrl;
    m_pFile = new QFile( qsFile );
    m_iTally = 0;

    m_pCountryProgress->setFormat( QString( "%p% [%1]" ).arg( m_mapIt.value().right( 2 ).toUpper() ) );
    m_pCountryProgress->setMaximum( m_settings.listCountries.count() );
    m_pCountryProgress->setValue( 0 );
    m_pDataProgress->setValue( 0 );
    m_pDataProgress->show();

    // Something is wrong with the file path or permissions
    if( !m_pFile->open( QIODevice::WriteOnly) )
    {
        delete m_pFile;
        m_pFile = nullptr;
        m_pGetDataButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
    }
    else
        startRequest( m_url );
}


void SettingsDialog::storage()
{
    QDir        storagePath( settingsRoot() + "/space.skyfun.stratofier" );
    QString     qsFile;
    QStringList qsFiles = storagePath.entryList();
    int         iFound = 0;

    foreach( qsFile, qsFiles )
    {
        if( qsFile.endsWith( ".aip" ) )
            iFound++;
    }

    // Set the getter icons according to whether the data has been downloaded
    if( iFound >= m_settings.listCountries.count() )
    {
        m_pCountryProgress->setValue( 100 );
        m_pGetDataButton->setIcon( QIcon( ":/icons/resources/OK.png" ) );
    }
    else
    {
        m_pCountryProgress->setValue( 0 );
        m_pGetDataButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
    }
}


void SettingsDialog::httpReadyRead()
{
    if( m_pFile != nullptr )
        m_pFile->write( m_pReply->readAll() );
}


void SettingsDialog::updateDownloadProgress( qint64 bytesRead, qint64 totalBytes )
{
    m_pDataProgress->setMaximum( static_cast<int>( totalBytes ) );
    m_pDataProgress->setValue( static_cast<int>( bytesRead ) );
}


// When download finished or canceled, this will be called
void SettingsDialog::httpDownloadFinished()
{
    QString qsRoot( "http://skyfun.space/stratofier/" );
    QString qsFileRoot = settingsRoot() + "/space.skyfun.stratofier/";
    QString qsUrl;
    QString qsFile;

    m_pFile->flush();
    m_pFile->close();

    m_pReply->deleteLater();
    m_pReply = nullptr;
    delete m_pFile;
    m_pFile = nullptr;
    m_iTally++;
    m_pCountryProgress->setValue( m_iTally );

    if( !m_mapIt.hasNext() )
    {
        m_pCountryProgress->setFormat( "%p% [ALL]" );
        m_pGetDataButton->setIcon( QIcon( ":/icons/resources/OK.png" ) );
        m_pManager = nullptr;
        m_pDataProgress->hide();
    }
    else
    {
        nextCountry();
        m_pDataProgress->setValue( 0 );
        m_pCountryProgress->setFormat( QString( "%p% [%1]" ).arg( m_mapIt.value().right( 2 ).toUpper() ) );
        qsUrl = qsRoot + "openaip_airports_" + m_mapIt.value() + ".aip";
        qsFile = qsFileRoot + m_mapIt.value() + ".aip";
        m_url = qsUrl;
        m_pFile = new QFile( qsFile );

        // Something is wrong with the file path or permissions
        if( !m_pFile->open( QIODevice::WriteOnly) )
        {
            delete m_pFile;
            m_pFile = nullptr;
            m_pGetDataButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
        }
        else
            startRequest( m_url );
    }
}

// This will be called when download button is clicked
void SettingsDialog::startRequest( QUrl url )
{
    m_pGetDataButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );

    m_pReply = m_pManager->get( QNetworkRequest( url ) );

    connect( m_pReply, SIGNAL( readyRead() ), this, SLOT( httpReadyRead() ) );
    connect( m_pReply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( updateDownloadProgress( qint64, qint64 ) ) );
    connect( m_pReply, SIGNAL( finished() ), this, SLOT( httpDownloadFinished() ) );
}


const QString SettingsDialog::settingsRoot()
{
#if defined( Q_OS_ANDROID )
    return m_qsInternalStoragePath + "/data";
#else
    QString qsHomeStratofier = QDir::homePath() + "/stratofier_data/data";
    QDir    data( qsHomeStratofier );

    data.mkpath( qsHomeStratofier + "/space.skyfun.stratofier" );

    return qsHomeStratofier;
#endif
}


void SettingsDialog::switchable()
{
    m_settings.bSwitchableTanks = (!m_settings.bSwitchableTanks);

    g_pSet->beginGroup( "FuelTanks" );
    g_pSet->setValue( "DualTanks", m_settings.bSwitchableTanks );
    g_pSet->endGroup();
    g_pSet->sync();

    m_pSwitchableButton->setIcon( m_settings.bSwitchableTanks ? QIcon( ":/icons/resources/OK.png" ) : QIcon() );
}


void SettingsDialog::runways()
{
    m_settings.bShowRunways = (!m_settings.bShowRunways);

    g_pSet->setValue( "ShowRunways", m_settings.bShowRunways );
    g_pSet->sync();

    m_pShowRunwaysButton->setIcon( m_settings.bShowRunways ? QIcon( ":/icons/resources/OK.png" ) : QIcon() );

    emit showRunways( m_settings.bShowRunways );
}


void SettingsDialog::selCountries()
{
    CountryDialog  dlg( this, m_pC );

    // This geometry works for both orientations since dW is actually half width in landscape
    dlg.setGeometry( 0, 0, static_cast<int>( m_pC->dW ), static_cast<int>( m_pC->dH ) );
    if( dlg.exec() == QDialog::Accepted )
    {
        Canvas::CountryCode        country;
        QVariantList          countries;
        QList<Canvas::CountryCode> deleteCountries;
        QString               qsFile;

        m_settings.listCountries = dlg.countries();
        deleteCountries = dlg.deleteCountries();
        foreach( country, m_settings.listCountries )
            countries.append( static_cast<int>( country ) );
        foreach( country, deleteCountries )
        {
            qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapUrls[country] + ".aip";
            QFile::remove( qsFile );
        }
        g_pSet->setValue( "CountryAirports", countries );
        g_pSet->sync();
    }
}


void SettingsDialog::nextCountry()
{
    Canvas::CountryCode country;
    bool           bFound = false;

    // Move the iterator to the next selected country
    while( m_mapIt.hasNext() )
    {
        m_mapIt.next();
        foreach( country, m_settings.listCountries )
        {
            if( m_mapIt.key() == country )
            {
                bFound = true;
                break;
            }
        }
        if( bFound )
            break;
    }
}
