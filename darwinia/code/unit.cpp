#include <algorithm>
#include <iterator>
#include <math.h>
#include <memory>
#include <ranges>
#include <vector>

#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"

#include "app.h"
#include "entity_grid.h"
#include "level_file.h"
#include "location.h"
#include "routing_system.h"
#include "sound/soundsystem.h"
#include "team.h"
#include "unit.h"
#include "camera.h"

#include "worldobject/entity.h"
#include "worldobject/worldobject.h"
#include "worldobject/lasertrooper.h"

Unit::Unit(int troopType, int teamId, int unitId, int numEntities, Vector3 const &_pos)
:   m_wayPoint(0,0,0),
    m_routeId(-1),
	m_routeWayPointId(-1),
    m_teamId(teamId),
    m_unitId(unitId),
    m_troopType(troopType),
    m_centrePos(_pos),
    m_vel(0,0,0),
    m_radius(0.0f),
    m_targetDir(1,0,0),
    m_attackAccumulator(0.0f),
    m_accumulatedCentre(0,0,0),
    m_accumulatedRadiusSquared(0.0f),
    m_numAccumulated(0)
{
    //m_entities.SetSize( numEntities );
}

Unit::~Unit()
{
	Team *myTeam = &g_app->m_location->m_teams[m_teamId];
	if (myTeam->m_currentUnit == this)
	{
		myTeam->m_currentUnit = nullptr;
	}
}

void Unit::Begin()
{
}

Entity *Unit::NewEntity( int *_index )
{
    auto [index, new_entity] = m_entities.MoveOrReplaceFirstNull(std::unique_ptr<Entity>{Entity::NewEntity( m_troopType )});
    
    *_index = index;
    return new_entity.get();
}

int Unit::AddEntity( std::unique_ptr<Entity>&& entity)
{
    auto [index, _] = m_entities.MoveOrReplaceFirstNull(std::move(entity));
    return index;
}

void Unit::AdvanceEntities()
{
    for (const auto& [i, entity] : m_entities.EnumerateOptionals())
    {
        if(!entity) continue;

        if( entity->m_enabled )
        {
            Vector3 oldPos( entity->m_pos );

            START_PROFILE( g_app->m_profiler, Entity::GetTypeName( entity->m_type ) );
            bool amIdead = entity->Advance( this );
            END_PROFILE( g_app->m_profiler, Entity::GetTypeName( entity->m_type ) );

            if( amIdead )
            {
                WorldObjectId myId( m_teamId, m_unitId, i, entity->m_id.GetUniqueId() );

                g_app->m_location->m_entityGrid->RemoveObject( myId, oldPos.x, oldPos.z, entity->m_radius );

                entity.reset();
            }
            else
            {
                WorldObjectId myId( m_teamId, m_unitId, i, entity->m_id.GetUniqueId() );
                g_app->m_location->m_entityGrid->UpdateObject( myId, oldPos.x, oldPos.z, entity->m_pos.x, entity->m_pos.z, entity->m_radius );
            }
        }
    }
}


bool Unit::IsInView() const
{
    return( g_app->m_camera->SphereInViewFrustum( m_centrePos, m_radius ) );
}


void Unit::Render( float _predictionTime )
{
    for (auto& entity : m_entities.Values())
	{
        entity.Render( _predictionTime );
	}

    glEnable(GL_CULL_FACE);
}

bool Unit::Advance( )
{
    //
    // Maintain our centre and radius values

    Vector3 oldPos = m_centrePos;
    m_centrePos = m_accumulatedCentre;
    if( m_numAccumulated != 0 ) m_centrePos /= m_numAccumulated;
    m_vel = (m_centrePos - oldPos) / SERVER_ADVANCE_PERIOD;
    m_radius = sqrtf( m_accumulatedRadiusSquared );

    if( m_entities.Values().empty() )
    {
        m_radius = 0.0f;
        return true;
    }

    m_accumulatedCentre.Zero();
    m_accumulatedRadiusSquared = 0.0f;
    m_numAccumulated = 0;

	if (m_routeId != -1)
	{
		FollowRoute();
	}

    bool waypointChanged = false;
    if( waypointChanged )
    {
        RecalculateOffsets();
    }

    float leadDistance = 1.0f;

    if( m_troopType == Entity::TypeLaserTroop )
    {
        for (auto& entity : m_entities.Values())
        {
            auto& l = static_cast<LaserTrooper&>(entity);

            if( (l.m_pos - l.m_targetPos).Mag() < leadDistance / 5.0f )
            {
                Vector3 pos = l.m_pos;
//              Vector3 targetPos = m_wayPoint;
//              targetPos += GetFormationOffset( FormationRectangle, l->m_unitIndex );
//              targetPos = l->PushFromObstructions( targetPos );
//              //targetPos = l->PushFromEachOther( targetPos );
//              l->m_unitTargetPos = targetPos;

                Vector3 targetPos = l.m_unitTargetPos;
                Vector3 desiredDirection = (targetPos - pos).Normalise();
                float distance = (targetPos - pos).Mag();
                float amountToMove = leadDistance;
                if( amountToMove > distance ) amountToMove = distance;
                pos += desiredDirection * amountToMove;
                pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
                pos = l.PushFromObstructions( pos );
                //pos = l->PushFromEachOther( pos );

                l.m_targetPos = pos;
            }
        }
    }

    return false;
}

int Unit::NumEntities() const
{
    return std::ranges::distance(m_entities.Values());
}


int Unit::NumAliveEntities() const
{
    return std::ranges::count_if(m_entities.Values(), [](const Entity& entity){return !entity.m_dead;});
}


void Unit::Attack( Vector3 pos, bool _withGrenade )
{
	//
	// Deal with grenades

	if (_withGrenade)
	{
        float nearest = 9999.9f;
        Entity *nearestEnt = nullptr;

        //
        // Find the entity nearest to the target that has a grenade

        for( auto& entity : m_entities.Values())
        {
            if( !entity.m_dead )
            {
                float distance = (entity.m_pos - pos).Mag();
                if( distance < nearest )
                {
                    nearest = distance;
                    nearestEnt = &entity;
                }
            }
        }

        if( nearestEnt )
        {
            g_app->m_location->ThrowWeapon( nearestEnt->m_pos, pos, WorldObject::EffectThrowableGrenade, m_teamId );
        }
	}


    //
    // Build a list of entities that can attack now

    std::vector<Entity*> canAttack;

    for( const auto& [i, entity] : m_entities.EnumerateValues() )
    {
        if( entity.m_enabled &&
            !entity.m_dead &&
            entity.m_reloading == 0.0f )
        {
            canAttack.push_back(&entity);
        }
    }


    if( !canAttack.empty() )
    {
        //
        // Decide the maximum number of entities
        // that can attack now without pauses appearing in fire rate

        float reloadTime = EntityBlueprint::GetStat( m_troopType, Entity::StatRate );
        float timeToWait = (float) reloadTime / (float) NumEntities();
        m_attackAccumulator += ( (float) SERVER_ADVANCE_PERIOD / timeToWait );


        //
        // Pick guys randomly to attack

        while( !canAttack.empty() && m_attackAccumulator >= 1.0f )
        {
            m_attackAccumulator -= 1.0f;
            int randomIndex = syncfrand(canAttack.size());
            auto* entity = canAttack[randomIndex];
            canAttack.erase(canAttack.begin() + randomIndex);
    		entity->Attack( pos );
        }
    }
}


void Unit::UpdateEntityPosition( Vector3 pos, float _radius )
{
    m_accumulatedCentre += pos;
    ++m_numAccumulated;

    float distanceFromCentre = (pos - m_centrePos).Mag();
    distanceFromCentre += _radius;
    float distanceFromCentreSquared = distanceFromCentre * distanceFromCentre;

    if( distanceFromCentreSquared > m_accumulatedRadiusSquared )
    {
        m_accumulatedRadiusSquared = distanceFromCentreSquared;
    }
}


Vector3 Unit::GetWayPoint()
{
	return m_wayPoint;
}


void Unit::SetWayPoint(Vector3 const &_pos)
{
	m_wayPoint = _pos;
}


Vector3 Unit::GetFormationOffset(int _formation, int _index)
{
    static float *s_offsets = nullptr;
    int const numOffsets = 100;
    float const spacedOut = 4.0f;

    if( _index == -1 )
    {
        return g_zeroVector;
    }

    // Generate some noise in our formation
    if( !s_offsets )
    {
        s_offsets = new float[numOffsets];
        for( int i = 0; i < numOffsets; i++ )
        {
            s_offsets[i] = syncfrand(spacedOut/4.0f);
        }
    }

    switch(_formation)
    {
        case FormationRectangle:
        {
            int rowLen = sqrtf( NumEntities() );

            float x = _index % rowLen;
            x -= rowLen/2.0f;
            x *= spacedOut;

            float z = _index / rowLen;
			z -= rowLen/2.0f;
			z *= spacedOut;

            x += s_offsets[_index % numOffsets];
            z += s_offsets[_index % numOffsets];

            return Vector3(z, 0, x);
        }

        case FormationAirstrike:
        {
            int rowLen = 4;

            float x = _index % rowLen;
            x -= rowLen/2.0f;
            x *= spacedOut * 6.0f;

            float y = _index / rowLen;
			y -= rowLen/2.0f;
			y *= spacedOut * 3.0f;

            float z = _index / rowLen;
			z -= rowLen/2.0f;
			z *= spacedOut * 6.0f;

            x += s_offsets[_index % numOffsets];
            y += s_offsets[_index % numOffsets];
            z += s_offsets[_index % numOffsets];

            return Vector3(x, -y, z);
        }
    }

    return g_zeroVector;
}


Vector3 Unit::GetOffset(int _formation, int _index)
{
    Vector3 formationOffset = GetFormationOffset( _formation, _index );
    Vector3 finalPos = m_wayPoint + formationOffset;

    if( finalPos.Mag() == 0.0f )
    {
        return finalPos;
    }
    else
    {
        return (finalPos - m_wayPoint);
    }
}


void Unit::RecalculateOffsets()
{
    int offset = 0;

    for( auto& entity : m_entities.Values() )
    {
        if( !entity.m_dead )
        {
            entity.m_formationIndex = offset;
            ++offset;
        }
        else
        {
            entity.m_formationIndex = -1;
        }
    }

    if( m_troopType == Entity::TypeLaserTroop )
    {
        for (auto& entity : m_entities.Values())
        {
            auto& l = static_cast<LaserTrooper&>(entity);
            Vector3 targetPos = m_wayPoint;
            targetPos += GetFormationOffset( FormationRectangle, l.m_id.GetIndex() );
            targetPos = l.PushFromObstructions( targetPos );
            //targetPos = l->PushFromEachOther( targetPos );
            l.m_unitTargetPos = targetPos;
        }
    }
}

void Unit::FollowRoute()
{
	DarwiniaDebugAssert(m_routeId != -1);
	Route *route = g_app->m_location->m_levelFile->GetRoute(m_routeId);
	DarwiniaDebugAssert(route);

	if (m_routeWayPointId == -1)
	{
		m_routeWayPointId = 0;
	}

    WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
    m_wayPoint = waypoint->GetPos();
	Vector3 targetVect = m_wayPoint - m_centrePos;

    if (waypoint->m_type != WayPoint::TypeBuilding &&
        targetVect.Mag() < 10.0f)
	{
		m_routeWayPointId++;
		if (m_routeWayPointId >= route->m_wayPoints.Size())
		{
			m_routeWayPointId = -1;
			m_routeId = -1;
		}
	}

    //
    // If its a building instead of a 3D pos, this unit will never
    // get to the next waypoint.  A new unit is created when the unit
    // enters the teleport, and taht new unit will automatically
    // continue towards the next waypoint instead.
    //
}


Entity *Unit::RayHit(Vector3 const &_rayStart, Vector3 const &_rayDir)
{
	for (auto& entity : m_entities.Values())
	{
        if (entity.RayHit(_rayStart, _rayDir))
        {
            return &entity;
        }
	}

	return nullptr;
}

void Unit::DirectControl( TeamControls const& )
{
}

