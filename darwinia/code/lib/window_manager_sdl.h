#ifndef INCLUDED_WINDOW_MANAGER_SDL_H
#define INCLUDED_WINDOW_MANAGER_SDL_H

#include "window_manager.h"
#include "lib/input/sdl_eventproc.h"
#include "lib/input/inputdriver_sdl_mouse.h"

extern SDLMouseInputDriver *g_sdlMouseDriver;

class WindowManagerSDL : public WindowManager
{
	// The mouse driver needs to know the size of the window in order to
	// clip the mouse coordinates to the screen.
	friend class SDLMouseInputDriver;

private:
	int m_x;
	int m_y;
	
public:
	WindowManagerSDL();
	~WindowManagerSDL();

	bool        CreateWin           (int _width, int _height,		       // Set _colourDepth, _refreshRate and/or 
			                         bool _windowed, int _colourDepth,	   // _zDepth to -1 to get default values
			                         int _refreshRate, int _zDepth, bool _waitVRT,
									 bool _antiAlias,
									 const wchar_t *_title ) override;
    
	void		HideWin				();
	void        DestroyWin          ()override;
	void        Flip                ()override;
	void        PollForMessages     ();
	void        SetMousePos         (int x, int y);
    
	void		EnsureMouseCaptured	  ()override;
	void		EnsureMouseUncaptured ()override;
	void        CaptureMouse          ()override;
	void        UncaptureMouse        ()override;
    
	void        HideMousePointer    ()override;
	void        UnhideMousePointer  ()override;
    
	void        SaveDesktop         ();
	void        RestoreDesktop      ();
	void        OpenWebsite( const char *_url )override;
	
	void NastySetMousePos(int x, int y)override;
	void NastyMoveMouse(int x, int y)override;
	void NastyPollForMessages()override;
	
	PlatformWindow *Window()override;
	void		PreventFullscreenStartup();
	
protected:
	void        ListAllDisplayModes ()override;
	bool		m_tryingToCaptureMouse;
		
	bool		m_setVideoMode;
	bool		m_preventFullscreenStartup;
};

#endif

