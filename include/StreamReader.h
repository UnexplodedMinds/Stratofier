/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __STREAMREADER_H__
#define __STREAMREADER_H__

#include <QObject>
#include <QWebSocket>
#include <QUdpSocket>
#include <QPair>

#include "StratuxStreams.h"
#include "Canvas.h"


class QCoreApplication;


class StreamReader : public QObject
{
    Q_OBJECT

public:
    explicit StreamReader( const QString &qsIP );
    ~StreamReader();

    bool isConnected() { return m_bConnected; }

    static void initTraffic( StratuxTraffic &traffic );
    static void initSituation( StratuxSituation &situation );
    static void initStatus( StratuxStatus &status );

    void setUnits( Canvas::Units eUnits ) { m_eUnits = eUnits; }
    void snapshotOrientation();

    void setAirspeedCal( double dCal ) { m_dAirspeedCal = dCal; }

public slots:
    void connectStreams();
    void disconnectStreams();

private:
    double unitsMult();
    void   calcHeading( double dX, double dY, double dZ );

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
    QString       m_qsIP;
    Canvas::Units m_eUnits;
    QUdpSocket    m_wtSocket;

    double        m_dBaroPress;
    int           m_iMagCalIndex;
    QList<double> m_headSamples;
    QList<double> m_airspeedSamples;
    QList<double> m_pitchSamples;
    QList<double> m_rollSamples;

    double        m_dRollRef, m_dPitchRef, m_dRawRoll, m_dRawPitch;
    double        m_dAirspeedCal;

private slots:
    void situationUpdate( const QString &qsMessage );
    void trafficUpdate( const QString &qsMessage );
    void statusUpdate( const QString &qsMessage );
    void stratuxConnected();
    void stratuxDisconnected();

signals:
    void newSituation( StratuxSituation );
    void newTraffic( StratuxTraffic );          // ICAO, Rest of traffic struct
    void newStatus( bool, bool, bool, bool );   // Stratux available, AHRS available, GPS available, Traffic available
};

#endif // __STREAMREADER_H__
