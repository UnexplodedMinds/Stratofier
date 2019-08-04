/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __FUELTANKSDIALOG_H__
#define __FUELTANKSDIALOG_H__

#include <QDialog>

#include "ui_FuelTanksDialog.h"
#include "Canvas.h"


// NOTE: The reason this is called settings instead of Fuel or something similar is that the expectation that other non-fuel related settings
//       will go here or possibly be moved here from the menu.
class FuelTanksDialog : public QDialog, public Ui::FuelTanksDialog
{
    Q_OBJECT

public:
    explicit FuelTanksDialog( QWidget *pParent, Canvas *pCanvas );
    ~FuelTanksDialog();

    FuelTanks settings() { return m_tanks; }

private:
    void loadSettings();

    FuelTanks m_tanks;

private slots:
    void saveSettings();
    void switchable();
    void resetFuel();
};

#endif // __FUELTANKSDIALOG_H__
