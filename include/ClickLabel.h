/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __CLICKLABEL_H__
#define __CLICKLABEL_H__

#include <QLabel>


class Canvas;
class Keyboard;


class ClickLabel : public QLabel
{
    Q_OBJECT

public:
    ClickLabel( QWidget *pParent = nullptr );
    ~ClickLabel();

    void setTitle( const QString &qsTitle ) { m_qsTitle = qsTitle; }
    void setCanvas( Canvas *pCanvas ) { m_pCanvas = pCanvas; }
    void setDecimals( int iDec ) { m_iDecimals = iDec; }
    void setFullKeyboard( bool bFullKeyboard ) { m_bFullKeyboard = bFullKeyboard; }

protected:
    void mousePressEvent( QMouseEvent *pEvent );

private:
    QString   m_qsTitle;
    Canvas   *m_pCanvas;
    int       m_iDecimals;
    bool      m_bFullKeyboard;

private slots:
    void key( const QString &qsKey );
    void keyboardComplete();
    void keyboardComplete2();
};

#endif // __CLICKLABEL_H__
