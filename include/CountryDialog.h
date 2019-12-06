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

    QList<Canvas::CountryCodeAirports> countriesAirports() { return m_countryListAirports; }
    QList<Canvas::CountryCodeAirports> deleteCountriesAirports() { return m_deleteCountryListAirports; }
    QList<Canvas::CountryCodeAirspace> countriesAirspaces() { return m_countryListAirspaces; }
    QList<Canvas::CountryCodeAirspace> deleteCountriesAirspaces() { return m_deleteCountryListAirspaces; }

private:
    void updateAirports();
    void populateCountriesAirports();
    void updateAirspaces();
    void populateCountriesAirspaces();

    CanvasConstants                   *m_pC;
    QList<Canvas::CountryCodeAirports> m_countryListAirports;
    QList<Canvas::CountryCodeAirports> m_deleteCountryListAirports;
    QList<Canvas::CountryCodeAirspace> m_countryListAirspaces;
    QList<Canvas::CountryCodeAirspace> m_deleteCountryListAirspaces;

private slots:
    void ok();
};

#endif // __COUNTRYDIALOG_H__
