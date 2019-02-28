/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#ifndef __CLICKLABEL_H__
#define __CLICKLABEL_H__

#include <QLabel>


class ClickLabel : public QLabel
{
    Q_OBJECT

public:
    ClickLabel( QWidget *pParent = Q_NULLPTR ) : QLabel( pParent ) {}

    void setTitle( const QString &qsTitle ) { m_qsTitle = qsTitle; }

protected:
    void mousePressEvent( QMouseEvent *pEvent );

private:
    QString m_qsTitle;
};

#endif // __CLICKLABEL_H__
