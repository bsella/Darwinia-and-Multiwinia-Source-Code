#ifndef INCLUDED_SDL_EVENTHANDLER_H
#define INCLUDED_SDL_EVENTHANDLER_H

#include "lib/universal_include.h"

#include "lib/input/eventhandler.h"
#include "lib/input/sdl_eventproc.h"
#include <vector>

#include <SDL2/SDL.h>

class SDLEventHandler : public EventHandler, public SDLEventProcessor
{
private:
	std::vector<SDLEventProcessor *> eventProcessors;

	SDL_Window& m_window;

	bool m_has_focus;
	
public:
	SDLEventHandler(SDL_Window&);

	 bool WindowHasFocus();
	 int HandleSDLEvent(const SDL_Event & event);
	 
	// Register driver for SDL callbacks
	void AddEventProcessor( SDLEventProcessor *_driver );

	// Unregister driver (if it is still the registered one)
	void RemoveEventProcessor( SDLEventProcessor *_driver );
};

SDLEventHandler *getSDLEventHandler();

#endif