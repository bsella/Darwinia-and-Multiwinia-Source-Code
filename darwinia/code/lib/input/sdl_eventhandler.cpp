#include "sdl_eventhandler.h"
#include <SDL2/SDL.h>
#include "app.h"
#include <SDL2/SDL_video.h>
#include <iostream>
#include <algorithm>

typedef std::vector<SDLEventProcessor *>::iterator ProcIt;

SDLEventHandler::SDLEventHandler(SDL_Window& window)
	: m_window(window)
	, m_has_focus(false)
{
}

bool SDLEventHandler::WindowHasFocus()
{
	return m_has_focus;
}

int SDLEventHandler::HandleSDLEvent(const SDL_Event & event)
{
	switch (event.type)
	{
		case SDL_QUIT:
			g_app->m_requestQuit = true;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
				case SDL_WINDOWEVENT_FOCUS_GAINED: m_has_focus = true; break;
				case SDL_WINDOWEVENT_FOCUS_LOST:   m_has_focus = false; break;
			}
			break;
	}
	
	int ans = -1;
	for ( ProcIt i = eventProcessors.begin(); i != eventProcessors.end(); ++i ) {
		ans = (*i)->HandleSDLEvent( event );
		if ( ans != -1 ) break;
	}
	return ans;
}

void SDLEventHandler::AddEventProcessor( SDLEventProcessor *_driver )
{
	if ( _driver )
		eventProcessors.push_back( _driver );
	else
		std::cerr << "AddEventProcessor: _driver is nullptr\n" << std::endl;
}

void SDLEventHandler::RemoveEventProcessor( SDLEventProcessor *_driver )
{
	auto found_driver = std::find(eventProcessors.begin(), eventProcessors.end(), _driver);
	if(found_driver != eventProcessors.end())
		eventProcessors.erase(found_driver);
}

SDLEventHandler *getSDLEventHandler()
{
	return dynamic_cast<SDLEventHandler *>(g_eventHandler);
}
