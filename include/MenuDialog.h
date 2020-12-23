/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __MENUDIALOG_H__
#define __MENUDIALOG_H__

#include <QDialog>

#include "ui_MenuDialog.h"
#include "Canvas.h"


class MenuDialog : public QDialog, public Ui::MenuDialog
{
    Q_OBJECT

public:
    explicit MenuDialog( QWidget *pParent, bool bPortrait );
    ~MenuDialog();

private:
    Canvas::ShowAirports m_eShowAirports;
    bool                 m_bPortrait;

private slots:
    void fuel();
    void settings();
    void help();

signals:
    void resetLevel();
    void resetGMeter();
    void upgradeStratofier();
    void shutdownStratux();
    void shutdownStratofier();
    void timer();
    void fuelTanks( FuelTanks );
    void stopFuelFlow();
    void unitsAirspeed();
    void setSwitchableTanks( bool );
    void halfMode( bool );
    void settingsClosed();
    void magDev( int );
};

#endif // __MENUDIALOG_H__
