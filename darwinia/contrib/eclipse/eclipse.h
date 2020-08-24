
/*
 *          ECLIPSE
 *        version 2.0
 *
 *
 *  Generic Interface Library
 *
 */

#ifndef _included_eclipse_h
#define _included_eclipse_h

#include "eclwindow.h"
#include "eclbutton.h"

// ============================================================================
// High level management

void EclInitialise ( int screenW, int screenH );

void EclUpdateMouse     ( int mouseX, int mouseY, bool lmb, bool rmb );
void EclUpdateKeyboard  ( int keyCode, bool shift, bool ctrl, bool alt );
void EclRender ();
void EclUpdate ();

void EclShutdown ();


// ============================================================================
// Window management


void EclRegisterWindow          ( EclWindow *window, EclWindow *parent=NULL );
void EclRemoveWindow            ( const char *name );
void EclRegisterPopup           ( EclWindow *window );
void EclRemovePopup             ();

void EclBringWindowToFront      ( const char *name );
void EclSetWindowPosition       ( const char *name, int x, int y );
void EclSetWindowSize           ( const char *name, int w, int h );

int EclGetWindowIndex           ( const char *name );                                             // -1 = failure
EclWindow *EclGetWindow         ( const char *name );
EclWindow *EclGetWindow         ( int x, int y );                                           

bool EclMouseInWindow           ( EclWindow *window );
bool EclMouseInButton           ( EclWindow *window, EclButton *button );
bool EclIsTextEditing			();

void EclRegisterTooltipCallback ( void (*_callback) (EclWindow *, EclButton *) );

void EclMaximiseWindow          ( const char *name );
void EclUnMaximise              ();

const char *EclGetCurrentButton          ();
const char *EclGetCurrentClickedButton   ();

const char *EclGetCurrentFocus             ();
void EclSetCurrentFocus              ( const char *name );

const char *EclGenerateUniqueWindowName( const char *name );                                // In static mem (don't delete!)
LList <EclWindow *> *EclGetWindows ();

// ============================================================================
// Dirty rectangles

class DirtyRect
{
public:
    DirtyRect();
    DirtyRect( int newx, int newy, 
               int newwidth, int newheight ); 
	int m_x;
	int m_y;
	int m_width;
	int m_height;
};


void EclRegisterClearFunction   ( void (*_clearDraw) (int, int, int, int) );

void EclDirtyWindow             ( const char *name );
void EclDirtyWindow             ( EclWindow *window );
void EclDirtyRectangle          ( int x, int y, int w, int h );

bool EclRectangleOverlap        ( int x1, int y1, int w1, int h1,		
                                  int x2, int y2, int w2, int h2 );

void EclResetDirtyRectangles    ();

LList<DirtyRect *> *EclGetDirtyRects ();


// ============================================================================
// Other

int EclGetAccurateTime          ();
int EclGetScreenW				();
int EclGetScreenH				();


#endif
