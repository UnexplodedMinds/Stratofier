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

    connect( m_pSwitchableButton, SIGNAL( clicked() ), this, SLOT( switchable() ) );

    connect( m_pGetDataButton, SIGNAL( clicked() ), this, SLOT( getMapData() ) );

    connect( m_pSelCountriesButton, SIGNAL( clicked() ), this, SLOT( selCountries() ) );

    m_pDataProgress->hide();

    QTimer::singleShot( 100, this, SLOT( init() ) );
}


SettingsDialog::~SettingsDialog()
{
    g_pSet->setValue( "CurrDataSet", m_settings.iCurrDataSet );

    g_pSet->setValue( "StratuxIP", m_pIPEdit->text().simplified().remove( ' ' ) );
    g_pSet->setValue( "OwnshipID", m_pOwnshipIDEdit->text().simplified().toUpper().remove( ' ' ) );

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
    m_settings.listCountries.clear();
    foreach( country, countries )
        m_settings.listCountries.append( static_cast<Canvas::CountryCode>( country.toInt() ) );
    g_pSet->beginGroup( "FuelTanks" );
    m_settings.bSwitchableTanks = (!g_pSet->value( "DualTanks" ).toBool());
    g_pSet->endGroup();
    switchable();

    m_pIPEdit->setText( m_settings.qsStratuxIP );
    m_pOwnshipIDEdit->setText( m_settings.qsOwnshipID );

    Builder::getStorage( &m_qsInternalStoragePath );

    storage();
}


void SettingsDialog::getMapData()
{
    QObject *pSender = sender();

    if( pSender == nullptr )
        return;

    if( m_settings.listCountries.count() == 0 )
        return;

    QString qsUrl, qsFile, qsType, qsValue;

    m_pManager = new QNetworkAccessManager( this );

    m_mapIt = m_mapUrls;
    nextCountry();

    qsUrl = "http://skyfun.space/stratofier/openaip_" + m_mapIt.value() + ".aip";
    qsValue = m_mapIt.value();
    if( qsValue.startsWith( "airports") )
        qsType = "AP";
    else
        qsType = "AS";
    m_pCountryProgress->setFormat( QString( "%p% [%1 %2]" ).arg( qsValue.right( 2 ).toUpper() ).arg( qsType ) );

    qsFile = settingsRoot() + "/space.skyfun.stratofier/" + m_mapIt.value() + ".aip";
    m_url = qsUrl;
    m_pFile = new QFile( qsFile );
    m_iTally = 0;

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
    QString qsType, qsValue;

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

        qsValue = m_mapIt.value();
        if( qsValue.startsWith( "airports") )
            qsType = "AP";
        else
            qsType = "AS";
        m_pCountryProgress->setFormat( QString( "%p% [%1 %2]" ).arg( qsValue.right( 2 ).toUpper() ).arg( qsType ) );
        qsUrl = qsRoot + "openaip_" + m_mapIt.value() + ".aip";
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

    m_pSwitchableButton->setIcon( m_settings.bSwitchableTanks ? QIcon( ":/icons/resources/on.png" ) : QIcon( ":/icons/resources/off.png" ) );
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
