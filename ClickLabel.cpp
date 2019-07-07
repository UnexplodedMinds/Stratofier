/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include "ClickLabel.h"
#include "Keypad.h"


void ClickLabel::mousePressEvent( QMouseEvent *pEvent )
{
    Keypad keypad( this, m_qsTitle );

    keypad.setGeometry( 0, 0, 400, 350 );
    keypad.exec();

    setText( QString::number( keypad.value() ) );

    QLabel::mousePressEvent( pEvent );
}
