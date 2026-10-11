#include "window_manager_rust.h"

extern "C"
{
	WindowManagerHandle* darw_WindowManager_Create();
	void darw_WindowManager_Delete(WindowManagerHandle*);

    bool darw_WindowManager_CreateWin(WindowManagerHandle*, int _width, int _height, bool _windowed, int _colourDepth, int _refreshRate, int _zDepth, bool _waitVRT, bool _antiAlias, const char *_title );
	void darw_WindowManager_HideWin(WindowManagerHandle*);
	void darw_WindowManager_DestroyWin(WindowManagerHandle*);
	void darw_WindowManager_Flip(WindowManagerHandle*);
	void darw_WindowManager_PollForMessages(WindowManagerHandle*);
	void darw_WindowManager_SetMousePos(WindowManagerHandle*, int x, int y);
	void darw_WindowManager_EnsureMouseCaptured(WindowManagerHandle*);
	void darw_WindowManager_EnsureMouseUncaptured(WindowManagerHandle*);
	void darw_WindowManager_CaptureMouse(WindowManagerHandle*);
	void darw_WindowManager_UncaptureMouse(WindowManagerHandle*);
	void darw_WindowManager_HideMousePointer(WindowManagerHandle*);
	void darw_WindowManager_UnhideMousePointer(WindowManagerHandle*);
	void darw_WindowManager_SaveDesktop(WindowManagerHandle*);
	void darw_WindowManager_RestoreDesktop(WindowManagerHandle*);
	void darw_WindowManager_OpenWebsite(WindowManagerHandle*, const char *_url );
	void darw_WindowManager_NastySetMousePos(WindowManagerHandle*, int x, int y);
	void darw_WindowManager_NastyMoveMouse(WindowManagerHandle*, int x, int y);
	void darw_WindowManager_NastyPollForMessages(WindowManagerHandle*);
	void darw_WindowManager_PreventFullscreenStartup(WindowManagerHandle*);
	void darw_WindowManager_ListAllDisplayModes(WindowManagerHandle*);
	void* darw_WindowManager_Window(WindowManagerHandle*);

	void darw_WindowManager_RunGameLoop(WindowManagerHandle*);
}


WindowManagerRust::WindowManagerRust()
{
    m_handle = darw_WindowManager_Create();
}

WindowManagerRust::~WindowManagerRust()
{
    darw_WindowManager_Delete(m_handle);
}

bool WindowManagerRust::CreateWin(int _width, int _height,bool _windowed, int _colourDepth, int _refreshRate, int _zDepth, bool _waitVRT, bool _antiAlias, const char *_title )
{
    return darw_WindowManager_CreateWin(m_handle, _width, _height, _windowed, _colourDepth, _refreshRate, _zDepth, _waitVRT, _antiAlias, _title );
}

void WindowManagerRust::HideWin(){darw_WindowManager_HideWin(m_handle);}
void WindowManagerRust::DestroyWin(){darw_WindowManager_DestroyWin(m_handle);}
void WindowManagerRust::Flip(){darw_WindowManager_Flip(m_handle);}
void WindowManagerRust::PollForMessages(){darw_WindowManager_PollForMessages(m_handle);}
void WindowManagerRust::SetMousePos(int x, int y){darw_WindowManager_SetMousePos(m_handle, x, y);}
void WindowManagerRust::EnsureMouseCaptured (){darw_WindowManager_EnsureMouseCaptured(m_handle);}
void WindowManagerRust::EnsureMouseUncaptured (){darw_WindowManager_EnsureMouseUncaptured(m_handle);}
void WindowManagerRust::CaptureMouse (){darw_WindowManager_CaptureMouse(m_handle);}
void WindowManagerRust::UncaptureMouse (){darw_WindowManager_UncaptureMouse(m_handle);}
void WindowManagerRust::HideMousePointer    (){darw_WindowManager_HideMousePointer(m_handle);}
void WindowManagerRust::UnhideMousePointer  (){darw_WindowManager_UnhideMousePointer(m_handle);}
void WindowManagerRust::SaveDesktop         (){darw_WindowManager_SaveDesktop(m_handle);}
void WindowManagerRust::RestoreDesktop      (){darw_WindowManager_RestoreDesktop(m_handle);}
void WindowManagerRust::OpenWebsite( const char *_url ){darw_WindowManager_OpenWebsite(m_handle, _url );}
void WindowManagerRust::NastySetMousePos(int x, int y){darw_WindowManager_NastySetMousePos(m_handle, x, y);}
void WindowManagerRust::NastyMoveMouse(int x, int y){darw_WindowManager_NastyMoveMouse(m_handle, x, y);}
void WindowManagerRust::NastyPollForMessages(){darw_WindowManager_NastyPollForMessages(m_handle);}
void WindowManagerRust::PreventFullscreenStartup(){darw_WindowManager_PreventFullscreenStartup(m_handle);}
void WindowManagerRust::ListAllDisplayModes (){darw_WindowManager_ListAllDisplayModes(m_handle);}

void WindowManagerRust::RunGameLoop()
{
    darw_WindowManager_RunGameLoop(m_handle);
}

PlatformWindow WindowManagerRust::Window()
{
    return darw_WindowManager_Window(m_handle);
}