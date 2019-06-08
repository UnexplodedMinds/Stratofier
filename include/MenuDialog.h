/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
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
    void traffic( bool bAll );
    void inOut( bool bAll );
    void airports();
    void fuel();

signals:
    void resetLevel();
    void resetGMeter();
    void upgradeRosco();
    void shutdownStratux();
    void shutdownRoscoPi();
    void trafficToggled( bool );
    void inOutToggled( bool );
    void showAirports( Canvas::ShowAirports );
    void timer();
    void fuelTanks( FuelTanks );
    void stopFuelFlow();
    void unitsKnots();
    void dayMode();
};

#endif // __MENUDIALOG_H__
