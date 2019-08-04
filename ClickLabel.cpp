/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include "ClickLabel.h"
#include "Keypad.h"
#include "Canvas.h"


void ClickLabel::mousePressEvent( QMouseEvent *pEvent )
{
    Keypad keypad( this, m_qsTitle );

    m_pCanvas->setKeypadGeometry( &keypad );
    keypad.exec();

    setText( QString::number( keypad.value() ) );

    QLabel::mousePressEvent( pEvent );
}
