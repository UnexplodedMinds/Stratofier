/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __OVERLAYS_H__
#define __OVERLAYS_H__

#include <QDialog>

#include "ui_Overlays.h"
#include "Canvas.h"


class Overlays : public QDialog, public Ui::Overlays
{
    Q_OBJECT

public:
    explicit Overlays( QWidget *pParent );
    ~Overlays();

private:
    void loadSettings();

    StratofierSettings m_settings;

private slots:
    void init();

    void traffic();
    void airports();
    void runways();
    void airspaces();
    void altitudes();
    void privateAirports();

signals:
    void trafficToggled( bool );
    void showAirports( Canvas::ShowAirports );
    void showPrivate( bool );
    void showRunways( bool );
    void showAirspaces( bool );
    void showAltitudes( bool );
};

#endif // __Overlays_H__
