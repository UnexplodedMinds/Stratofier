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
    {
        if( m_iDecimals == 0 )
            setText( QString::number( static_cast<int>( keypad.value() ) ) );
        else
            setText( QString::number( keypad.value(), 'f', m_iDecimals ) );
    }

    QLabel::mousePressEvent( pEvent );
}
