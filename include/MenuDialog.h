/*
Stratux AHRS Display
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
    void resetLevel();
    void exitRoscoPi();
    void exitRoscoPiApp();
};

#endif // __MENUDIALOG_H__
