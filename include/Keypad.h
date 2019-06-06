/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __ROSCOPIKEYPAD_H__
#define __ROSCOPIKEYPAD_H__

#include <QDialog>

#include "ui_Keypad.h"


class Keypad : public QDialog, public Ui::Keypad
{
    Q_OBJECT

public:
    explicit Keypad( QWidget *pParent, const QString &qsTitle, bool bTimeMode = false );
    ~Keypad();

    int     value();
    QString textValue();
    void    clear();
    void    setTitle( const QString &qsTitle );

private:
    bool    m_bTimeMode;
    int     m_iTimePos;

private slots:
    void keypadClick();
};

#endif // __ROSCOPIKEYPAD_H__
