
#ifndef _included_networkwindow_h
#define _included_networkwindow_h

#include "interface/darwinia_window.h"


class NetworkWindow : public DarwiniaWindow
{
public:
	NetworkWindow( const char *name );

    void Render( bool hasFocus );
};


#endif
