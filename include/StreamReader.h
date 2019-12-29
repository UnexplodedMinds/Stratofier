/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __STREAMREADER_H__
#define __STREAMREADER_H__

#include <QObject>
#include <QWebSocket>
#include <QBluetoothServiceInfo>
#include <QBluetoothServiceDiscoveryAgent>

#include "StratuxStreams.h"
#include "Canvas.h"


class QCoreApplication;
class QBluetoothSocket;
class QBluetoothServiceInfo;


class StreamReader : public QObject
{
    Q_OBJECT

public:
    explicit StreamReader( QObject *parent, const QString &qsIP, bool bEnableBT, bool bUseBTBaro );
    ~StreamReader();

    void connectStreams();
    void disconnectStreams();
    bool isConnected() { return m_bConnected; }

    static void initTraffic( StratuxTraffic &traffic );
    static void initSituation( StratuxSituation &situation );
    static void initStatus( StratuxStatus &status );

    void setUnits( Canvas::Units eUnits ) { m_eUnits = eUnits; }
    void setBaroPress( double dBaro );

private:
    double unitsMult();

    bool          m_bHaveMyPos;
    bool          m_bAHRSStatus;
    bool          m_bStratuxStatus;
    bool          m_bGPSStatus;
    bool          m_bTrafficStatus;
    QWebSocket    m_stratuxSituation;
    QWebSocket    m_stratuxTraffic;
    QWebSocket    m_stratuxStatus;
    QWebSocket    m_stratuxWeather;
    double        m_dMyLat;
    double        m_dMyLong;
    bool          m_bConnected;
    bool          m_bBTConnected;
    QString       m_qsIP;
    Canvas::Units m_eUnits;

    QBluetoothServiceDiscoveryAgent *m_pBTserviceDiscoverer;
    QBluetoothSocket                *m_pBTsocket;
    QByteArray                       m_btBuffer;
    bool                             m_bHaveBTtelemetry;
    BluetoothTelemetry               m_btTelem;
    double                           m_dBaroPress;
    bool                             m_bUseBTBaro;

private slots:
    void situationUpdate( const QString &qsMessage );
    void trafficUpdate( const QString &qsMessage );
    void statusUpdate( const QString &qsMessage );
    void stratuxConnected();
    void stratuxDisconnected();

    void btServiceDiscovered( const QBluetoothServiceInfo &devInfo );
    void btDisconnected();
    void btReadData();
    void btServiceDiscoveryFinished();
    void btServiceDiscoveryError( QBluetoothServiceDiscoveryAgent::Error err );
    void reconnectBT();

signals:
    void newSituation( StratuxSituation );
    void newTraffic( int, StratuxTraffic );     // ICAO, Rest of traffic struct
    void newStatus( bool, bool, bool, bool );   // Stratux available, AHRS available, GPS available, Traffic available

    void newBTStatus( bool );                   // HC-06 is sending data
};

#endif // __STREAMREADER_H__
