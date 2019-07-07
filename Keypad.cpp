/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QKeyEvent>

#include "Keypad.h"

#include "ui_Keypad.h"


Keypad::Keypad( QWidget *pParent, const QString &qsTitle, bool bTimeMode )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint ),
      m_bTimeMode( bTimeMode ),
      m_iTimePos( 0 )
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

    if( bTimeMode )
    {
        m_pValueLabel->setText( "00:00" );
        m_pBack->setIcon( QIcon() );
        m_pBack->setText( "MM" );
    }
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
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '0';
        else
            qsValue.append( "0" );
    }
    else if( qsObjName == "m_p1" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '1';
        else
            qsValue.append( "1" );
    }
    else if( qsObjName == "m_p2" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '2';
        else
            qsValue.append( "2" );
    }
    else if( qsObjName == "m_p3" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '3';
        else
            qsValue.append( "3" );
    }
    else if( qsObjName == "m_p4" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '4';
        else
            qsValue.append( "4" );
    }
    else if( qsObjName == "m_p5" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '5';
        else
            qsValue.append( "5" );
    }
    else if( qsObjName == "m_p6" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '6';
        else
            qsValue.append( "6" );
    }
    else if( qsObjName == "m_p7" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '7';
        else
            qsValue.append( "7" );
    }
    else if( qsObjName == "m_p8" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '8';
        else
            qsValue.append( "8" );
    }
    else if( qsObjName == "m_p9" )
    {
        if( m_bTimeMode )
            qsValue[m_iTimePos] = '9';
        else
            qsValue.append( "9" );
    }
    else if( (qsObjName == "m_pBack") && (!m_bTimeMode) )
    {
        if( !qsValue.isEmpty() )
            qsValue.chop( 1 );
    }
    else if( (qsObjName == "m_pBack") && m_bTimeMode )
    {
        if( m_iTimePos < 3 )
        {
            m_iTimePos = 3;
            m_pBack->setText( "SS" );
        }
        else
        {
            m_iTimePos = 0;
            m_pBack->setText( "MM" );
        }
    }

    if( m_bTimeMode )
    {
        m_iTimePos++;
        if( m_iTimePos == 2 )
        {
            m_iTimePos = 3;
            m_pBack->setText( "SS" );
        }
        else if( m_iTimePos > 4 )
        {
            m_pValueLabel->setText( qsValue );
            accept();
        }
    }

    m_pValueLabel->setText( qsValue );
}


int Keypad::value()
{
    return m_pValueLabel->text().toInt();
}


QString Keypad::textValue()
{
    return m_pValueLabel->text();
}


void Keypad::clear()
{
    m_pValueLabel->clear();
}


void Keypad::setTitle( const QString &qsTitle )
{
    m_pTitleLabel->setText( qsTitle );
}

