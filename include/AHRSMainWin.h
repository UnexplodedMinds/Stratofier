/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __AHRSMAINWIN_H__
#define __AHRSMAINWIN_H__

#include <QMainWindow>
#include <QDateTime>

#include "ui_AHRSMainWin.h"
#include "Canvas.h"


class StreamReader;
class AHRSCanvas;
class MenuDialog;
class QPushButton;


class AHRSMainWin : public QMainWindow, public Ui::AHRSMainWin
{
    Q_OBJECT

public:
    explicit AHRSMainWin( const QString &qsIP, bool bPortrait );
    ~AHRSMainWin();

    bool          menuActive() { return (m_pMenuDialog != nullptr); }
    void          restartTimer();
    void          stopTimer();
    AHRSCanvas   *disp() { return m_pAHRSDisp; }
    StreamReader *streamReader() { return m_pStratuxStream; }

public slots:
    void menu();
    void changeTimer();
    void init();

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

private slots:
    void statusUpdate( bool bStratux, bool bAHRS, bool bGPS, bool bTraffic );
    void resetLevel();
    void resetGMeter();
    void upgradeStratofier();
    void shutdownStratux();
    void shutdownStratofier();
    void trafficToggled( bool bAll );
    void showAirports( Canvas::ShowAirports eShow );
    void showRunways( bool bShow );
    void orient( Qt::ScreenOrientation o );
    void fuelTanks( FuelTanks tanks );
    void fuelTanks2();
    void stopFuelFlow();
    void unitsAirspeed();
    void dayMode();
    void setSwitchableTanks( bool bSwitchable );
};

#endif // __AHRSMAINWIN_H__
