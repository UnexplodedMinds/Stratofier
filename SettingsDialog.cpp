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
#include <QTimer>

#include "SettingsDialog.h"
#include "Builder.h"
#include "TrafficMath.h"
#include "CountryDialog.h"
#include "ClickLabel.h"


extern QSettings *g_pSet;


SettingsDialog::SettingsDialog( QWidget *pParent, CanvasConstants *pC )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_iTally( 0 ),
      m_pC( pC ),
      m_mapItAirports( m_mapUrlsAirports ),
      m_mapItAirspaces( m_mapUrlsAirspaces )
{
    setupUi( this );

    m_pDataProgress->hide();

    m_pIPClickLabel->setFullKeyboard( true );
    m_pOwnshipClickLabel->setFullKeyboard( true );

    Builder::populateUrlMapAirports( &m_mapUrlsAirports );
    Builder::populateUrlMapAirspaces( &m_mapUrlsAirspaces );

    loadSettings();

    connect( m_pSwitchableButton, SIGNAL( clicked() ), this, SLOT( switchable() ) );
    connect( m_pEnableBTButton, SIGNAL( clicked() ), this, SLOT( enableBT() ) );

    connect( m_pGetButton, SIGNAL( clicked() ), this, SLOT( getMapDataAirports() ) );

    connect( m_pSelCountriesButton, SIGNAL( clicked() ), this, SLOT( selCountries() ) );

    QTimer::singleShot( 100, this, SLOT( init() ) );
}


SettingsDialog::~SettingsDialog()
{
    g_pSet->setValue( "CurrDataSet", m_settings.iCurrDataSet );

    g_pSet->setValue( "StratuxIP", m_pIPClickLabel->text().simplified().remove( ' ' ) );
    g_pSet->setValue( "OwnshipID", m_pOwnshipClickLabel->text().simplified().toUpper().remove( ' ' ) );

    g_pSet->setValue( "EnableBluetooth", m_settings.bEnableBT );

    g_pSet->beginGroup( "FuelTanks" );
    g_pSet->setValue( "DualTanks", m_settings.bSwitchableTanks );
    g_pSet->endGroup();

    g_pSet->sync();
}


void SettingsDialog::init()
{
    int   iBtnHeight = m_pSwitchableButton->height();    // They're all the same height
    QSize iconSize( static_cast<int>( static_cast<double>( iBtnHeight ) / 2.0 * 2.3 ), iBtnHeight / 2 );

    m_pSwitchableButton->setIconSize( iconSize );
    m_pEnableBTButton->setIconSize( iconSize );
}


void SettingsDialog::loadSettings()
{
    QVariantList countries;
    QVariant     country;

    // Persistent settings
    m_settings.iCurrDataSet = g_pSet->value( "CurrDataSet", 0 ).toInt();
    m_settings.qsStratuxIP = g_pSet->value( "StratuxIP", "192.168.10.1" ).toString();
    m_settings.qsOwnshipID = g_pSet->value( "OwnshipID", QString() ).toString();
    countries = g_pSet->value( "CountryAirports", countries ).toList();
    m_settings.listAirports.clear();
    foreach( country, countries )
        m_settings.listAirports.append( static_cast<Canvas::CountryCodeAirports>( country.toInt() ) );
    countries = g_pSet->value( "CountryAirspaces", countries ).toList();
    m_settings.listAirspaces.clear();
    foreach( country, countries )
        m_settings.listAirspaces.append( static_cast<Canvas::CountryCodeAirspace>( country.toInt() ) );
    g_pSet->beginGroup( "FuelTanks" );
    m_settings.bSwitchableTanks = (!g_pSet->value( "DualTanks" ).toBool());
    g_pSet->endGroup();
    switchable();
    m_settings.bEnableBT = (!g_pSet->value( "EnableBluetooth" ).toBool());
    enableBT();

    m_pIPClickLabel->setText( m_settings.qsStratuxIP );
    m_pOwnshipClickLabel->setText( m_settings.qsOwnshipID );

    Builder::getStorage( &m_qsInternalStoragePath );

    storage();
}


void SettingsDialog::getMapDataAirports()
{
    QObject *pSender = sender();

    if( pSender == nullptr )
        return;

    if( m_settings.listAirports.count() == 0 )
        return;

    QString qsUrl, qsFile, qsValue;

    m_pManager = new QNetworkAccessManager( this );

    m_mapItAirports = m_mapUrlsAirports;
    if( !nextCountryAirport() )
        return;

    qsUrl = "http://skyfun.space/stratofier/openaip_" + m_mapItAirports.value() + ".aip";
    qsValue = m_mapItAirports.value();
    m_pDataProgress->setFormat( QString( "%p% %1 AP" ).arg( qsValue.right( 2 ).toUpper() ) );

    qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapItAirports.value() + ".aip";
    m_url = qsUrl;
    m_pFile = new QFile( qsFile );
    m_iTally = 0;

    m_pDataProgress->setMaximum( 100 );
    m_pDataProgress->setValue( 0 );

    // Something is wrong with the file path or permissions
    if( !m_pFile->open( QIODevice::WriteOnly) )
    {
        delete m_pFile;
        m_pFile = nullptr;
        m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
    }
    else
    {
        m_pDataProgress->show();
        startRequestAirports( m_url );
    }
}


void SettingsDialog::getMapDataAirspace()
{
    QObject *pSender = sender();

    if( pSender == nullptr )
        return;

    if( m_settings.listAirspaces.count() == 0 )
        return;

    QString qsUrl, qsFile, qsValue;

    m_pManager = new QNetworkAccessManager( this );

    m_mapItAirspaces = m_mapUrlsAirspaces;
    if( !nextCountryAirspace() )
        return;

    qsUrl = "http://skyfun.space/stratofier/openaip_" + m_mapItAirspaces.value() + ".aip";
    qsValue = m_mapItAirspaces.value();
    m_pDataProgress->setFormat( QString( "%p% %1 AS" ).arg( qsValue.right( 2 ).toUpper() ) );

    qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapItAirspaces.value() + ".aip";
    m_url = qsUrl;
    m_pFile = new QFile( qsFile );
    m_iTally = 0;

    m_pDataProgress->setMaximum( 100 );
    m_pDataProgress->setValue( 0 );

    // Something is wrong with the file path or permissions
    if( !m_pFile->open( QIODevice::WriteOnly) )
    {
        delete m_pFile;
        m_pFile = nullptr;
        m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
    }
    else
    {
        m_pDataProgress->show();
        startRequestAirspace( m_url );
    }
}


void SettingsDialog::storage()
{
    QDir        storagePath( settingsRoot() + "/space.skyfun.stratofier" );
    QString     qsFile;
    QStringList qslFiles = storagePath.entryList();
    int         iFound = 0;

    foreach( qsFile, qslFiles )
    {
        if( qsFile.startsWith( "airports" ) && qsFile.endsWith( ".aip" ) )
            iFound++;
    }

    // Set the getter icons according to whether the data has been downloaded
    if( iFound >= m_settings.listAirports.count() )
    {
        m_pDataProgress->setValue( 100 );
        m_pGetButton->setIcon( QIcon( ":/icons/resources/OK.png" ) );
    }
    else
    {
        m_pDataProgress->setValue( 0 );
        m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
    }

    iFound = 0;
    foreach( qsFile, qslFiles )
    {
        if( qsFile.startsWith( "airspace" ) && qsFile.endsWith( ".aip" ) )
            iFound++;
    }

    // Set the getter icons according to whether the data has been downloaded
    if( iFound >= m_settings.listAirspaces.count() )
    {
        m_pDataProgress->setValue( 100 );
        m_pGetButton->setIcon( QIcon( ":/icons/resources/OK.png" ) );
    }
    else
    {
        m_pDataProgress->setValue( 0 );
        m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
    }
}


void SettingsDialog::httpReadyRead()
{
    if( m_pFile != nullptr )
        m_pFile->write( m_pReply->readAll() );
}


void SettingsDialog::updateDownloadProgressAirports( qint64 bytesRead, qint64 totalBytes )
{
    m_pDataProgress->setMaximum( static_cast<int>( totalBytes ) );
    m_pDataProgress->setValue( static_cast<int>( bytesRead ) );
}


void SettingsDialog::updateDownloadProgressAirspace( qint64 bytesRead, qint64 totalBytes )
{
    m_pDataProgress->setMaximum( static_cast<int>( totalBytes ) );
    m_pDataProgress->setValue( static_cast<int>( bytesRead ) );
}


// When download finished or canceled, this will be called
void SettingsDialog::httpDownloadFinishedAirports()
{
    QString qsRoot( "http://skyfun.space/stratofier/" );
    QString qsFileRoot = settingsRoot() + "/space.skyfun.stratofier/";
    QString qsUrl;
    QString qsFile;
    QString qsValue;

    m_pFile->flush();
    m_pFile->close();

    m_pReply->deleteLater();
    m_pReply = nullptr;
    delete m_pFile;
    m_pFile = nullptr;
    m_iTally++;

    if( !m_mapItAirports.hasNext() )
    {
        m_pDataProgress->setFormat( "%p%" );
        if( m_iTally >= m_settings.listAirports.count() )
        {
            m_iTally = 0;
            QTimer::singleShot( 100, this, SLOT( getMapDataAirspace() ) );
        }
        m_pManager = nullptr;
    }
    else
    {
        if( !nextCountryAirport() )
        {
            if( m_iTally >= m_settings.listAirports.count() )
            {
                m_iTally = 0;
                QTimer::singleShot( 100, this, SLOT( getMapDataAirspace() ) );
            }
            m_pManager = nullptr;
            return;
        }
        m_pDataProgress->setValue( 0 );
        qsValue = m_mapItAirports.value();
        m_pDataProgress->setFormat( QString( "%p% %1 AP" ).arg( qsValue.right( 2 ).toUpper() ) );
        qsUrl = qsRoot + "openaip_" + m_mapItAirports.value() + ".aip";
        qsFile = qsFileRoot + m_mapItAirports.value() + ".aip";
        m_url = qsUrl;
        m_pFile = new QFile( qsFile );

        // Something is wrong with the file path or permissions
        if( !m_pFile->open( QIODevice::WriteOnly) )
        {
            delete m_pFile;
            m_pFile = nullptr;
            m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
        }
        else
            startRequestAirports( m_url );
    }
}


// When download finished or canceled, this will be called
void SettingsDialog::httpDownloadFinishedAirspace()
{
    QString qsRoot( "http://skyfun.space/stratofier/" );
    QString qsFileRoot = settingsRoot() + "/space.skyfun.stratofier/";
    QString qsUrl;
    QString qsFile;
    QString qsValue;

    m_pFile->flush();
    m_pFile->close();

    m_pReply->deleteLater();
    m_pReply = nullptr;
    delete m_pFile;
    m_pFile = nullptr;
    m_iTally++;

    m_pDataProgress->setValue( m_iTally );

    if( !m_mapItAirspaces.hasNext() )
    {
        if( m_iTally >= m_settings.listAirspaces.count() )
        {
            m_pDataProgress->hide();
            m_pGetButton->setIcon( QIcon( ":/icons/resources/OK.png" ) );
        }
        m_pManager = nullptr;
    }
    else
    {
        if( !nextCountryAirspace() )
        {
            if( m_iTally >= m_settings.listAirspaces.count() )
            {
                m_pDataProgress->hide();
                m_pGetButton->setIcon( QIcon( ":/icons/resources/OK.png" ) );
            }
            m_pManager = nullptr;
            return;
        }
        m_pDataProgress->setValue( 0 );
        qsValue = m_mapItAirspaces.value();
        m_pDataProgress->setFormat( QString( "%p% %1 AS" ).arg( qsValue.right( 2 ).toUpper() ) );
        qsUrl = qsRoot + "openaip_" + m_mapItAirspaces.value() + ".aip";
        qsFile = qsFileRoot + m_mapItAirspaces.value() + ".aip";
        m_url = qsUrl;
        m_pFile = new QFile( qsFile );

        // Something is wrong with the file path or permissions
        if( !m_pFile->open( QIODevice::WriteOnly) )
        {
            delete m_pFile;
            m_pFile = nullptr;
            m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );
        }
        else
            startRequestAirspace( m_url );
    }
}


// This will be called when download button is clicked
void SettingsDialog::startRequestAirports( QUrl url )
{
    m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );

    m_pReply = m_pManager->get( QNetworkRequest( url ) );

    connect( m_pReply, SIGNAL( readyRead() ), this, SLOT( httpReadyRead() ) );
    connect( m_pReply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( updateDownloadProgressAirports( qint64, qint64 ) ) );
    connect( m_pReply, SIGNAL( finished() ), this, SLOT( httpDownloadFinishedAirports() ) );
}


// This will be called when download button is clicked
void SettingsDialog::startRequestAirspace( QUrl url )
{
    m_pGetButton->setIcon( QIcon( ":/icons/resources/Cancel.png" ) );

    m_pReply = m_pManager->get( QNetworkRequest( url ) );

    connect( m_pReply, SIGNAL( readyRead() ), this, SLOT( httpReadyRead() ) );
    connect( m_pReply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( updateDownloadProgressAirspace( qint64, qint64 ) ) );
    connect( m_pReply, SIGNAL( finished() ), this, SLOT( httpDownloadFinishedAirspace() ) );
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

    m_pSwitchableButton->setIcon( m_settings.bSwitchableTanks ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );
}


void SettingsDialog::enableBT()
{
    m_settings.bEnableBT = (!m_settings.bEnableBT);

    g_pSet->setValue( "EnableBluetooth", m_settings.bSwitchableTanks );
    g_pSet->sync();

    m_pEnableBTButton->setIcon( m_settings.bEnableBT ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );
}


void SettingsDialog::selCountries()
{
    CountryDialog dlg( this, m_pC );

    // This geometry works for both orientations since dW is actually half width in landscape
    dlg.setGeometry( 0, 0, static_cast<int>( m_pC->dW ), static_cast<int>( m_pC->dH ) );
    if( dlg.exec() == QDialog::Accepted )
    {
        Canvas::CountryCodeAirports        countryAirport;
        Canvas::CountryCodeAirspace        countryAirspace;
        QVariantList                       countries;
        QList<Canvas::CountryCodeAirports> deleteCountriesAirports;
        QList<Canvas::CountryCodeAirspace> deleteCountriesAirspaces;
        QString                            qsFile;

        m_settings.listAirports = dlg.countriesAirports();
        m_settings.listAirspaces = dlg.countriesAirspaces();
        deleteCountriesAirports = dlg.deleteCountriesAirports();
        deleteCountriesAirspaces = dlg.deleteCountriesAirspaces();
        foreach( countryAirport, m_settings.listAirports )
            countries.append( static_cast<int>( countryAirport ) );
        foreach( countryAirport, deleteCountriesAirports )
        {
            qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapUrlsAirports[countryAirport] + ".aip";
            QFile::remove( qsFile );
        }
        g_pSet->setValue( "CountryAirports", countries );
        countries.clear();
        foreach( countryAirspace, m_settings.listAirspaces )
            countries.append( static_cast<int>( countryAirspace ) );
        foreach( countryAirspace, deleteCountriesAirspaces )
        {
            qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapUrlsAirspaces[countryAirspace] + ".aip";
            QFile::remove( qsFile );
        }
        g_pSet->setValue( "CountryAirspaces", countries );
        g_pSet->sync();
    }
}


bool SettingsDialog::nextCountryAirport()
{
    Canvas::CountryCodeAirports country;
    bool                        bFound = false;

    // Move the iterator to the next selected country
    while( m_mapItAirports.hasNext() )
    {
        m_mapItAirports.next();
        foreach( country, m_settings.listAirports )
        {
            if( m_mapItAirports.key() == country )
            {
                bFound = true;
                break;
            }
        }
        if( bFound )
            break;
    }

    return bFound;
}


bool SettingsDialog::nextCountryAirspace()
{
    Canvas::CountryCodeAirspace country;
    bool                        bFound = false;

    // Move the iterator to the next selected country
    while( m_mapItAirspaces.hasNext() )
    {
        m_mapItAirspaces.next();
        foreach( country, m_settings.listAirspaces )
        {
            if( m_mapItAirspaces.key() == country )
            {
                bFound = true;
                break;
            }
        }
        if( bFound )
            break;
    }

    return bFound;
}
