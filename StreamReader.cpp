/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QtDebug>
#include <QApplication>
#include <QUrl>
#include <QStringList>
#include <QColor>
#include <QPalette>
#include <QNetworkInterface>
#include <QTimer>
#include <QSettings>
#include <QNetworkDatagram>

#include <math.h>

#include "StreamReader.h"
#include "TrafficMath.h"
#include "StratofierDefs.h"


extern QSettings *g_pSet;


StreamReader::StreamReader( const QString &qsIP )
    : QObject( nullptr ),
      m_bHaveMyPos( false ),
      m_bAHRSStatus( false ),
      m_bStratuxStatus( false ),
      m_bGPSStatus( false ),
      m_bConnected( false ),
      m_qsIP( qsIP ),
      m_eUnits( Canvas::Knots ),
      m_bHaveWTtelem( false ),
      m_dBaroPress( 29.92 ),
      m_wtHost(),
      m_lastPacketDateTime( QDateTime::currentDateTime() ),
      m_iMagCalIndex( 0 ),
      m_dRollRef( 0.0 ),
      m_dPitchRef( 0.0 ),
      m_dRawRoll( 0.0 ),
      m_dRawPitch( 0.0 ),
      m_bReported( false ),
      m_dAirspeedCal( 1.0 )
{
    m_dPitchRef = g_pSet->value( "PitchRef", 0.0 ).toDouble();
    m_dRollRef = g_pSet->value( "RollRef", 0.0 ).toDouble();
    m_dAirspeedCal = g_pSet->value( "AirspeedCal", 1.0 ).toDouble();

    // If one connects there's a 99.99% chance they all will so just use the status
    connect( &m_stratuxStatus, SIGNAL( connected() ), this, SLOT( stratuxConnected() ) );
    connect( &m_stratuxStatus, SIGNAL( connected() ), this, SLOT( stratuxDisconnected() ) );

    connect( &m_wtSocket, SIGNAL( readyRead() ), this, SLOT( wtDataAvail() ) );
    connect( &m_wtSocket, SIGNAL( disconnected() ), this, SLOT( wtDisconnected() ) );
    m_wtSocket.bind( QHostAddress( "192.168.10.255" ), 45678 );

    // Re-transmit the currently set barometric pressure every ten seconds in case the WingThing restarted
    startTimer( 10000 );
}


// This will likely not get called but it needs to be here just in case
void StreamReader::wtDisconnected()
{
    m_bHaveWTtelem = false;
    emit newWTStatus( false );
}


void StreamReader::wtDataAvail()
{
    QNetworkDatagram sensorData = m_wtSocket.receiveDatagram();

    // Format coming from the WingThing is:
    // <Firmware Version>,<Airspeed>,<Altitude>,<Temp>,<Mag X>,<Mag Y>,<Mag Z>,<Orient X>,<Orient Y>,<Orient Z>,<Accel X>,<Accel Y>,<Accel Z>
    QString               qsBuffer( sensorData.data() );
    QStringList           qslFields = qsBuffer.split( ',' );
    double                dAS;
    QPair<double, double> orient;

    // Firmware Version (e.g. 213), Airspeed, Altitude, Heading, Baro Press (placeholder), Temp, Mag X, Mag Y, Mag Z
    if( qslFields.count() == 13 )
    {
        m_wtTelem.qsFWversion = qslFields.first();

        dAS = qslFields.at( 1 ).toDouble() / 8192.0 * 173.7952 * m_dAirspeedCal;    // Ratio of sensor value over sensor maximum times the rated maximum speed times a calibration factor
        m_airspeedSamples.append( dAS );

        m_wtTelem.dAltitude = qslFields.at( 2 ).toDouble();
        m_wtTelem.dTemp = qslFields.at( 3 ).toDouble();
        m_wtTelem.dMagX = qslFields.at( 4 ).toDouble();
        m_wtTelem.dMagY = qslFields.at( 5 ).toDouble();
        m_wtTelem.dMagZ = qslFields.at( 6 ).toDouble();
        calcHeading( m_wtTelem.dMagX, m_wtTelem.dMagY, m_wtTelem.dMagZ );   // m_wtTelem.dHeading will be set in the function
        m_wtTelem.dOrientX = qslFields.at( 7 ).toDouble();                  // Yaw  0 to 360 (not currently used)

        m_dRawPitch = qslFields.at( 8 ).toDouble() / 4.0;
        m_dRawRoll = qslFields.at( 9 ).toDouble();
        m_pitchSamples.append( m_dRawPitch );
        m_rollSamples.append( m_dRawRoll );

        m_wtTelem.dAccelX = qslFields.at( 10 ).toDouble();
        m_wtTelem.dAccelY = qslFields.at( 11 ).toDouble();
        m_wtTelem.dAccelZ = qslFields.last().toDouble();
        m_bHaveWTtelem = true;
        m_wtHost = sensorData.senderAddress();  // So we know where to send data TO
        m_lastPacketDateTime = QDateTime::currentDateTime();

        m_wtLast = m_wtTelem;

        if( m_airspeedSamples.count() == 4 )
        {
            dAS = (m_airspeedSamples.first() + m_airspeedSamples.at( 1 ) + m_airspeedSamples.at( 2 ) + m_airspeedSamples.last()) / 4.0;
            m_airspeedSamples.removeFirst();
            m_dRawRoll = (m_rollSamples.first() + m_rollSamples.at( 1 ) + m_rollSamples.at( 2 ) + m_rollSamples.last()) / 4.0;
            m_rollSamples.removeFirst();
            m_dRawPitch = (m_pitchSamples.first() + m_pitchSamples.at( 1 ) + m_pitchSamples.at( 2 ) + m_pitchSamples.last()) / 4.0;
            m_pitchSamples.removeFirst();
        }
        m_wtTelem.dAirspeed = dAS;
        m_wtTelem.dOrientY = m_dRawPitch - m_dPitchRef;     // Pitch -90 to 90, negative is down
        m_wtTelem.dOrientZ = m_dRawRoll - m_dRollRef;       // Roll -90 to 90, negative is left

        // Don't rapid fire this signal
        if( !m_bReported )
        {
            m_bReported = true;
            emit newWTStatus( true );
        }
    }
}


StreamReader::~StreamReader()
{
}


void StreamReader::calcHeading( double dX, double dY, double dZ )
{
    Q_UNUSED( dZ )

    double dHead = 360.0 - (atan2( dY, dX ) * 57.29578);

    dHead += 90.0;
    if( dHead < 0 )
        dHead += 360.0;
    else if( dHead > 360.0 )
        dHead -= 360.0;

    m_headSamples.append( dHead );
    if( m_headSamples.count() == 4 )
    {
        dHead = (m_headSamples.first() + m_headSamples.at( 1 ) + m_headSamples.at( 2 ) + m_headSamples.last()) / 4.0;
        m_headSamples.removeFirst();
    }

    m_wtTelem.dHeading = dHead;
}


void StreamReader::timerEvent( QTimerEvent* )
{
    if( m_bHaveWTtelem && (m_lastPacketDateTime.secsTo( QDateTime::currentDateTime() ) > 10) )
    {
        m_bHaveWTtelem = false;
        m_bReported = false;
        emit newWTStatus( false );
    }

    setBaroPress( m_dBaroPress );
}


void StreamReader::setBaroPress( double dBaro )
{
    if( !m_wtHost.isNull() )
    {
        m_dBaroPress = dBaro;

        QString qsData = QString( "%1" ).arg( m_dBaroPress, 5, 'f', 2, QChar( '0' ) );

        m_wtSocket.writeDatagram( qsData.toLatin1(), m_wtHost, 45678 );
    }
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
            situation.dGPSGroundSpeed = dVal * unitsMult();
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

    // Replace Stratux telemetry with actual air data from the sensor sending UDP data
    if( m_bHaveWTtelem )
    {
        situation.bHaveWTData = true;
        situation.dTAS = m_wtTelem.dAirspeed;
        situation.dAHRSMagHeading = m_wtTelem.dHeading;
        situation.dBaroPressAlt = m_wtTelem.dAltitude;
        situation.dBaroTemp = m_wtTelem.dTemp;
        situation.dAHRSSlipSkid = m_wtTelem.dAccelY;
        situation.dAHRSroll = m_wtTelem.dOrientZ;
        situation.dAHRSpitch = m_wtTelem.dOrientY;
        situation.qsBADASPversion = m_wtTelem.qsFWversion;
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
            traffic.dSpeed = dVal * unitsMult();
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
        emit newTraffic( traffic );
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
    situation.bHaveWTData = false;
    situation.dTAS = 0.0;
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


double StreamReader::unitsMult()
{
    double dUnitsMult = 1.0;

    switch( m_eUnits )
    {
        case Canvas::MPH:
            dUnitsMult = KnotsToMPH;
            break;
        case Canvas::Knots:
            dUnitsMult = 1.0;
            break;
        case Canvas::KPH:
            dUnitsMult = KnotsToKPH;
            break;
    }

    return dUnitsMult;
}


void StreamReader::snapshotOrientation()
{
    m_dRollRef = m_dRawRoll;
    m_dPitchRef = m_dRawPitch;
    g_pSet->setValue( "PitchRef", m_dPitchRef );
    g_pSet->setValue( "RollRef", m_dRollRef );
}

