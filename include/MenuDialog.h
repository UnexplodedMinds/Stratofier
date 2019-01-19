/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __MENUDIALOG_H__
#define __MENUDIALOG_H__

#include <QDialog>

#include "ui_MenuDialog.h"


class MenuDialog : public QDialog, public Ui::MenuDialog

{
    Q_OBJECT

public:
    explicit MenuDialog( QWidget *pParent );
    ~MenuDialog();

private slots:
    void traffic( bool bAll );

signals:
    void resetLevel();
    void resetGMeter();
    void upgradeRosco();
    void shutdownStratux();
    void shutdownRoscoPi();
    void trafficToggled( bool );
    void timer();
};

#endif // __MENUDIALOG_H__
