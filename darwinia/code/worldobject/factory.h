#ifndef _included_factory_h
#define _included_factory_h

#include "worldobject/building.h"
#include "worldobject/entity.h"
#include "worldobject/spiritstore.h"


class FileWriter;


class Factory: public Building
{
public:
    unsigned char   m_troopType;
    unsigned char   m_stats[Entity::NumStats];

	int				m_initialCapacity;		// Read from level file

    int             m_unitId;
    int             m_numToCreate;
    int             m_numCreated;

    float           m_timeToCreate;         // Total Time to create ALL troops
    float           m_timeSoFar;

    enum
    {
        StateUnused,
        StateCreating,
        StateRecharging
    };
    int m_state;

    SpiritStore     m_spiritStore;

public:
    Factory();

	void Initialise( Building *_template )override;

	void Render         ( float predictionTime )override;
    void RenderAlphas   ( float predictionTime );

	bool Advance()override;
    void AdvanceStateUnused();
    void AdvanceStateCreating();
    void AdvanceStateRecharging();

	void SetTeamId( int _teamId )override;

    void RequestUnit( unsigned char _troopType, int _numToCreate );

	void Read(TextReader *_in, bool _dynamic)override;
	void Write(FileWriter *_out)override;
};

#endif
