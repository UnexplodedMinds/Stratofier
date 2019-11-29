/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QWindow>
#include <QGuiApplication>
#include <QTimer>

#include "ClickLabel.h"
#include "Keypad.h"
#include "Keyboard.h"
#include "Canvas.h"


extern Keyboard *g_pKeyboard;


ClickLabel::ClickLabel( QWidget *pParent )
    : QLabel( pParent ),
      m_iDecimals( 2 ),
      m_bFullKeyboard( false )
{
}


ClickLabel::~ClickLabel()
{
    if( g_pKeyboard != nullptr )
    {
        delete g_pKeyboard;
        g_pKeyboard = nullptr;
    }
}

void ClickLabel::mousePressEvent( QMouseEvent *pEvent )
{
    if( g_pKeyboard != nullptr )
        keyboardComplete2();
    if( m_bFullKeyboard )
    {
        QWindow *pWindow = QGuiApplication::topLevelAt( QPoint( 0, parentWidget()->height() + 2 ) );
        QSize    screenSize = pWindow->size();

        g_pKeyboard = new Keyboard( this );
        // Landscape
        if( screenSize.width() > screenSize.height() )
            g_pKeyboard->setGeometry( 0, 0, screenSize.width(), screenSize.height() / 2 );
        // Portrait
        else
            g_pKeyboard->setGeometry( 0, screenSize.height() * 2 / 3, screenSize.width(), screenSize.height() / 3 );
        connect( g_pKeyboard, SIGNAL( key( const QString& ) ), this, SLOT( key( const QString& ) ) );
        connect( g_pKeyboard, SIGNAL( accepted() ), this, SLOT( keyboardComplete() ) );
        g_pKeyboard->show();
    }
    else
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
}


void ClickLabel::key( const QString &qsKey )
{
    QString qs( text() );

    if( qsKey == "_" )
        qs.chop( 1 );
    else
        qs.append( qsKey );

    setText( qs );
}


void ClickLabel::keyboardComplete()
{
    QTimer::singleShot( 100, this, SLOT( keyboardComplete2() ) );
}


void ClickLabel::keyboardComplete2()
{
    delete g_pKeyboard;
    g_pKeyboard = nullptr;
}

