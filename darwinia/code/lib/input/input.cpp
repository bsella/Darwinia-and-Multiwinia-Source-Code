
#include "lib/input/input.h"
#include "lib/targetcursor.h"

#include <cstring>
#include <vector>
#include <iostream>

#define Q(x) "\"" << x << "\""

InputManager g_inputManager;


InputManager::InputManager()
: drivers(),
  m_idle( true ),
  m_inputMode( INPUT_MODE_KEYBOARD )
{}


InputManager::~InputManager()
{
	for ( unsigned i = 0; i < drivers.size(); ++i )
		if ( drivers[ i ] ) delete drivers[ i ];
}


void InputManager::parseInputPrefs( TextReader &reader, bool replace )
{
#ifdef TARGET_DEBUG
	ofstream derr( "inputprefs_debug.txt" );
#else
	std::ostream &derr = std::cout;
#endif
	while ( reader.ReadLine() ) {
		// derr << "Line " << line++ << ": ";
		bool iconline = false;
		char *control = reader.GetNextToken();
		if ( control ) {
			char *eq = reader.GetNextToken();
			if ( !eq || strcmp( eq, "=" ) != 0 ) {
				if ( eq && !strcmp( eq, "~" ) ) {
					iconline = true;
				} else {
					derr << "Assignment not found." << std::endl;
					continue;
				}
			}

			std::string inputspec = reader.GetRestOfLine();
			controltype_t control_id = getControlID( control );
			if ( control_id >= 0 ) {

				if ( iconline ) {
					if ( inputspec != "" ) {
						unsigned len = inputspec.length() - 1;
						if ( inputspec[ len ] == '\n' )
							inputspec = inputspec.substr( 0, len-- );
						if ( inputspec[ len ] == '\r' )
							inputspec = inputspec.substr( 0, len );
						bindings.setIcon( control_id, inputspec );
						derr << "Treated as icon: " << Q(bindings.getIcon( control_id )) << std::endl;
					} else
						derr << "Empty icon line." << std::endl;
					continue;
				}

				InputSpec spec;
				std::string err;
				if ( PARSE_SUCCESS( parseInputSpecString( inputspec, spec, err ) ) ) {
					if ( bindings.bind( control_id, spec, replace ) )
						derr << "Success. Driver = " << drivers[ spec.driver ]->getName() << std::endl;
					else
						derr << "Binding failed." << std::endl;
				} else derr << "Parse failed - " << err << std::endl;
			} else derr << "Control ID not found." << std::endl;

		} else derr << "Blank line." << std::endl;
	}
}


InputParserState InputManager::parseInputSpecString( std::string const &description, InputSpec &spec, std::string &err )
{
	InputSpecTokens tokens( description );
	return parseInputSpecTokens( tokens, spec, err );
}


InputParserState InputManager::parseInputSpecTokens( InputSpecTokens const &tokens, InputSpec &spec, std::string &err )
{
	InputParserState state = STATE_ERROR;
	InputParserState curr  = STATE_ERROR;
	err = "";
	if ( tokens.length() == 0 ) return state;

	for ( unsigned i = 0; i < drivers.size(); ++i )
		if ( PARSE_SUCCESS( curr = drivers[ i ]->parseInputSpecification( tokens, spec ) ) ) {
			spec.driver = i;
			return curr;
		} else {
			if ( curr >= state ) {
				state = curr;
				err = drivers[ i ]->getLastParseError( state );
			}
		}

	return state;
}


bool InputManager::controlEventA( ControlType type, InputDetails &details )
{
	if ( bindings.isActive( type ) ) {
		const InputSpecList &specs = bindings[ type ];

		for ( unsigned i = 0; i < specs.size(); ++i )
			if ( checkInput( specs[ i ], details ) ) return true;
	}

	details.type = INPUT_TYPE_FAIL;
	return false;
}


bool InputManager::controlEvent( ControlType type, InputDetails &details )
{
	return controlEventA( type, details );
}


bool InputManager::controlEvent( ControlType type )
{
	InputDetails details;
	return controlEvent( type, details );
}


const std::string &InputManager::controlIcon( ControlType type ) const
{
	return bindings.getIcon( type );
}


void InputManager::Advance()
{
	bindings.Advance();

	bool idleNext = true;
	InputMode nextInputMode = INPUT_MODE_NONE;

	for ( unsigned i = 0; i < drivers.size(); ++i ) {
		InputDriver *driver = drivers[ i ];
		driver->Advance();
		bool driverIdle = driver->isIdle();
		idleNext = idleNext && driverIdle;

		if ( !driverIdle ) {
			InputMode driverInputMode = driver->getInputMode();
			if ( driverInputMode > nextInputMode )
				nextInputMode = driverInputMode; // This prefers the Gamepad
		}
	}

	m_idle = idleNext;

	// Record the mode, if we know it, otherwise stick with last recorded
	if ( nextInputMode > INPUT_MODE_NONE )
		m_inputMode = nextInputMode;

//	if ( g_inputFilterManager ) g_inputFilterManager->Advance();

	if ( g_target ) g_target->Advance();
}


void InputManager::PollForEvents()
{
	for ( unsigned i = 0; i < drivers.size(); ++i ) {
		InputDriver *driver = drivers[ i ];
		driver->PollForEvents();
	}
}


bool InputManager::checkInput( InputSpec const &spec, InputDetails &details )
{
	InputDriver *driver = drivers[ spec.driver ];
	bool ans = driver->getInput( spec, details );
	if ( !ans )
		details.type = INPUT_TYPE_FAIL;
	return ans;
}


bool InputManager::getFirstActiveInput( InputSpec &spec, bool instant )
{
	for ( unsigned i = 0; i < drivers.size(); ++i ) {
		InputDriver *driver = drivers[ i ];
		if ( driver->getFirstActiveInput( spec, instant ) ) {
			spec.driver = i;
			return true;
		}
	}
	return false;
}


void InputManager::addDriver( InputDriver *driver )
{
	if ( driver )
		drivers.push_back( driver );
}


void InputManager::suppressEvent( ControlType type )
{
	bindings.suppress( type );
}


bool InputManager::getBoundInputDescription( ControlType type, InputDescription &desc )
{
	const InputSpecList &specs = bindings[ type ];
	if ( specs.size() >= 1 )
		for ( const InputSpec& spec : specs )
			if ( getInputDescription( spec, desc ) ) {
				desc.translate();
				return true;
			}
	return false;
}


bool InputManager::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	InputDriver *driver = drivers[ spec.driver ];
	return driver->getInputDescription( spec, desc );
}


controltype_t InputManager::getControlID( std::string const &control )
{
	return bindings.getControlID( control );
}


void InputManager::getControlString( ControlType type, std::string &name )
{
	bindings.getControlString( type, name );
}


void InputManager::replacePrimaryBinding( ControlType type, std::string const &prefString )
{
	InputSpec spec;
	std::string err;
	if ( PARSE_SUCCESS( parseInputSpecString( prefString, spec, err ) ) ) {
		//bindings.replacePrimaryBinding( type, spec );
		bindings.bind( type, spec, true );
	}
}


void InputManager::Clear()
{
	bindings.Clear();
}


bool InputManager::isIdle()
{
	return m_idle;
}


InputMode InputManager::getInputMode()
{
	return m_inputMode;
}


void InputManager::printNumBindings()
{
	for ( controltype_t i = 0; i < NumControlTypes; ++i ) {
		const InputSpecList &specs = bindings[ i ];
		std::cout << "There are "
		     << specs.size()
	         << " binding for type " << i << std::endl;
	}
}
