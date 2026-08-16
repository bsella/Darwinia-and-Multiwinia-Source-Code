#include "lib/preferences.h"
#include <vector>
#include "lib/input/inputdriver_prefs.h"

enum {
	COND_TRUE,
	COND_FALSE
};


PrefsInputDriver::PrefsInputDriver()
: m_keys()
{
	setName( "Prefs" );
}


InputParserState PrefsInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
		                                                    InputSpec &spec )
{
	InputParserState state = STATE_WANT_DRIVER;
	unsigned int idx = 0;
	if ( ( idx >= tokens.length() ) ||
	     ( tokens[idx++] != "pref" ) ) return state;

	state = STATE_WANT_CONTROL;
	if ( idx >= tokens.length() ) return state;

	std::string key = tokens[idx++];
	spec.condition = COND_TRUE;
	if ( "not" == key || "!" == key ) {
		spec.condition = COND_FALSE;
		if ( idx >= tokens.length() ) return state;
		key = tokens[idx++];
	}

	if ( '!' == key[0] ) {
		if ( '!' == key[1] ) return STATE_WANT_COND;
		if ( COND_FALSE == spec.condition ) return STATE_WANT_COND;
		spec.condition = COND_FALSE;
		key = key.substr( 1, key.length() - 1 );
	}

	if ( !g_prefsManager->DoesKeyExist( key.c_str() ) ) return state;
	spec.control_id = keyPosition( key );

	spec.type = INPUT_TYPE_BOOL;

	return ( idx < tokens.length() ) ? STATE_OVERSTEP : STATE_DONE;
}


bool PrefsInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	details.type = INPUT_TYPE_BOOL;
	if ( 0 <= spec.control_id && spec.control_id < m_keys.size() ) {
		bool val = ( g_prefsManager->GetInt( m_keys[ spec.control_id ].c_str(), 0 ) > 0 );
		switch ( spec.condition ) {
			case COND_TRUE:  return  val;
			case COND_FALSE: return !val;
			default:         return false; // Should never get here!
		}
	}
	return false; // Should never get here!
}


void PrefsInputDriver::Advance()
{
}


// In the same order as enum InputParserState (see inputdriver.h)
static std::string errors[] = {
	"An unknown error occurred.",
	"The driver type was not recognised.",
	"The preference key was not recognised.",
	"Double negatives not allowed.",
	"",
	"",
	"There are too many tokens in the input description.",
	"",
	"There was no parsing error."
};


const std::string &PrefsInputDriver::getLastParseError( InputParserState state )
{
	return errors[ state ];
}


bool PrefsInputDriver::getInputDescription( InputSpec const &spec, InputDescription & )
{
	InputDetails details;
	return getInput( spec, details );
}


int PrefsInputDriver::keyPosition( std::string const &key )
{
	for ( size_t i=0; i < m_keys.size(); i++ )
		if ( m_keys[i] == key )
			return i;

	m_keys.emplace_back( key );
	return m_keys.size();
}
