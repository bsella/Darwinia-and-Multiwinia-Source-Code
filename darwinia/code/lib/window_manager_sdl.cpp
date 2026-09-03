#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_syswm.h>

#include <SDL2/SDL_video.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "lib/debug_utils.h"
#include "window_manager_sdl.h"
#include "lib/input/sdl_eventhandler.h"
#include "lib/input/inputdriver_sdl_mouse.h"
//#include "input.h"
#include "app.h"

//#include "app.h"
//#include "renderer.h"

#ifdef TARGET_OS_LINUX
//#include "prefix.h"
#endif

#ifdef TARGET_OS_MACOSX
#include <ApplicationServices/ApplicationServices.h>
#endif

SDLMouseInputDriver *g_sdlMouseDriver = nullptr;

// Uncomment if you want lots of output in debug mode.
//#define VERBOSE_DEBUG

// *** Constructor
WindowManagerSDL::WindowManagerSDL()
	:	m_tryingToCaptureMouse(false),
		m_preventFullscreenStartup(false)
{
	DarwiniaReleaseAssert(SDL_Init(SDL_INIT_VIDEO) == 0, "Couldn't initialise SDL");

	ListAllDisplayModes();
	SaveDesktop();
}

WindowManagerSDL::~WindowManagerSDL()
{
	while ( m_resolutions.ValidIndex ( 0 ) )
	{
		Resolution *res = m_resolutions.GetData ( 0 );
		delete res;
		m_resolutions.RemoveData ( 0 );
	}
	m_resolutions.EmptyAndDelete();
	SDL_Quit();
}

void WindowManagerSDL::ListAllDisplayModes()
{
	auto display_index = 0;

	auto num_display_modes = SDL_GetNumDisplayModes(display_index);
	
	for(auto display_mode_index = 0; display_mode_index < num_display_modes; display_mode_index++)
	{
		SDL_DisplayMode mode;
		if(SDL_GetDisplayMode(display_index, display_mode_index, &mode) == 0)
		{
			Resolution *res = new Resolution(mode.w, mode.h);
			if (GetResolutionId(mode.w, mode.h) == -1)
				m_resolutions.PutData( res );
			else
				delete res;
		}
	}
}


void WindowManagerSDL::PreventFullscreenStartup()
{
	if (!m_window)
		m_preventFullscreenStartup = true;
}


void WindowManagerSDL::SaveDesktop()
{
	SDL_DisplayMode mode;

	if (SDL_GetDesktopDisplayMode(0, &mode) == 0)
	{
		m_desktopColourDepth = SDL_BITSPERPIXEL(mode.format);
		m_desktopRefresh = mode.refresh_rate;

		m_desktopScreenW = mode.w;
		m_desktopScreenH = mode.h;
	}
}


void WindowManagerSDL::RestoreDesktop()
{
}


void WindowManagerSDL::NastySetMousePos(int x, int y)
{
	m_x = x;
	m_y = y;
}


void WindowManagerSDL::NastyMoveMouse(int x, int y)
{ }


bool WindowManagerSDL::CreateWin(int _width, int _height, bool _windowed, int _colourDepth, int _refreshRate,
								 int _zDepth, bool _waitVRT, bool _antiAlias, const char *_title)
{
    int bpp = m_desktopColourDepth;
    int flags = 0;

	m_windowed = _windowed || m_preventFullscreenStartup;
	m_preventFullscreenStartup = false;

	// Set the flags for creating the mode
	flags = SDL_WINDOW_OPENGL;
	if (!_windowed)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		
		// Look for the best valid video mode
		if (_colourDepth != -1)
			bpp = _colourDepth;

		unsigned best_diagonal_difference = (unsigned) -1;
		SDL_DisplayMode best_mode;

		{
			auto display_index = 0;

			auto num_display_modes = SDL_GetNumDisplayModes(display_index);
			
			for(auto display_mode_index = 0; display_mode_index < num_display_modes; display_mode_index++)
			{
				SDL_DisplayMode mode;
				if(SDL_GetDisplayMode(display_index, display_mode_index, &mode) == 0)
				{
					unsigned diagonal_difference = (_width  - mode.w) * (_width  - mode.w) + 
					                               (_height - mode.h) * (_height - mode.h);
					if (diagonal_difference < best_diagonal_difference) {
						best_mode = mode;
						best_diagonal_difference = diagonal_difference;
					}
				}
			}
		}
	
		m_screenW = best_mode.w;
		m_screenH = best_mode.h;
	}
	else {
#ifdef TARGET_OS_MACOSX
		const SDL_version *linkedVersion = SDL_Linked_Version();
		// We ensure that the we are linked with SDL version >= 1.2.9 because
		// previous versions had major problems with the coordinate system 
		// when using OpenGL, in windowed and in full-screen mode.
		
		AppReleaseAssert(linkedVersion->major * 1000 + linkedVersion->minor * 100 + linkedVersion->patch >= 1209,
			"App requires at to run with SDL version 1.2.9 or greater");		
#endif
		// Usually any combination is OK for windowed mode.
		m_screenW = _width;
		m_screenH = _height;
		
		// Add it to the list of screen resolutions if need be
		Resolution *found = nullptr;
		for (int i = 0; i < m_resolutions.Size(); i++) {
			Resolution *res = m_resolutions.GetData(i);
			if (res->m_width == _width && res->m_height == _height) {
				found = res;
				break;
			}
		}
		
		if (!found) {
			Resolution *res = new Resolution(_width, _height);
			m_resolutions.PutData(res);
		}
	}	
	
	switch (bpp) {	
		case 24:
		case 32:
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
			break;
	
		case 16:
		default:
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
		break;
	}
	
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );	
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );

	if ( _antiAlias ) {		
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 2 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 4 );
	}
	else {
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 0 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 0 );
	}
	
	// Synchronize to the vertical refresh rate of the monitor, typically 60Hz. This
	// does end up causing graphics flushing to block eventually. But that's what we
	// want.
	SDL_GL_SetSwapInterval ( 1 );
	

#if 0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

	m_window = SDL_CreateWindow(_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, flags);

	// Fall back to not antialiased
	if (!m_window && _antiAlias) {
		_antiAlias = 0;
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 0 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 0 );
		m_window = SDL_CreateWindow(_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, flags);
	}
	
	// Fall back to a 16 bit Z-Buffer
	if (!m_window && _zDepth != 16) {
		DebugOut ( "SDL_SetVideoMode failed with '%s'. Switching to 16-bit Z-Buffer.\n", SDL_GetError() );
		_zDepth = 16;
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );
		m_window = SDL_CreateWindow(_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, flags);
	}
	
	if (!m_window)
	{
		DebugOut ( "SDL_SetVideoMode failed with '%s'. Can't continue.\n", SDL_GetError() );
		return false;
	}

	m_context = SDL_GL_CreateContext(m_window);

	glewExperimental = GL_TRUE;
	glewInit();

	// Pass back the actual values to the Renderer
	_width = m_screenW;
	_height = m_screenH;
	_colourDepth = bpp;

	if (m_mouseCaptured)
		CaptureMouse();
	
	//UnicodeString unicodeTitle(_title);
	//const char *asciiTitle = unicodeTitle.GetCharArray();
	//SDL_WM_SetCaption(asciiTitle, asciiTitle);

	return true;
}


void WindowManagerSDL::DestroyWin()
{
}


void WindowManagerSDL::Flip()
{
	PollForMessages();
	
	if (m_tryingToCaptureMouse) 
		g_windowManager->CaptureMouse();
	
	SDL_GL_SwapWindow(m_window);
}

void WindowManagerSDL::PollForMessages()
{
	SDL_Event sdlEvent;

	while (SDL_PollEvent(&sdlEvent))
		static_cast<SDLEventHandler *>(g_eventHandler)->HandleSDLEvent(sdlEvent);
	
	if (m_tryingToCaptureMouse)
		CaptureMouse();
}


void WindowManagerSDL::EnsureMouseCaptured()
{
	if (g_app->m_gameMode != App::GameModeNone)
		CaptureMouse();
}


void WindowManagerSDL::EnsureMouseUncaptured()
{
	UncaptureMouse();
}


void WindowManagerSDL::CaptureMouse()
{
	if (m_mouseCaptured)
		return;
		
#ifdef TARGET_OS_MACOSX
	// Important not to grab the mouse 
	// until it's in the window proper. Otherwise
	// we might end up doing some strange things on MAC OS X
	 if (!(SDL_GetAppState() & SDL_APPMOUSEFOCUS)) {
		m_tryingToCaptureMouse = true;
		return;
	}
#endif
		
	// Don't grab if we don't have focus
	if (!g_eventHandler->WindowHasFocus())
		return;
		
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_SetWindowGrab(m_window, SDL_TRUE);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_SetRelativeMouseMode(SDL_TRUE);

	m_mouseCaptured = true;
	m_tryingToCaptureMouse = false;
}


void WindowManagerSDL::UncaptureMouse()
{
	m_tryingToCaptureMouse = false;
	if (!m_mouseCaptured)
		return;
		
#ifdef VERBOSE_DEBUG
	AppDebugOut("Uncapturing mouse\n");
#endif
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_SetWindowGrab(m_window, SDL_FALSE);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouseInWindow(m_window, m_x, m_y);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	g_sdlMouseDriver->SetMousePosNoVelocity(m_x, m_y);

	m_mouseCaptured = false;
}


void WindowManagerSDL::HideMousePointer()
{
	SDL_ShowCursor(false);
	m_mousePointerVisible = false;
}


void WindowManagerSDL::UnhideMousePointer()
{
	SDL_ShowCursor(true);
	m_mousePointerVisible = true;
}


void WindowManagerSDL::SetMousePos(int x, int y)
{
	if (!m_mouseCaptured)
		SDL_WarpMouseInWindow(m_window, x, y);
}


void WindowManagerSDL::OpenWebsite( const char *_url )
{	
#ifdef TARGET_OS_MACOSX 
	CFURLRef url = CFURLCreateWithBytes(nullptr, (const UInt8 *)_url, strlen(_url),
										kCFStringEncodingASCII, nullptr);
	if (url)
	{
		LSOpenCFURLRef(url, nullptr);
		CFRelease(url);
	}
#elif defined TARGET_OS_LINUX
	/* Child */
	//char * const args[4] = { "/bin/sh", "open-www.sh", (char *)_url,  nullptr };
	//spawn("/bin/sh", args);
#endif
}

void WindowManagerSDL::HideWin()
{
#ifdef TARGET_OS_MACOSX
	ProcessSerialNumber me;
	
	GetCurrentProcess(&me);
	ShowHideProcess(&me, false);
#endif
}

#if defined(TARGET_OS_LINUX)
void SetupMemoryAccessHandlers();
void SetupPathToProgram(const char *program);

#include <string>
#include <unistd.h>

char *g_origWorkingDir = nullptr;

void ChangeToProgramDir(const char *program)
{
	std::string dir(program);
	std::string::size_type pos = dir.find_last_of('/');

	// Store the original working directory
	g_origWorkingDir = new char [PATH_MAX];
	getcwd(g_origWorkingDir, PATH_MAX);

	if (pos != std::string::npos) 
		dir.erase(pos);
	DarwiniaReleaseAssert(
		chdir(dir.c_str()) == 0, 
		"Failed to change directory to %s", dir.c_str());
}

#endif // TARGET_OS_LINUX || TARGET_OS_MACOSX

void WindowManagerSDL::NastyPollForMessages()
{

}

PlatformWindow *WindowManagerSDL::Window()
{
	SDL_VERSION(&m_info.version);
	SDL_GetWindowWMInfo(m_window, &m_info);

	if (SDL_GetWindowWMInfo(m_window, &m_info))
		return (PlatformWindow *)&m_info;
	else
		return nullptr;
}
