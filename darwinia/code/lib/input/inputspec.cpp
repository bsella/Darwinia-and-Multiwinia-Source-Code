#include <string>
#include <sstream>

#include "lib/input/inputspec.h"

InputSpecTokens::InputSpecTokens( std::vector<std::string> _tokens )
: m_tokens( _tokens ) {}


InputSpecTokens::InputSpecTokens( std::string _string )
: m_tokens()
{
	std::istringstream in( _string );
	std::string token;

	m_tokens.clear();

	while ( in >> token )
		m_tokens.push_back( token );

}


InputSpecTokens::~InputSpecTokens() {}


unsigned InputSpecTokens::length() const
{
	return m_tokens.size();
}


const std::string &InputSpecTokens::operator[] ( unsigned _index ) const
{
	if ( 0 <= _index && _index < length() )
		return m_tokens[ _index ];
	else
		throw "Index out of bounds.";
}


std::unique_ptr<InputSpecTokens> InputSpecTokens::operator()( int _start, int _end ) const {

	std::vector<std::string> vec;
	vec.clear();

	int len = length();

	if ( _start < 0 ) _start += len; // Allow negatives to count from the last token
	if ( _end < 0 ) _end += len;

	if ( 0 <= _start && _start < len &&
	     0 <= _end && _end < len &&
		 _start <= _end ) { // We're in range
		for ( int idx = _start; idx <= _end; idx++ )
			vec.push_back( (*this)[ idx ] );
	}

	std::unique_ptr<InputSpecTokens> tokens( new InputSpecTokens( vec ) );
	return tokens;
}


std::ostream &operator<<( std::ostream &stream, InputSpecTokens const &tokens )
{
	unsigned len = tokens.length();
	if ( len > 0 )
		stream << tokens[ 0 ];
	for ( unsigned i = 1; i < len; ++i )
		stream << " " << tokens[ i ];
	return stream;
}
