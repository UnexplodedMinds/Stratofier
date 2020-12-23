/*
Stratofier Stratux AHRS Display
(c) 2018 Allen K. Lair, Sky Fun
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
    connect( m_pAirportsButton, SIGNAL( clicked() ), this, SLOT( airports() ) );
    connect( m_pOverlaysButton, SIGNAL( clicked() ), this, SLOT( overlays() ) );
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


void BugSelector::airports()
{
    done( static_cast<int>( Airports ) );
}


void BugSelector::overlays()
{
    done( static_cast<int>( Overlays ) );
}


void BugSelector::calibrate()
{
    done( static_cast<int>( Calibrate ) );
}
