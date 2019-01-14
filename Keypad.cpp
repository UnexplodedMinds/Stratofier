/*
Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QKeyEvent>

#include "Keypad.h"

#include "ui_Keypad.h"


Keypad::Keypad( QWidget *pParent, const QString &qsTitle )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );

    QObjectList  kids = children();
    QObject     *pKid;
    QPushButton *pButton;

    foreach( pKid, kids )
    {
        pButton = qobject_cast<QPushButton *>( pKid );
        if( pButton != 0 )
            connect( pButton, SIGNAL( clicked() ), this, SLOT( keypadClick() ) );
    }

    m_pTitleLabel->setText( qsTitle );
}


Keypad::~Keypad()
{
}


void Keypad::keypadClick()
{
    QObject *pObj = sender();

    if( pObj == 0 )
        return;

    QString qsObjName = pObj->objectName();

    if( qsObjName.isEmpty() )
        return;
    if( (qsObjName == "m_pCancel") || (qsObjName == "m_pSet") )
        return;

    QString qsValue( m_pValueLabel->text() );

    if( qsObjName == "m_p0" )
        qsValue.append( "0" );
    else if( qsObjName == "m_p1" )
        qsValue.append( "1" );
    else if( qsObjName == "m_p2" )
        qsValue.append( "2" );
    else if( qsObjName == "m_p3" )
        qsValue.append( "3" );
    else if( qsObjName == "m_p4" )
        qsValue.append( "4" );
    else if( qsObjName == "m_p5" )
        qsValue.append( "5" );
    else if( qsObjName == "m_p6" )
        qsValue.append( "6" );
    else if( qsObjName == "m_p7" )
        qsValue.append( "7" );
    else if( qsObjName == "m_p8" )
        qsValue.append( "8" );
    else if( qsObjName == "m_p9" )
        qsValue.append( "9" );
    else if( qsObjName == "m_pBack" )
    {
        if( !qsValue.isEmpty() )
            qsValue.chop( 1 );
    }

    m_pValueLabel->setText( qsValue );
}


int Keypad::value()
{
    return m_pValueLabel->text().toInt();
}


void Keypad::clear()
{
    m_pValueLabel->clear();
}


void Keypad::setTitle( const QString &qsTitle )
{
    m_pTitleLabel->setText( qsTitle );
}

