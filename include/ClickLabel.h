/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __CLICKLABEL_H__
#define __CLICKLABEL_H__

#include <QLabel>


class Canvas;


class ClickLabel : public QLabel
{
    Q_OBJECT

public:
    ClickLabel( QWidget *pParent = Q_NULLPTR ) : QLabel( pParent )
    {
    }

    void setTitle( const QString &qsTitle ) { m_qsTitle = qsTitle; }
    void setCanvas( Canvas *pCanvas ) { m_pCanvas = pCanvas; }

protected:
    void mousePressEvent( QMouseEvent *pEvent );

private:
    QString m_qsTitle;
    Canvas *m_pCanvas;
};

#endif // __CLICKLABEL_H__
