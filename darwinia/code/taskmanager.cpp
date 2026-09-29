#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/vector2.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"

#include "app.h"
#include "gamecursor.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"
#include "sepulveda.h"
#include "level_file.h"
#include "routing_system.h"
#include "entity_grid.h"
#include "particle_system.h"
#include "helpsystem.h"
#include "camera.h"
#include "location_input.h"

#include "sound/soundsystem.h"

#include "interface/prefs_other_window.h"

#include "worldobject/entity.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/officer.h"
#include "worldobject/darwinian.h"
#include "worldobject/trunkport.h"
#include "worldobject/worldobject.h"

Task::Task()
:   m_id(-1),
    m_type(GlobalResearch::TypeSquad),
    m_state(StateIdle),
    m_route(nullptr)
{
}


Task::~Task()
{
    delete m_route;
}


void Task::Start()
{
    m_state = StateStarted;

    switch( m_type )
    {
        case GlobalResearch::TypeSquad:         g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadSummon );           break;
        case GlobalResearch::TypeEngineer:      g_app->m_helpSystem->PlayerDoneAction( HelpSystem::EngineerSummon );        break;
    }
}


void Task::Target( Vector3 const &_pos )
{
    switch( m_type )
    {
        case GlobalResearch::TypeSquad:         TargetSquad( _pos );        break;
        case GlobalResearch::TypeEngineer:      TargetEngineer( _pos );     break;
        case GlobalResearch::TypeOfficer:       TargetOfficer( _pos );      break;
        case GlobalResearch::TypeArmour:        TargetArmour( _pos );       break;
    }
}


void Task::TargetSquad( Vector3 const &_pos )
{
    int teamId = g_app->m_globalWorld->m_myTeamId;

    int numEntities = 2 + g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeSquad );

    int unitId;
    m_unit = g_app->m_location->m_teams[teamId].NewUnit( Entity::TypeInsertionSquadie, numEntities, &unitId, _pos );
    m_entity = nullptr;
    g_app->m_location->SpawnEntities( _pos, teamId, unitId,
                                      Entity::TypeInsertionSquadie, numEntities, g_zeroVector, 10 );

    g_app->m_location->m_teams[teamId].SelectUnit( unitId, -1, -1 );

    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadSummon );
    m_state = StateRunning;

    g_app->m_soundSystem->TriggerOtherEvent( nullptr, "GestureSuccess", SoundSourceBlueprint::TypeGesture );

    int trackEntity = g_prefsManager->GetInt( OTHER_AUTOMATICCAM, 0 );
    if( trackEntity == 0 )
    {
        // work out if player is using control pad
		if ( g_inputManager.getInputMode() == INPUT_MODE_GAMEPAD )
			trackEntity = 2;
    }

    if( trackEntity == 2 )
    {
        g_app->m_camera->RequestEntityTrackMode( *m_unit );
    }
}


void Task::TargetEngineer( Vector3 const &_pos )
{
    int teamId = g_app->m_globalWorld->m_myTeamId;

	Vector3 pos = _pos;
	pos.y += 10.0f;
    m_entity = g_app->m_location->SpawnEntities( pos, teamId, -1, Entity::TypeEngineer, 1, g_zeroVector, 0 )[0];
    m_unit = nullptr;
    g_app->m_location->m_teams[teamId].SelectUnit( -1, m_entity->m_id.GetIndex(), -1 );

    m_state = StateRunning;
    g_app->m_soundSystem->TriggerOtherEvent( nullptr, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
}


void Task::TargetArmour( Vector3 const &_pos )
{
#ifndef DEMOBUILD
    int teamId = g_app->m_globalWorld->m_myTeamId;

    m_entity = g_app->m_location->SpawnEntities( _pos, teamId, -1, Entity::TypeArmour, 1, g_zeroVector, 0 )[0];
    m_unit = nullptr;
    g_app->m_location->m_teams[teamId].SelectUnit( -1, m_entity->m_id.GetIndex(), -1 );

    m_state = StateRunning;

    g_app->m_soundSystem->TriggerOtherEvent( nullptr, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
#endif
}


Entity& Task::Promote( Entity& entity )
{
    int teamId = g_app->m_globalWorld->m_myTeamId;

    //
    // Spawn an Officer

    auto spawned = g_app->m_location->SpawnEntities( entity.m_pos, teamId, -1, Entity::TypeOfficer, 1, entity.m_vel, 0 );
    Officer *officer = static_cast<Officer *>(spawned[0]);
    DarwiniaDebugAssert( officer );


    //
    // Particle effect

    int numFlashes = 5 + darwiniaRandom() % 5;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(5.0f), frand(15.0f), sfrand(5.0f) );
        g_app->m_particleSystem->CreateParticle( entity.m_pos, vel, Particle::TypeControlFlash );
    }


    static_cast<Darwinian&>(entity).m_promoted = true;

    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::OfficerCreate );

    return *spawned[0];
}


Entity& Task::Demote( WorldObjectId _id )
{
    // Make demoted officers return to green
    //int teamId = g_app->m_globalWorld->m_myTeamId;
    int teamId = 0;

    Entity *entity = g_app->m_location->GetEntity( _id );
    DarwiniaDebugAssert( entity );


    //
    // Spawn a Darwinian

    auto spawned = g_app->m_location->SpawnEntities( entity->m_pos, teamId, -1, Entity::TypeDarwinian, 1, entity->m_vel, 0 );


    //
    // Particle effect

    int numFlashes = 5 + darwiniaRandom() % 5;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(5.0f), frand(15.0f), sfrand(5.0f) );
        g_app->m_particleSystem->CreateParticle( entity->m_pos, vel, Particle::TypeControlFlash );
    }


    return *spawned[0];
}


Darwinian* Task::FindDarwinian( Vector3 const &_pos )
{
    int teamId = g_app->m_globalWorld->m_myTeamId;

    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( _pos.x, _pos.z, 10.0f, &numFound, teamId );
    Darwinian* nearestId = nullptr;
    float nearest = 99999.9f;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity &&
            entity->m_type == Entity::TypeDarwinian )
        {
            float distance = ( entity->m_pos - _pos ).MagSquared();
            if( distance < nearest )
            {
                nearestId = static_cast<Darwinian*>(entity);
                nearest = distance;
            }
        }
    }

    return nearestId;
}


void Task::TargetOfficer( Vector3 const &_pos )
{
    //
    // We will not upgrade people if we're controlling something right now

    Team *myTeam = g_app->m_location->GetMyTeam();
    if( myTeam->m_currentUnitId != -1 ||
        myTeam->m_currentEntityId != -1 ||
        myTeam->m_currentBuildingId != -1 )
    {
        return;
    }


    //
    // Find the nearest friendly Darwinian to upgrade to an Officer
    // If we found someone, promote them
    // Then shutdown this task
    // Then select them

    if( Darwinian* nearest = FindDarwinian( _pos ))
    {
        auto& promoted = Promote( *nearest );
        g_app->m_taskManager->TerminateTask( m_id );
        g_app->m_location->m_teams[ promoted.m_id.GetTeamId() ].SelectUnit( promoted.m_id.GetUnitId(), promoted.m_id.GetIndex(), -1 );
        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, GlobalResearch::TypeOfficer, 2.5f );

        g_app->m_soundSystem->TriggerOtherEvent( nullptr, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
    }
}


bool Task::Advance()
{
    if( m_state == StateRunning )
    {
        switch( m_type )
        {
            case GlobalResearch::TypeSquad:
            case GlobalResearch::TypeController:
            {
                if( !m_unit || m_unit->NumAliveEntities() == 0 )
                {
                    if( g_app->m_taskManager->m_currentTaskId == m_id )
                    {
                        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, m_type, 3.0f );
                    }
                    return true;
                }
                break;
            }

            case GlobalResearch::TypeEngineer:
            case GlobalResearch::TypeArmour:
            {
                if( !m_entity || m_entity->m_dead )
                {
                    if( g_app->m_taskManager->m_currentTaskId == m_id )
                    {
                        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, m_type, 3.0f );
                    }
                    return true;
                }
                break;
            }

            case GlobalResearch::TypeOfficer:
            {
                m_state = StateStarted;
                break;
            }
        }
    }

    return( m_state == StateStopping );
}


void Task::SwitchTo()
{
    if( g_app->m_camera->IsInModeRadarAim() ||
        g_app->m_camera->IsInModeTurretAim() )
    {
        g_app->m_camera->RequestFreeMovementMode();
    }

    int teamId = g_app->m_globalWorld->m_myTeamId;

    switch( m_type )
    {
        case GlobalResearch::TypeSquad:
        {
            if (m_unit) g_app->m_location->m_teams[teamId].SelectUnit( m_unit->m_unitId, -1, -1 );
            else        g_app->m_location->m_teams[teamId].SelectUnit( -1, -1, -1 );
            break;
        }

        case GlobalResearch::TypeEngineer:
        case GlobalResearch::TypeArmour:
        {
            if(m_entity) g_app->m_location->m_teams[teamId].SelectUnit( -1, m_entity->m_id.GetIndex(), -1 );
            else         g_app->m_location->m_teams[teamId].SelectUnit( -1, -1, -1 );
            break;
        }

        case GlobalResearch::TypeController:
        {
            for( int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i )
            {
                Task *task = g_app->m_taskManager->m_tasks.GetData(i);
                if( task->m_type == GlobalResearch::TypeSquad &&
                    task->m_unit == m_unit )
                {
                    g_app->m_taskManager->SelectTask( task->m_id );
                    break;
                }
            }
            break;
        }

        case GlobalResearch::TypeOfficer:
        {
            g_app->m_location->m_teams[teamId].SelectUnit( -1, -1, -1 );
            break;
        }
    }
}


void Task::Stop()
{
    switch( m_type )
    {
        case GlobalResearch::TypeSquad:
        {
            if( m_unit )
            {
                for( int i = 0; i < m_unit->m_entities.Size(); ++i )
                {
                    if( m_unit->m_entities.ValidIndex(i) )
                    {
                        Entity *entity = m_unit->m_entities[i];
                        entity->ChangeHealth( -1000 );
                    }
                }
            }
            break;
        }

        case GlobalResearch::TypeEngineer:
        case GlobalResearch::TypeArmour:
        {
            if( m_entity )
            {
                m_entity->ChangeHealth( -1000 );
            }
            break;
        }
    }

    m_state = StateStopping;
}


const char *Task::GetTaskName( int _type )
{
    return GlobalResearch::GetTypeName( _type );
}


const char *Task::GetTaskNameTranslated( int _type )
{
    return GlobalResearch::GetTypeNameTranslated( _type );
}


// ============================================================================



TaskManager::TaskManager()
:   m_nextTaskId(0),
    m_currentTaskId(-1),
    m_verifyTargetting(true)
{
}


bool TaskManager::RunTask( Task *_task )
{
    if( CapacityUsed() < Capacity() )
    {
        _task->m_id = m_nextTaskId;
        ++m_nextTaskId;
        m_tasks.PutDataAtEnd( _task );
        _task->Start();
        m_currentTaskId = _task->m_id;

        return true;
    }
    else
    {
        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageFailure, -1, 2.5f );
        g_app->m_sepulveda->Say( "task_manager_full" );
    }

    return false;
}


bool TaskManager::RunTask( int _type )
{
    switch( _type )
    {
        case GlobalResearch::TypeSquad:
        case GlobalResearch::TypeEngineer:
        case GlobalResearch::TypeOfficer:
#ifndef DEMOBUILD
        case GlobalResearch::TypeArmour:
#endif
        {
            Task *task = new Task();
            task->m_type = _type;
            bool success = RunTask( task );
            if( success )
            {
                int teamId = g_app->m_globalWorld->m_myTeamId;
                g_app->m_location->m_teams[teamId].SelectUnit( -1, -1, -1 );
            }
            return success;
        }

        case GlobalResearch::TypeController:
        {
            Task *task = GetCurrentTask();
            if( task && task->m_type == GlobalResearch::TypeSquad )
            {
                Unit *unit = task->m_unit;
                if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
                {
                    InsertionSquad *squad = (InsertionSquad *) unit;
                    squad->SetWeaponType( _type );

                    if( !GetTask( squad->m_controllerId ) )
                    {
                        Task *controller = new Task();
                        controller->m_type = _type;
                        controller->m_unit = squad;
                        controller->m_entity = nullptr;
                        controller->m_route = new Route(-1);
                        controller->m_route->AddWayPoint( squad->m_centrePos );
                        bool success = RunTask( controller );
                        if( success )
                        {
                            squad->m_controllerId = controller->m_id;
                            SelectTask( task->m_id );
                            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, _type, 2.5f );
                        }
                        return success;
                    }

                    return true;
                }
            }

            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageFailure, -1, 2.5f );
            g_app->m_sepulveda->Say( "task_manager_needsquadfirst" );
            return false;
        }

        case GlobalResearch::TypeGrenade:
        case GlobalResearch::TypeRocket:
        case GlobalResearch::TypeAirStrike:
        {
            Task *task = GetCurrentTask();
            if( task && task->m_type == GlobalResearch::TypeSquad )
            {
                Unit *unit = task->m_unit;
                if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
                {
                    InsertionSquad *squad = (InsertionSquad *) unit;
                    squad->SetWeaponType( _type );
                    g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, _type, 2.5f );
                    return true;
                }
            }

            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageFailure, -1, 2.5f );
            g_app->m_sepulveda->Say( "task_manager_needsquadfirst" );
            return false;
        }
    }

    return false;
}


bool TaskManager::RegisterTask( Task *_task )
{
    if( CapacityUsed() < Capacity() )
    {
        _task->m_id = m_nextTaskId;
        ++m_nextTaskId;
        m_tasks.PutDataAtEnd( _task );
        return true;
    }

    return false;
}


bool TaskManager::TerminateTask( int _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_id == _id )
        {
            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, task->m_type, 3.0f );
            m_tasks.RemoveData(i);
            task->Stop();
            delete task;
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::TaskShutdown );

            if( m_currentTaskId == _id )
            {
                m_currentTaskId = -1;
            }

            return true;
        }
    }

    return false;
}


int TaskManager::Capacity()
{
    int taskManagerResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeTaskManager );
    int capacity = 1 + taskManagerResearch;
    return capacity;
}


int TaskManager::CapacityUsed()
{
    return m_tasks.Size();
}


int TaskManager::MapGestureToTask( int _gestureId )
{
    switch( _gestureId )
    {
        case 0 :        return GlobalResearch::TypeSquad;
        case 1 :        return GlobalResearch::TypeEngineer;
        case 2 :        return GlobalResearch::TypeGrenade;
        case 3 :        return GlobalResearch::TypeRocket;
        case 4 :        return GlobalResearch::TypeAirStrike;
        case 5 :        return GlobalResearch::TypeController;
        case 6 :        return GlobalResearch::TypeOfficer;
        case 7 :        return GlobalResearch::TypeArmour;

        default :       return -1;
    }
}


void TaskManager::AdvanceTasks()
{
    //
    // Are we currently placing a task?

    Task *currentTask = GetCurrentTask();
    if( currentTask && currentTask->m_state == Task::StateStarted )
    {
        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, currentTask->m_type, 2.5f );
    }


    //
    // Advance all other tasks

    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        bool amIDone = task->Advance();
        if( amIDone )
        {
            if( m_currentTaskId == task->m_id )
            {
                m_currentTaskId = -1;
                int teamId = g_app->m_globalWorld->m_myTeamId;
                g_app->m_location->m_teams[teamId].SelectUnit( -1, -1, -1 );
            }

            m_tasks.RemoveData(i);
            --i;
            delete task;
        }
    }
}


void TaskManager::StopAllTasks()
{
    m_tasks.EmptyAndDelete();
    m_currentTaskId = -1;
}


void TaskManager::Advance()
{
    AdvanceTasks();

    g_app->m_globalWorld->m_research->AdvanceResearch();
}


void TaskManager::SelectTask( int _id )
{
    m_currentTaskId = _id;
    if( m_currentTaskId != -1 )
    {
        int currentIndex = -1;
        for( int i = 0; i < m_tasks.Size(); ++i )
        {
            Task *task = m_tasks[i];
            if( task->m_id == m_currentTaskId )
            {
                currentIndex = i;
                break;
            }
        }

        DarwiniaReleaseAssert( currentIndex != -1, "Error in TaskManager::SelectTask. Tried to select a task that doesn't exist." );

        Task *task = m_tasks[ currentIndex ];
        //m_tasks.RemoveData(currentIndex);
        //m_tasks.PutDataAtStart( task );
        task->SwitchTo();

		g_app->m_gameCursor->BoostSelectionArrows(2.0f);
    }
}


void TaskManager::SelectTask( WorldObjectId _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( (task->m_entity &&
            task->m_entity->m_id.GetTeamId() == _id.GetTeamId() &&
            task->m_entity->m_id.GetUnitId() == _id.GetUnitId() &&
            task->m_entity->m_id.GetIndex() == _id.GetIndex()) ||

            (task->m_unit &&
            task->m_unit->m_teamId == _id.GetTeamId() &&
            task->m_unit->m_unitId == _id.GetUnitId() &&
            _id.GetIndex() -1
            )
         )
        {
            SelectTask( task->m_id );
            break;
        }
    }
}


Task *TaskManager::GetCurrentTask ()
{
    return GetTask( m_currentTaskId );
}


Task *TaskManager::GetTask( int _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_id == _id )
        {
            return task;
        }
    }

    return nullptr;
}


Task *TaskManager::GetTask( WorldObjectId _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_entity && task->m_entity->m_id == _id )
        {
            return task;
        }
    }

    return nullptr;
}


bool TaskManager::TargetTask( int _id, Vector3 const &_pos )
{
	if (IsValidTargetArea(_id, _pos))
	{
		Task *task = GetTask( _id );
        task->Target( _pos );
		return true;
	}

	return false;
}


bool TaskManager::IsValidTargetArea( int _id, Vector3 const &_pos )
{
    Task *task = g_app->m_taskManager->GetTask( _id );
    if( !task ) return false;

    if( !g_app->m_location || !g_app->m_location->m_landscape.IsInLandscape( _pos ) ) return false;

    if( m_verifyTargetting )
    {
        if( task->m_type == GlobalResearch::TypeOfficer )
        {
            int numFound;
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( _pos.x, _pos.z, 10.0f, &numFound, g_app->m_globalWorld->m_myTeamId );
            bool foundDarwinian = false;
            for( int i = 0; i < numFound; ++i )
            {
                WorldObjectId id = ids[i];
                Entity *ent = g_app->m_location->GetEntity( id );
                if( ent && ent->m_type == Entity::TypeDarwinian )
                {
                    foundDarwinian = true;
                    break;
                }
            }
            return foundDarwinian;
        }
        else
        {
            LList <TaskTargetArea> *targetAreas = GetTargetArea( _id );
            bool success = false;

            for( int i = 0; i < targetAreas->Size(); ++i )
            {
                TaskTargetArea *targetArea = targetAreas->GetPointer(i);
                if( (_pos - targetArea->m_centre).Mag() <= targetArea->m_radius )
                {
                    success = true;
                    break;
                }
            }

            delete targetAreas;
            return success;
        }
    }
    else
    {
        return true;
    }

    return false;
}


LList <TaskTargetArea> *TaskManager::GetTargetArea( int _id )
{
    LList<TaskTargetArea> *result = new LList<TaskTargetArea>();

    Task *task = GetTask( _id );
    if( task )
    {
        switch( task->m_type )
        {
            case GlobalResearch::TypeArmour:
            {
                for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
                {
                    if( g_app->m_location->m_buildings.ValidIndex(i) )
                    {
                        Building *building = g_app->m_location->m_buildings[i];
                        if( building &&
                            building->m_type == Building::TypeTrunkPort &&
                            ((TrunkPort *)building)->m_openTimer > 0.0f )
                        {
                            TaskTargetArea tta;
                            tta.m_centre = building->m_pos;
                            tta.m_radius = 120.0f;
                            tta.m_stationary = true;
                            result->PutData( tta );
                        }
                    }
                }
                break;
            }

            case GlobalResearch::TypeEngineer:
            {
                Team *team = g_app->m_location->GetMyTeam();
                for( int i = 0; i < team->m_units.Size(); ++i )
                {
                    if( team->m_units.ValidIndex(i) )
                    {
                        Unit *unit = team->m_units[i];
                        if( unit->m_troopType == Entity::TypeInsertionSquadie )
                        {
                            TaskTargetArea tta;
                            tta.m_centre = unit->m_centrePos;
                            tta.m_radius = 100.0f;
                            tta.m_stationary = false;
                            result->PutData( tta );
                        }
                    }
                }
                //break;                // DELIBERATE FALL THROUGH
            }
            //fallthrough

            case GlobalResearch::TypeSquad:
                for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
                {
                    if( g_app->m_location->m_buildings.ValidIndex(i) )
                    {
                        Building *building = g_app->m_location->m_buildings[i];
                        if( building &&
                            building->m_type == Building::TypeControlTower &&
                            building->m_id.GetTeamId() == g_app->m_location->GetMyTeam()->m_teamId )
                        {
                            TaskTargetArea tta;
                            tta.m_centre = building->m_pos;
                            tta.m_radius = 75.0f;
                            tta.m_stationary = true;
                            result->PutData( tta );
                        }
                    }
                }
                break;

            case GlobalResearch::TypeOfficer:
            {
                TaskTargetArea tta;
                tta.m_centre.Set(0, 0, 0);
                tta.m_radius = 99999.9f;
                result->PutData( tta );
                break;
            }
        }
    }

    return result;
}
