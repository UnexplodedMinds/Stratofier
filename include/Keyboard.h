/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __StratofierKEYBOARD_H__
#define __StratofierKEYBOARD_H__

#include <QDialog>

#include "ui_Keyboard.h"


class Keyboard : public QDialog, public Ui::Keyboard
{
    Q_OBJECT

public:
    explicit Keyboard( QWidget *pParent );
    ~Keyboard();

private slots:
    void keyboardClick();
    void init();

signals:
    void key( const QString& );
};

#endif // __StratofierKEYBOARD_H__
