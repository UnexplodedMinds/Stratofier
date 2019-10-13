/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __COUNTRYDIALOG_H__
#define __COUNTRYDIALOG_H__

#include <QDialog>

#include "ui_CountryDialog.h"
#include "Canvas.h"


class CountryDialog : public QDialog, public Ui::CountryDialog
{
    Q_OBJECT

public:
    explicit CountryDialog( QWidget *pParent, CanvasConstants *pC );
    ~CountryDialog();

    QList<Canvas::CountryCode> countries() { return m_countryList; }
    QList<Canvas::CountryCode> deleteCountries() { return m_deleteCountryList; }

private:
    void updateAirports();
    void populateCountries();

    CanvasConstants           *m_pC;
    QList<Canvas::CountryCode> m_countryList;
    QList<Canvas::CountryCode> m_deleteCountryList;

private slots:
    void ok();
};

#endif // __COUNTRYDIALOG_H__
