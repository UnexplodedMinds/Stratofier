/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __AIRPORTDIALOG_H__
#define __AIRPORTDIALOG_H__

#include <QDialog>

#include "ui_AirportDialog.h"
#include "Canvas.h"


class AirportDialog : public QDialog, public Ui::AirportDialog
{
    Q_OBJECT

public:
    explicit AirportDialog( QWidget *pParent, CanvasConstants *pC, const QString &qsTitle );
    ~AirportDialog();

    QString selectedAirport() { return m_qsSel; } // Airport name (ID isn't always reliable)

private:
    void updateAirports();
    void toggle( QObject *pObj );

    CanvasConstants *m_pC;
    bool             m_bAllAirports;
    QString          m_qsSel;
    double           m_dDist;

private slots:
    void within();
    void airportsType();
    void airportSelected( QTableWidgetItem *pItem );
};

#endif // __AIRPORTDIALOG_H__
