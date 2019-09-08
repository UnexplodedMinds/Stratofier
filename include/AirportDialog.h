/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __AIRPORTDIALOG_H__
#define __AIRPORTDIALOG_H__

#include <QDialog>

#include "ui_AirportDialog.h"
#include "Canvas.h"


// NOTE: The reason this is called settings instead of Fuel or something similar is that the expectation that other non-fuel related settings
//       will go here or possibly be moved here from the menu.
class AirportDialog : public QDialog, public Ui::AirportDialog
{
    Q_OBJECT

public:
    explicit AirportDialog( QWidget *pParent, CanvasConstants *pC );
    ~AirportDialog();

    QString selectedAirport() { return m_qsSel; } // Airport name (ID isn't always reliable)

private:
    void updateAirports();

    CanvasConstants *m_pC;
    bool             m_bAllAirports;
    QString          m_qsSel;

private slots:
    void within( int iDist );
    void airportsType();
    void airportSelected( QTableWidgetItem *pItem );
};

#endif // __AIRPORTDIALOG_H__
