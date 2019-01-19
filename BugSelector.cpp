/*
RoscoPi Stratux AHRS Display
(c) 2018 Allen K. Lair, Unexploded Minds
*/

#include <QKeyEvent>

#include "BugSelector.h"


BugSelector::BugSelector( QWidget *pParent )
    : QDialog( pParent, Qt::Dialog | Qt::FramelessWindowHint )
{
    setupUi( this );
	connect( m_pHeadingButton, SIGNAL( clicked() ), this, SLOT( headingSel() ) );
	connect( m_pWindButton, SIGNAL( clicked() ), this, SLOT( windSel() ) );
    connect( m_pClearButton, SIGNAL( clicked() ), this, SLOT( clearBugs() ) );
}


BugSelector::~BugSelector()
{
}


void BugSelector::headingSel()
{
	done( static_cast<int>( HeadingBug ) );
}


void BugSelector::windSel()
{
	done( static_cast<int>( WindBug ) );
}


void BugSelector::clearBugs()
{
    done( static_cast<int>( ClearBugs ) );
}

