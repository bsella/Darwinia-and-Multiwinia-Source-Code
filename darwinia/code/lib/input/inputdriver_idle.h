#ifndef INCLUDED_INPUTDRIVER_IDLE_H
#define INCLUDED_INPUTDRIVER_IDLE_H

#include "lib/input/inputdriver_simple.h"


class IdleInputDriver : public SimpleInputDriver {

private:
	double m_idleTime;
	double m_oldIdleTime;
	double m_lastChecked;

	bool acceptDriver( std::string const &name )override;

	control_id_t getControlID( std::string const &name )override;

	inputtype_t getControlType( control_id_t control_id )override;

	condition_t getConditionID( std::string const &name, inputtype_t &type )override;

public:
	IdleInputDriver();

	// Get input state. True if the input was triggered (input condition met). If true,
	// details are placed in details.
	bool getInput( InputSpec const &spec, InputDetails &details )override;

	// This triggers a read from the input hardware and does message polling
	void Advance()override;

	// Return a helpful error string when there's a problem
	//const std::string &getLastParseError( InputParserState state )override;

	// Fill out a description of the input defined by spec
	bool getInputDescription( InputSpec const &spec, InputDescription &desc )override;

	// Get the name of the driver (debuggung purposes)
	//const std::string &getName()const;

};


#endif // INCLUDED_INPUTDRIVER_IDLE_H
