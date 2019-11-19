/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
*/

#ifndef __DETAILSDIALOG_H__
#define __DETAILSDIALOG_H__

#include <QDialog>

#include "ui_DetailsDialog.h"
#include "Canvas.h"


class DetailsDialog : public QDialog, public Ui::DetailsDialog
{
    Q_OBJECT

public:
    explicit DetailsDialog( QWidget *pParent, CanvasConstants *pC, Airport *pA );
    ~DetailsDialog();
};

#endif // __DETAILSDIALOG_H__
