/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __AHRSMAINWIN_H__
#define __AHRSMAINWIN_H__

#include <QMainWindow>
#include <QDateTime>
#include <QUdpSocket>
#include <QFile>

#include "ui_AHRSMainWin.h"
#include "Canvas.h"
#include "StreamReader.h"


class StreamReader;
class AHRSCanvas;
class MenuDialog;


class AHRSMainWin : public QMainWindow, public Ui::AHRSMainWin
{
    Q_OBJECT

public:
    explicit AHRSMainWin( const QString &qsIP, bool bPortrait, StreamReader *pStream );
    ~AHRSMainWin();

    bool          menuActive() { return (m_pMenuDialog != nullptr); }
    void          restartTimer();
    void          stopTimer();
    AHRSCanvas   *disp() { return m_pAHRSDisp; }
    StreamReader *streamReader() { return m_pStratuxStream; }
    bool          isRecording() { return m_bRecording; }
    void          appendTrackPt( TrackPoint tp );

public slots:
    void menu();
    void changeTimer();
    void init();
    void splashOff();

protected:
    void keyReleaseEvent( QKeyEvent *pEvent );
    void timerEvent( QTimerEvent *pEvent );

private:
    void emptyHttpPost( const QString &qsToken );

    StreamReader *m_pStratuxStream;
    bool          m_bStartup;
    QDateTime     m_lastStatusUpdate;
    MenuDialog   *m_pMenuDialog;
    QString       m_qsIP;
    bool          m_bPortrait;
    QDateTime     m_timerStart;
    int           m_iTimerSeconds;
    bool          m_bTimerActive;
    int           m_iReconnectTimer;
    int           m_iTimerTimer;
    bool          m_bRecording;

    QList<TrackPoint> m_Track;
    QUdpSocket       *m_pHostListener;
    QHostAddress      m_hostAddress;
    QTcpSocket       *m_pSender;
    qint64            m_iSent;
    qint64            m_iBufferSize;

private slots:
    void statusUpdate( bool bStratux, bool bAHRS, bool bGPS, bool bTraffic );
    void WTUpdate( bool bValid );
    void resetLevel();
    void resetGMeter();
    void upgradeStratofier();
    void shutdownStratux();
    void shutdownStratofier();
    void orient( Qt::ScreenOrientation o );
    void fuelTanks( FuelTanks tanks );
    void fuelTanks2();
    void stopFuelFlow();
    void unitsAirspeed();
    void setSwitchableTanks( bool bSwitchable );
    void settingsClosed();
    void magDev( int iMagDev );
    void downloaderConnected();
    void senderConnected();
    void senderWritten( qint64 sent );
};

#endif // __AHRSMAINWIN_H__
