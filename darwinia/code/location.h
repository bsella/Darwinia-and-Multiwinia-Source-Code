#ifndef INCLUDED_LOCATION_H
#define INCLUDED_LOCATION_H

#include <float.h>

#include "lib/vector3.h"
#include "lib/vector_with_options.hpp"

#include "network/server.h"

#include "globals.h"
#include "landscape.h"
#include "worldobject/building.h"
#include "worldobject/worldobject.h"
#include "worldobject/weapons.h"
#include "worldobject/spirit.h"


#include <ranges>

class ServerToClientLetter;
class WorldObject;
class WorldObjectEffect;
class Entity;
class EntityGrid;
class ObstructionGrid;
class WorldObjectId;
class Unit;
class LaserGod;
class LevelFile;
class Clouds;
class Water;
class Light;
class Team;
class TeamControls;


// ****************************************************************************
//  Class Location
// ****************************************************************************

class Location
{
protected:
	bool m_missionComplete;

    void SetMyTeamId			( unsigned char _teamId );
    void LoadLevel				( char const *_missionFilename, char const *_mapFilename );

    void AdvanceWeapons			();
    void AdvanceBuildings		();
    void AdvanceTeams			();
    void AdvanceSpirits			();
    void AdvanceClouds			();

    void RenderLandscape		();
    void RenderWeapons			();
    void RenderBuildings		();
    void RenderBuildingAlphas	();
    void RenderParticles		();
    void RenderTeams			();
    void RenderMagic			();
    void RenderSpirits			();
    void RenderClouds			();
    void RenderWater			();

    void InitLandscape			();
    void InitLights				();
    void InitTeams				();

	void DoMissionCompleteActions();

    Vector3     FindValidSpawnPosition( Vector3 const &_pos, float _spread );

public:
    Landscape       m_landscape;
    EntityGrid      *m_entityGrid;
    ObstructionGrid *m_obstructionGrid;
	LevelFile		*m_levelFile;
    Clouds          *m_clouds;
    Water           *m_water;

    Team            *m_teams;

    float           m_christmasTimer;

	std::vector<Light>               m_lights;
    VectorWithOptionals<WorldObject> m_effects;
    
    VectorWithOptionals<Laser>    m_lasers;
    
private:
    VectorWithOptionals<Building> m_buildings;
    VectorWithOptionals<Spirit>   m_spirits;

public:
    Location();
    ~Location();

    void Init               ( char const *_missionFilename, char const *_mapFilename );
    void InitBuildings			();
	void Empty				();

    void Advance            ();
    void Render             ( bool renderWaterAndClouds = true );

    void InitialiseTeam     ( unsigned char _teamId,
                              unsigned char _teamType );

    void RemoveTeam         ( unsigned char _teamId );

    Building* GetBuilding   ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float _maxDistance=FLT_MAX, float *_range=nullptr ) const;
    Unit*     GetUnit       ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float *_range=nullptr ) const;
    Entity*   GetEntity     ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float *_range=nullptr ) const;

    bool IsWalkable         ( Vector3 const &_from, Vector3 const &_to, bool _evaluateCliffs=false );
    bool IsVisible          ( Vector3 const &_from, Vector3 const &_to );

    void UpdateTeam         ( unsigned char teamId, TeamControls const& teamControls );

	int  SpawnSpirit        ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId, const WorldObjectId& _id );
    void ThrowWeapon        ( Vector3 const &_pos, Vector3 const &_target, int _type, unsigned char _fromTeamId );
    void FireRocket         ( Vector3 const &_pos, Vector3 const &_target, unsigned char _fromTeamId );
    void FireLaser          ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _fromTeamId );
    void FireTurretShell    ( Vector3 const &_pos, Vector3 const &_vel );
    void Bang               ( Vector3 const &_pos, float _range, float _damage );
    void CreateShockwave    ( Vector3 const &_pos, float _size, unsigned char _teamId=255 );

	bool MissionComplete	();

    void AdvanceChristmas();
    static int ChristmasModEnabled();           // 0 = unavailable, 1 = enabled, 2 = disabled

    std::vector<Entity*> SpawnEntities  ( Vector3 const &_pos, unsigned char _teamId, int _unitId,
                              unsigned char _type, int _numEntities, Vector3 const &_vel,
                              float _spread, float _range=-1.0f, int _routeId = -1, int _routeWaypointId = -1 );

    int         GetSpirit   ( WorldObjectId _id );

    bool        IsFriend    ( unsigned char _teamId1, unsigned char _teamId2 );

    Team        *GetMyTeam	();
	Entity		*GetEntity	( Vector3 const &_rayStart, Vector3 const &_rayDir );
	Building	*GetBuilding( Vector3 const &_rayStart, Vector3 const &_rayDir );

    WorldObject *GetWorldObject ( WorldObjectId _id );
    Entity		*GetEntity      ( WorldObjectId _id );
    Entity      *GetEntitySafe  ( WorldObjectId _id, unsigned char _type );                 // Safe to cast
    Unit        *GetUnit        ( WorldObjectId _id );
    WorldObject *GetEffect      ( WorldObjectId _id );
    Building    *GetBuilding    ( int _id );

    std::unique_ptr<Spirit>& GetSpirit( int index );

    void SetupFog			();
    void SetupLights		();

	void WaterReflect       (); // inverts direction of all lights

	void FlushOpenGlState	();
	void RegenerateOpenGlState();

    decltype(m_spirits)::ValuesView ValidSpirits();
    decltype(m_spirits)::EnumerateOptionalsView EnumerateSpirits();

    
    decltype(m_buildings)::ValuesView ValidBuildings();
    decltype(m_buildings)::EnumerateValuesView EnumerateValidBuildings();
    decltype(m_buildings)::EnumerateOptionalsView EnumerateBuildings();

    std::unique_ptr<Building>& AddBuilding(std::unique_ptr<Building>&&);
};



#endif
