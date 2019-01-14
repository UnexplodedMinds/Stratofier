/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include <QDialog>

#include "ui_Keypad.h"


class Keypad : public QDialog, public Ui::Keypad
{
    Q_OBJECT

public:
    explicit Keypad( QWidget *pParent, const QString &qsTitle );
    ~Keypad();

    int  value();
    void clear();
    void setTitle( const QString &qsTitle );

private slots:
    void keypadClick();
};

#endif // __KEYPAD_H__
