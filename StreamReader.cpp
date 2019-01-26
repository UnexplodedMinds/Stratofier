/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QtDebug>
#include <QApplication>
#include <QUrl>
#include <QStringList>
#include <QColor>
#include <QPalette>
#include <QNetworkInterface>

#include <math.h>

#include "StreamReader.h"
#include "TrafficMath.h"


extern bool g_bEmulated;


StreamReader::StreamReader( QObject *parent, const QString &qsIP )
    : QObject( parent ),
      m_bHaveMyPos( false ),
      m_bAHRSStatus( false ),
      m_bStratuxStatus( false ),
      m_bGPSStatus( false ),
      m_bConnected( false ),
      m_qsIP( qsIP )
{
    // If one connects there's a 99.99% chance they all will so just use the status
    connect( &m_stratuxStatus, SIGNAL( connected() ), this, SLOT( stratuxConnected() ) );
    connect( &m_stratuxStatus, SIGNAL( connected() ), this, SLOT( stratuxDisconnected() ) );
}


StreamReader::~StreamReader()
{
}


// Open the websocket URLs from the Stratux
void StreamReader::connectStreams()
{
    // Open the streams
    m_stratuxSituation.open( QUrl( QString( "ws://%1/situation" ).arg( m_qsIP ) ) );
    m_stratuxTraffic.open( QUrl( QString( "ws://%1/traffic" ).arg( m_qsIP ) ) );
    m_stratuxStatus.open( QUrl( QString( "ws://%1/status" ).arg( m_qsIP ) ) );
    connect( &m_stratuxTraffic, SIGNAL( textMessageReceived( const QString& ) ), this, SLOT( trafficUpdate( const QString& ) ) );
    connect( &m_stratuxSituation, SIGNAL( textMessageReceived( const QString& ) ), this, SLOT( situationUpdate( const QString& ) ) );
    connect( &m_stratuxStatus, SIGNAL( textMessageReceived( const QString& ) ), this, SLOT( statusUpdate( const QString& ) ) );
}


// Close all the streams
void StreamReader::disconnectStreams()
{
    disconnect( &m_stratuxTraffic, SIGNAL( textMessageReceived( const QString& ) ), this, SLOT( trafficUpdate( const QString& ) ) );
    disconnect( &m_stratuxSituation, SIGNAL( textMessageReceived( const QString& ) ), this, SLOT( situationUpdate( const QString& ) ) );
    disconnect( &m_stratuxStatus, SIGNAL( textMessageReceived( const QString& ) ), this, SLOT( statusUpdate( const QString& ) ) );
    m_stratuxSituation.close();
    m_stratuxTraffic.close();
    m_stratuxStatus.close();
    emit newStatus( false, false, false, false );
}


// Updates from the situation stream
// String is received from stratux and the situation struct filled in
void StreamReader::situationUpdate( const QString &qsMessage )
{
    QStringList      qslFields( qsMessage.split( ',' ) );
    QString          qsField;
    StratuxSituation situation;
    QStringList      qslThisField;
    QString          qsTag;
    QString          qsVal;
    double           dVal;
    int              iVal;

    initSituation( situation );

    foreach( qsField, qslFields )
    {
        qslThisField = qsField.split( "\":" );
        if( qslThisField.count() != 2 )
            continue;

        // Tag and value - see https://github.com/cyoung/stratux/blob/master/notes/app-vendor-integration.md
        qsTag = qslThisField.first().remove( 0, 1 ).trimmed().remove( '\"' );
        qsVal = qslThisField.last().trimmed().remove( '\"' ).remove( '}' );
        dVal = qsVal.toDouble();
        iVal = qsVal.toInt();

        if( qsTag == "GPSLastFixSinceMidnightUTC" )
            situation.dLastGPSFixSinceMidnight = dVal;
        else if( qsTag == "GPSLatitude" )
            situation.dGPSlat = dVal;
        else if( qsTag == "GPSLongitude" )
            situation.dGPSlong = dVal;
        else if( qsTag == "GPSFixQuality" )
            situation.iGPSFixQuality = iVal;
        else if( qsTag == "GPSHeightAboveEllipsoid" )
            situation.dGPSHeightAboveEllipsoid = dVal;
        else if( qsTag == "GPSGeoidSep" )
            situation.dGPSGeoidSep = dVal;
        else if( qsTag == "GPSSatellites" )
            situation.iGPSSats = iVal;
        else if( qsTag == "GPSSatellitesTracked" )
            situation.iGPSSatsTracked = iVal;
        else if( qsTag == "GPSSatellitesSeen" )
            situation.iGPSSatsSeen = iVal;
        else if( qsTag == "GPSHorizontalAccuracy" )
            situation.dGPSHorizAccuracy = dVal;
        else if( qsTag == "GPSNACp" )
            situation.iGPSNACp = iVal;
        else if( qsTag == "GPSAltitudeMSL" )
            situation.dGPSAltMSL = fabs( iVal );
        else if( qsTag == "GPSVerticalAccuracy" )
            situation.dGPSVertAccuracy = dVal;
        else if( qsTag == "GPSVerticalSpeed" )
            situation.dGPSVertSpeed = dVal;
        else if( qsTag == "GPSLastFixLocalTime" )
            situation.lastGPSFixTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "GPSTrueCourse" )
            situation.dGPSTrueCourse = dVal;
        else if( qsTag == "GPSTurnRate" )
            situation.dGPSTurnRate = dVal;
        else if( qsTag == "GPSGroundSpeed" )
            situation.dGPSGroundSpeed = dVal;
        else if( qsTag == "GPSLastGroundTrackTime" )
            situation.lastGPSGroundTrackTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "GPSTime" )
            situation.gpsDateTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "GPSLastGPSTimeStratuxTime" )
            situation.lastGPSTimeStratuxTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "GPSLastValidNMEAMessageTime" )
            situation.lastValidNMEAMessageTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "GPSLastValidNMEAMessage" )
            situation.qsLastNMEAMsg = qsVal;
        else if( qsTag == "GPSPositionSampleRate" )
            situation.iGPSPosSampleRate = iVal;
        else if( qsTag == "BaroTemperature" )
            situation.dBaroTemp = dVal;
        else if( qsTag == "BaroPressureAltitude" )
            situation.dBaroPressAlt = fabs( dVal );
        else if( qsTag == "BaroVerticalSpeed" )
            situation.dBaroVertSpeed = dVal;
        else if( qsTag == "BaroLastMeasurementTime" )
            situation.lastBaroMeasTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "AHRSPitch" )
            situation.dAHRSpitch = dVal;
        else if( qsTag == "AHRSRoll" )
            situation.dAHRSroll = dVal;
        else if( qsTag == "AHRSGyroHeading" )
            situation.dAHRSGyroHeading = dVal;
        else if( qsTag == "AHRSMagHeading" )
            situation.dAHRSMagHeading = dVal;
        else if( qsTag == "AHRSSlipSkid" )
            situation.dAHRSSlipSkid = dVal;
        else if( qsTag == "AHRSTurnRate" )
            situation.dAHRSTurnRate = dVal;
        else if( qsTag == "AHRSGLoad" )
            situation.dAHRSGLoad = dVal;
        else if( qsTag == "AHRSGLoadMin" )
            situation.dAHRSGLoadMin = dVal;
        else if( qsTag == "AHRSGLoadMax" )
            situation.dAHRSGLoadMax = dVal;
        else if( qsTag == "AHRSLastAttitudeTime" )
            situation.lastAHRSAttTime.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "AHRSStatus" )
            situation.iAHRSStatus = iVal;
    }

    while( situation.dAHRSGyroHeading > 360 )
        situation.dAHRSGyroHeading -= 360.0;
    while( situation.dAHRSMagHeading > 360.0 )
        situation.dAHRSMagHeading -= 360.0;

    if( (situation.dGPSlat != 0.0) && (situation.dGPSlong != 0.0) )
    {
        m_bHaveMyPos = true;
        m_dMyLat = situation.dGPSlat;
        m_dMyLong = situation.dGPSlong;
    }
    else
    {
        m_bHaveMyPos = false;
        m_dMyLat = 0.0;
        m_dMyLong = 0.0;
    }

    m_bAHRSStatus = (situation.iAHRSStatus > 0);

    emit newSituation( situation );
}


// Updates from the traffic stream
void StreamReader::trafficUpdate( const QString &qsMessage )
{
    QStringList    qslFields( qsMessage.split( ',' ) );
    QString        qsField;
    StratuxTraffic traffic;
    QStringList    qslThisField;
    QString        qsTag;
    QString        qsVal;
    double         dVal;
    int            iVal;
    bool           bVal;
    int            iICAO = 0;

    traffic.dLat = 0.0;
    traffic.dLong = 0.0;
    traffic.lastActualReport = QDateTime::currentDateTime();

    initTraffic( traffic );

    foreach( qsField, qslFields )
    {
        qslThisField = qsField.split( "\":" );
        if( qslThisField.count() != 2 )
            continue;

        // Tag and value - see https://github.com/cyoung/stratux/blob/master/notes/app-vendor-integration.md
        qsTag = qslThisField.first().remove( 0, 1 ).trimmed().remove( '\"' );
        qsVal = qslThisField.last().trimmed().remove( '\"' ).remove( '}' );
        dVal = qsVal.toDouble();
        iVal = qsVal.toInt();
        bVal = (qsVal == "true");

        if( qsTag == "Icao_addr" )
            iICAO = iVal;   // Note this is not part of the struct
        else if( qsTag == "OnGround" )
            traffic.bOnGround = bVal;
        else if( qsTag == "Lat" )
            traffic.dLat = dVal;
        else if( qsTag == "Lng" )
            traffic.dLong = dVal;
        else if( qsTag == "Position_valid" )
            traffic.bPosValid = bVal;
        else if( qsTag == "Alt" )
            traffic.dAlt = dVal;
        else if( qsTag == "Track" )
            traffic.dTrack = dVal;
        else if( qsTag == "Speed" )
            traffic.dSpeed = dVal;
        else if( qsTag == "Vvel" )
            traffic.dVertSpeed = dVal;
        else if( qsTag == "Tail" )
            traffic.qsTail = qsVal;
        else if( qsTag == "Last_seen" )
            traffic.lastSeen.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "Last_source" )
            traffic.iLastSource = iVal;
        else if( qsTag == "Reg" )
            traffic.qsReg = qsVal;
        else if( qsTag == "SignalLevel" )
            traffic.dSigLevel = dVal;
        else if( qsTag == "Squawk" )
            traffic.iSquawk = iVal;
        else if( qsTag == "Timestamp" )
            traffic.timestamp.fromString( qsVal, Qt::ISODate );
        else if( qsTag == "Last_source" )
            traffic.iLastSource = iVal;
        else if( qsTag == "Bearing" )
            traffic.dBearing = dVal;
        else if( qsTag == "Distance" )
            traffic.dDist = dVal * 0.000539957;  // Meters to Nautical Miles
        else if( qsTag == "Age" )
            traffic.dAge = dVal;
    }

    // If we know where we are, figure out where they are
    if( traffic.bPosValid && m_bHaveMyPos )
    {
        // Modified haversine algorithm for calculating distance and bearing
        BearingDist bd = TrafficMath::haversine( m_dMyLat, m_dMyLong, traffic.dLat, traffic.dLong );

        traffic.dBearing = bd.dBearing;
        traffic.dDist = bd.dDistance;
        traffic.bHasADSB = true;
    }
    else
        traffic.bHasADSB = false;

    if( iICAO > 0 )
        emit newTraffic( iICAO, traffic );
}


// Updates from the status stream
void StreamReader::statusUpdate( const QString &qsMessage )
{
    QStringList   qslFields( qsMessage.split( ',' ) );
    QString       qsField;
    QStringList   qslThisField;
    QString       qsTag;
    QString       qsVal;
    int           iVal;
    bool          bVal;
    StratuxStatus status;

    initStatus( status );

    foreach( qsField, qslFields )
    {
        qslThisField = qsField.split( "\":" );
        if( qslThisField.count() != 2 )
            continue;

        // Tag and value - see https://github.com/cyoung/stratux/blob/master/notes/app-vendor-integration.md
        qsTag = qslThisField.first().remove( 0, 1 ).trimmed().remove( '\"' );
        qsVal = qslThisField.last().trimmed().remove( '\"' ).remove( '}' );
        iVal = qsVal.toInt();
        bVal = (qsVal == "true");

        if( qsTag == "UAT_traffic_targets_tracking" )
            status.iUATTrafficTracking = iVal;
        else if( qsTag == "ES_traffic_targets_tracking" )
            status.iESTrafficTracking = iVal;
        else if( qsTag == "GPS_satellites_locked" )
            status.iGPSSatsLocked = iVal;
        else if( qsTag == "GPS_connected" )
            status.bGPSConnected = bVal;
    }

    m_bStratuxStatus = true;    // If this signal fired then we're at least talking to the Stratux
    m_bGPSStatus = (status.bGPSConnected && m_bHaveMyPos);
    m_bTrafficStatus = ((status.iUATTrafficTracking > 0) || (status.iESTrafficTracking > 0));

    emit newStatus( m_bStratuxStatus, m_bAHRSStatus, m_bGPSStatus, m_bTrafficStatus );
}


// Initialize the traffic struct
void StreamReader::initTraffic( StratuxTraffic &traffic )
{
    traffic.bOnGround = false;
    traffic.dLat = 0.0;
    traffic.dLong = 0.0;
    traffic.bPosValid = false;
    traffic.dAlt = 0.0;
    traffic.dTrack = 0.0;
    traffic.dSpeed = 0.0;
    traffic.dVertSpeed = false;
    traffic.qsTail = "N/A";
    traffic.lastSeen.setDate( QDate( 2000, 1, 1 ) );
    traffic.lastSeen.setTime( QTime( 0, 0, 0 ) );
    traffic.iLastSource = 0;
    traffic.qsReg = "N/A";
    traffic.dSigLevel = 0.0;
    traffic.iSquawk = 1200;
    traffic.timestamp.setDate( QDate( 2000, 1, 1 ) );
    traffic.timestamp.setTime( QTime( 0, 0, 0 ) );
    traffic.iLastSource = 0;
    traffic.dBearing = 0.0;
    traffic.dDist = 0.0;
    traffic.dAge = 3600.0;
    traffic.bHasADSB = false;
}


// Initialize the status struct
void StreamReader::initStatus( StratuxStatus &status )
{
    status.bGPSConnected = false;
    status.iESTrafficTracking = 0;
    status.iGPSSatsLocked = 0;
    status.iUATTrafficTracking = 0;
}


// Initialize the situation struct
void StreamReader::initSituation( StratuxSituation &situation )
{
    QDateTime nullDateTime( QDate( 2000, 1, 1 ), QTime( 0, 0, 0 ) );

    situation.dLastGPSFixSinceMidnight = 0.0;
    situation.dGPSlat = 0.0;
    situation.dGPSlong = 0.0;
    situation.iGPSFixQuality = 0;
    situation.dGPSHeightAboveEllipsoid = 0.0;
    situation.dGPSGeoidSep = 0.0;
    situation.iGPSSats = 0;
    situation.iGPSSatsTracked = 0;
    situation.iGPSSatsSeen = 0;
    situation.dGPSHorizAccuracy = 0.0;
    situation.iGPSNACp = 0;
    situation.dGPSAltMSL = 0;
    situation.dGPSVertAccuracy = 0.0;
    situation.dGPSVertSpeed = 0.0;
    situation.lastGPSFixTime = nullDateTime;
    situation.dGPSTrueCourse = 0.0;
    situation.dGPSTurnRate = 0.0;
    situation.dGPSGroundSpeed = 0.0;
    situation.lastGPSGroundTrackTime = nullDateTime;
    situation.gpsDateTime = nullDateTime;
    situation.lastGPSTimeStratuxTime = nullDateTime;
    situation.lastValidNMEAMessageTime = nullDateTime;
    situation.qsLastNMEAMsg = "";
    situation.iGPSPosSampleRate = 0;
    situation.dBaroTemp = 0.0;
    situation.dBaroPressAlt = 0.0;
    situation.dBaroVertSpeed = 0.0;
    situation.lastBaroMeasTime = nullDateTime;
    situation.dAHRSpitch = 0.0;
    situation.dAHRSroll = 0.0;
    situation.dAHRSGyroHeading = 0.0;
    situation.dAHRSMagHeading = 0.0;
    situation.dAHRSSlipSkid = 0.0;
    situation.dAHRSTurnRate = 0.0;
    situation.dAHRSGLoad = 1.0;
    situation.dAHRSGLoadMin = 1.0;
    situation.dAHRSGLoadMax = 1.0;
    situation.lastAHRSAttTime = nullDateTime;
    situation.iAHRSStatus = 0;
}


void StreamReader::stratuxConnected()
{
    m_bConnected = true;
}


void StreamReader::stratuxDisconnected()
{
    emit newStatus( false, false, false, false );
    m_bConnected = false;
}

