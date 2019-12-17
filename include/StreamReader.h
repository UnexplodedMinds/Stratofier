/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __STREAMREADER_H__
#define __STREAMREADER_H__

#include <QObject>
#include <QWebSocket>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>

#include "StratuxStreams.h"
#include "Canvas.h"


class QCoreApplication;


class StreamReader : public QObject
{
    Q_OBJECT

public:
    explicit StreamReader( QObject *parent, const QString &qsIP );
    ~StreamReader();

    void connectStreams();
    void disconnectStreams();
    bool isConnected() { return m_bConnected; }

    static void initTraffic( StratuxTraffic &traffic );
    static void initSituation( StratuxSituation &situation );
    static void initStatus( StratuxStatus &status );

    void setIP( const QString &qsIP );

    void setUnits( Canvas::Units eUnits ) { m_eUnits = eUnits; }

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

    QBluetoothDeviceDiscoveryAgent *m_pDiscoveryAgent;

private slots:
    void situationUpdate( const QString &qsMessage );
    void trafficUpdate( const QString &qsMessage );
    void statusUpdate( const QString &qsMessage );
    void stratuxConnected();
    void stratuxDisconnected();

    void deviceDiscovered( const QBluetoothDeviceInfo &device );
    void stopLookingForBT();

signals:
    void newSituation( StratuxSituation );
    void newTraffic( int, StratuxTraffic );     // ICAO, Rest of traffic struct
    void newStatus( bool, bool, bool, bool );   // Stratux available, AHRS available, GPS available, Traffic available

    void newBTStatus( bool );                   // HC-06 discovered
};

#endif // __STREAMREADER_H__
