#pragma once

#include "lib/input/inputdriver_simple.h"
//#include "lib/input/sdl_eventproc.h"
#include "lib/input/keydefs.h"
#include <string>

class SDLKeyboardInputDriver : public SimpleInputDriver//, SDLEventProcessor
{
	public:
		SDLKeyboardInputDriver();
		~SDLKeyboardInputDriver();

		void Advance() override;
		void PollForEvents() override;
		//int HandleSDLEvent(const SDL_Event & event);
		bool acceptDriver( std::string const &name ) override;
		control_id_t getControlID( std::string const &name ) override;
		bool getInput( InputSpec const &spec, InputDetails &details ) override;
		inputtype_t getControlType( control_id_t control_id ) override;
		bool getInputDescription( InputSpec const &spec, InputDescription &desc ) override;
		bool getFirstActiveInput( InputSpec &spec, bool instant ) override;

	protected:
		bool m_keys[KEY_MAX];
		int m_keyDeltas[KEY_MAX];
		int m_keyNewDeltas[KEY_MAX];
};


static const int NUM_MB = 3;
static const int NUM_AXES = 3;

class SDLMouseInputDriver : public SimpleInputDriver
{
	public:
		SDLMouseInputDriver();
		~SDLMouseInputDriver();

		void Advance()override;
		void PollForEvents()override;
		bool acceptDriver( std::string const &name )override;
		control_id_t getControlID( std::string const &name )override;
		bool getInput( InputSpec const &spec, InputDetails &details )override;
		inputtype_t getControlType( control_id_t control_id )override;
		bool getInputDescription( InputSpec const &spec, InputDescription &desc )override;
		//void SetMousePosNoVelocity( int _x, int _y );

	private:
		bool        m_mb[NUM_MB];            // Mouse button states from this frame
		bool        m_mbOld[NUM_MB];         // Mouse button states from last frame
		int         m_mbDeltas[NUM_MB];      // Mouse button diffs

		int         m_mousePos[NUM_AXES];    // X Y Z
		int         m_mousePosOld[NUM_AXES]; // X Y Z
		int         m_mouseVel[NUM_AXES];    // X Y Z
};
