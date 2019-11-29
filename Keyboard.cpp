/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#include <QKeyEvent>
#include <QObjectList>
#include <QTimer>

#include "Keyboard.h"


Keyboard::Keyboard( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );

    QObjectList  kids = children();
    QObject     *pKid;
    QPushButton *pButton;

    foreach( pKid, kids )
    {
        pButton = qobject_cast<QPushButton *>( pKid );
        if( pButton != nullptr )
            connect( pButton, SIGNAL( clicked() ), this, SLOT( keyboardClick() ) );
    }

    QTimer::singleShot( 100, this, SLOT( init() ) );
}


Keyboard::~Keyboard()
{
}


void Keyboard::init()
{
    pBack->setIconSize( QSize( pBack->width() / 3, pBack->width() / 3 ) );
}


void Keyboard::keyboardClick()
{
    QObject *pObj = sender();

    if( pObj == nullptr )
        return;

    QString qsObjName = pObj->objectName();

    if( qsObjName.isEmpty() )
        return;
    if( qsObjName == "pOK" )
        return;

    qsObjName = qsObjName.right( qsObjName.length() - 1 );

    if( qsObjName == "Back" )
        qsObjName = "_";
    else if( qsObjName == "Spc" )
        qsObjName = " ";
    else if( qsObjName == "Dot" )
        qsObjName = ".";

    emit key( qsObjName );
}

