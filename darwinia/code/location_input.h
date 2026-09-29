#ifndef INCLUDED_LOCATION_INPUT
#define INCLUDED_LOCATION_INPUT


#include "worldobject/worldobject.h"

class Building;
class Engineer;


class LocationInput
{
private:
	void	AdvanceRadarDishControl(Building *_building);
	void	AdvanceNoSelection();
    void	AdvanceTeamControl();

public:

	WorldObjectOrUnit GetObjectUnderMouse( int _teamId );

	void	Advance();
	void	Render();
};


#endif
