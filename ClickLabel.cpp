/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include "ClickLabel.h"
#include "Keypad.h"
#include "Canvas.h"


void ClickLabel::mousePressEvent( QMouseEvent *pEvent )
{
    Keypad keypad( this, m_qsTitle );

    m_pCanvas->setKeypadGeometry( &keypad );
    if( keypad.exec() == QDialog::Accepted )
        setText( QString::number( keypad.value(), 'f', 2 ) );

    QLabel::mousePressEvent( pEvent );
}
