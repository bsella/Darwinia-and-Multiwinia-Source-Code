#ifndef _INCLUDED_INSTANT_UNIT_WINDOW_H
#define _INCLUDED_INSTANT_UNIT_WINDOW_H

#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"


// ****************************************************************************
// Class InstantUnitEditWindow
// ****************************************************************************

class InstantUnitEditWindow: public DarwiniaWindow
{
public:
	InstantUnitEditWindow(const char *name);
	~InstantUnitEditWindow();

	void Create();
};


// ****************************************************************************
// Class InstantUnitCreateWindow
// ****************************************************************************

class InstantUnitCreateWindow: public DarwiniaWindow
{
public:
	InstantUnitCreateWindow( const char *name );
	~InstantUnitCreateWindow();

	void Create();
};


#endif // LOCATION_EDITOR

#endif
