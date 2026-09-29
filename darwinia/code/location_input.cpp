#include <float.h>

#include <eclipse.h>

#include "lib/text_renderer.h"
#include "lib/targetcursor.h"
#include "lib/input/input.h"

#include "network/clienttoserver.h"

#include "sound/soundsystem.h"

#include "worldobject/building.h"
#include "worldobject/switch.h"
#include "worldobject/radardish.h"
#include "worldobject/officer.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/armour.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "location_input.h"
#include "unit.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "helpsystem.h"
#include "sepulveda.h"
#include "taskmanager_interface.h"
#include "script.h"

#include "FFP_emulation.h"
#include "worldobject/worldobject.h"

// *** AdvanceTeleportControl
void LocationInput::AdvanceRadarDishControl(Building *_building)
{
	if( g_inputManager.controlEvent( ControlUnitSetTarget ) )
    {
        Vector3 rayStart;
        Vector3 rayDir;
        g_app->m_camera->GetClickRay(g_target->X(), g_target->Y(),
									 &rayStart, &rayDir);

        auto* building = g_app->m_location->GetBuilding(rayStart, rayDir, 255);
        if( building &&
            building->m_type == Building::TypeRadarDish &&
            building != _building )
        {
            RadarDish *dish = (RadarDish *) building;
            g_app->m_clientToServer->RequestAimBuilding( g_app->m_globalWorld->m_myTeamId,
                                                         _building->m_id.GetUniqueId(),
                                                         dish->GetStartPoint() );
        }
        else
        {
            g_app->m_clientToServer->RequestAimBuilding( g_app->m_globalWorld->m_myTeamId,
													     _building->m_id.GetUniqueId(),
													     g_app->m_userInput->GetMousePos3d() );
        }
    }
}

WorldObjectOrUnit LocationInput::GetObjectUnderMouse(int _teamId )
{
    Vector3 rayStart;
    Vector3 rayDir;
    g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(),
								  &rayStart, &rayDir );

    // Find any objects the ray intersects
 	float buildDist = FLT_MAX;
	float unitDist = FLT_MAX;
	float entDist = FLT_MAX;

    auto* building = g_app->m_location->GetBuilding(rayStart, rayDir, _teamId, FLT_MAX, &buildDist );
	auto* unit = g_app->m_location->GetUnit( rayStart, rayDir, _teamId, &unitDist );
    auto* entity = g_app->m_location->GetEntity( rayStart, rayDir, _teamId, &entDist );

    //
    // Look for Darwinians if we are running an officer program
    if( !entity )
    {
	    Task *task = g_app->m_taskManager->GetCurrentTask();
        if( task &&
            task->m_state == Task::StateStarted &&
            task->m_type == GlobalResearch::TypeOfficer )
        {
            Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
            entity = Task::FindDarwinian( mousePos );
            entDist = 0.0f;
        }
    }

    //
    // Now find which object is nearest

    if( entity && entDist < unitDist && entDist < buildDist )
    {
        // Entity is nearest
        return entity;
    }

    if( unit && unitDist < entDist && unitDist < buildDist )
    {
        // Unit is nearest
        return unit;
    }

    if( building  && buildDist < entDist && buildDist < unitDist )
    {
        // Building is nearest
        return building;
    }

    return nullptr;
}

// *** AdvanceNoSelection
void LocationInput::AdvanceNoSelection()
{
	if (g_inputManager.controlEvent( ControlSelectLocation ) &&
        !g_app->m_taskManagerInterface->m_visible )
    {
        WorldObjectId id;

        auto object = GetObjectUnderMouse( g_app->m_globalWorld->m_myTeamId );

        if(auto* world_object = object.GetWorldObject())
        {
            id = world_object->m_id;
        }

        if( id.IsValid() )
        {
			if( id.GetUnitId() == UNIT_BUILDINGS )
            {
                Task *currentTask = g_app->m_taskManager->GetCurrentTask();

                if( !currentTask )
                {
        		    Building *building = g_app->m_location->GetBuilding(id.GetUniqueId());

                    if( building->m_type == Building::TypeRadarDish )
                    {
                        g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), -1, -1, id.GetUniqueId() );
                        g_app->m_camera->RequestRadarAimMode( *building );
                    }
                    else if( building->m_type == Building::TypeGunTurret )
                    {
                        g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), -1, -1, id.GetUniqueId() );
                        g_app->m_camera->RequestTurretAimMode( *building );
                    }
                    else if( building->m_type == Building::TypeFenceSwitch )
                    {
                        FenceSwitch *fs = (FenceSwitch *)building;
                        fs->Switch();
                    }
                }
			}
			else
			{
                g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), id.GetUnitId(), id.GetIndex(), -1 );
			}
        }
	}
}


/*
void LocationInput::AdvanceNoSelection()
{
	if (g_inputManager.m_lmbClicked)
    {
        Vector3 rayStart;
        Vector3 rayDir;
		g_app->m_camera->GetClickRay(g_inputManager.m_mouseX, g_inputManager.m_mouseY,
									 &rayStart, &rayDir);

        // Find any objects the ray intersects
        int buildId = g_app->m_location->GetBuildingId(rayStart, rayDir, g_app->m_globalWorld->m_myTeamId);
		int unitId = g_app->m_location->GetUnitId( rayStart, rayDir, g_app->m_globalWorld->m_myTeamId );
        WorldObjectId entId = g_app->m_location->GetEntityId( rayStart, rayDir, g_app->m_globalWorld->m_myTeamId );

		// Now find which object is nearest
		Team *team = g_app->m_location->GetMyTeam();
		float buildDist = FLT_MAX;
		float unitDist = FLT_MAX;
		float entDist = FLT_MAX;
		if (buildId != -1) buildDist = (rayStart - g_app->m_location->GetBuilding(buildId)->m_pos).Mag();
		if (unitId != -1) unitDist = (rayStart - team->m_units.GetData(unitId)->m_centrePos).Mag();
		if (entId.IsValid())  entDist = (rayStart - g_app->m_location->GetEntity(entId)->m_pos).Mag();

		if (buildId != -1 || unitId != -1 || entId.IsValid() )
		{
			g_inputManager.m_lmbClicked = false;
		}

        if( buildDist < unitDist )
        {
			if (buildDist < entDist)
			{
				// Building is nearest
				Building *building = g_app->m_location->GetBuilding(buildId);

                if( building->m_type == Building::TypeFactory )
                {
                    g_app->m_camera->RequestBuildingFocusMode( building );
                    g_app->m_userInput->m_stretchyIcons->RequestMenu( StretchyIcons::MenuMicroUnit );
                    g_app->m_userInput->m_stretchyIcons->Enable();
                    g_app->m_clientToServer->RequestSelectUnit( g_app->m_globalWorld->m_myTeamId, -1, -1, building->m_id.GetUniqueId() );
                }
                else if( building->m_type == Building::TypeRadarDish )
                {
                    g_app->m_camera->RequestRadarAimMode( building );
                    g_app->m_clientToServer->RequestSelectUnit( g_app->m_globalWorld->m_myTeamId, -1, -1, building->m_id.GetUniqueId() );
                }
				else if(building->m_type == Building::TypePowerstation)
				{
                    g_app->m_camera->RequestBuildingFocusMode( building );
                    g_app->m_userInput->m_stretchyIcons->RequestMenu( StretchyIcons::MenuPowerstation );
                    g_app->m_userInput->m_stretchyIcons->Enable();
                    g_app->m_clientToServer->RequestSelectUnit( g_app->m_globalWorld->m_myTeamId, -1, -1, building->m_id.GetUniqueId() );
				}
			}
			else
			{
				// Entity is nearest
                g_app->m_clientToServer->RequestSelectUnit( g_app->m_globalWorld->m_myTeamId, -1, entId.GetIndex(), -1 );
			}
        }
        else
        {
            if (entDist < unitDist)
            {
				// Entity is nearest
                g_app->m_clientToServer->RequestSelectUnit( g_app->m_globalWorld->m_myTeamId, -1, entId.GetIndex(), -1 );
            }
			else
			{
				// Unit is nearest
				g_app->m_clientToServer->RequestSelectUnit( g_app->m_globalWorld->m_myTeamId, unitId, -1, -1 );
			}
        }
	}
}*/



// *** AdvanceTeamControl
void LocationInput::AdvanceTeamControl()
{
	bool inCutscene = false;
	if( g_app->m_script->IsRunningScript() &&
		g_app->m_script->m_permitEscape ) inCutscene = true;
	if( g_app->m_camera->IsInModeBuildingFocus() ) inCutscene = true;

	if( inCutscene ) return;


	Task *task = g_app->m_taskManager->GetCurrentTask();
	bool taskStarted = task && task->m_state == Task::StateStarted;

    //
    // Have we clicked to select something?

	if ( g_inputManager.controlEvent( ControlUnitSetTarget ) && // TODO: Really?
		//!g_inputManager.m_rmb &&
        !g_app->m_taskManagerInterface->m_visible &&
		!taskStarted)
    {
        auto object_or_unit =  GetObjectUnderMouse(g_app->m_globalWorld->m_myTeamId );
        if( object_or_unit.HasValue() && object_or_unit.id().GetUnitId() != 255 && object_or_unit.id().GetUnitId() != UNIT_BUILDINGS )
        {
            g_app->m_clientToServer->RequestSelectUnit( object_or_unit.id().GetTeamId(), object_or_unit.id().GetUnitId(), object_or_unit.id().GetIndex(), -1 );
            return;
        }
    }


	Team *team = g_app->m_location->GetMyTeam();

    // Space key should deselect current unit or building
    bool objectSelected = team->m_currentUnitId != -1 ||
                          team->m_currentEntityId != -1 ||
                          team->m_currentBuildingId != -1;

	if ( g_inputManager.controlEvent( ControlUnitDeselect ) )
    {
        if( objectSelected )
        {
            g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
            g_app->m_camera->RequestFreeMovementMode();
            g_app->m_taskManager->m_currentTaskId = -1;

            if( team->m_currentUnitId != -1 )
            {
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadDeselect );
            }
            else if( team->m_currentEntityId != -1 )
            {
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::EngineerDeselect );
            }
        }
    }

    if( taskStarted && !g_app->m_taskManagerInterface->m_visible )
    {
		if( g_inputManager.controlEvent( ControlUnitCreate ) )
        {
            Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
            g_app->m_clientToServer->RequestTargetProgram( g_app->m_globalWorld->m_myTeamId,
                                                           g_app->m_taskManager->m_currentTaskId,
                                                           mousePos );
        }
    }


	if( team->m_currentUnitId == -1 )
    {
        if( team->m_currentEntityId == -1 )
	    {
		    if (team->m_currentBuildingId == -1)
            {
			    AdvanceNoSelection();
            }
		    else
		    {
			    Building *building = g_app->m_location->GetBuilding(team->m_currentBuildingId);
				if (!building)
                {
                    team->m_currentBuildingId = -1;
                }
				else if (building->m_type == Building::TypeRadarDish)
			    {
				    AdvanceRadarDishControl(building);
			    }
				else if( building->m_type == Building::TypeGunTurret)
                {
					if( g_app->m_taskManagerInterface->ControlEvent( TaskManagerInterface::TMTerminate ) )
                    {
                        // Player pressed CTRL-C, so terminate this turret
                        g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
                        g_app->m_camera->RequestFreeMovementMode();
                        building->Damage( -100 );
                    }
                }
		    }
        }
        else
        {
            Entity *ent = team->GetMyEntity();
            if( !ent )
            {
                team->m_currentEntityId = -1;
            }
            else if( ent->m_type == Entity::TypeOfficer )
            {
                if( g_app->m_taskManagerInterface->ControlEvent( TaskManagerInterface::TMTerminate ) )
                {
                    // Player pressed CTRL-C, so demote this officer
                    g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
                    g_app->m_camera->RequestFreeMovementMode();
                    ent->ChangeHealth( -999 );
                }

                Officer *officer = (Officer *)ent;
				if( g_inputManager.controlEvent( ControlWeaponCycleLeft ) )
                {
                    officer->SetPreviousMode();
                }
				else if( g_inputManager.controlEvent( ControlWeaponCycleRight ) )
                {
                    officer->SetNextMode();
                }
            }
            else if( ent->m_type == Entity::TypeArmour )
            {
				if( g_inputManager.controlEvent( ControlWeaponCycleLeft ) ||
					g_inputManager.controlEvent( ControlWeaponCycleRight ) )
                {
                    Armour *armour = (Armour *)ent;
                    armour->SetDirectOrders();
                }
            }
        }
    }
	else
	{
		// Controlling a unit
        if( team->m_units.ValidIndex(team->m_currentUnitId) )
        {
		    Unit *unit = team->m_units.GetData(team->m_currentUnitId);
            if( unit->m_troopType == Entity::TypeInsertionSquadie )
            {
                if( !g_app->m_taskManagerInterface->m_visible )
                    {
                    InsertionSquad *squad = (InsertionSquad *)unit;
                    int currentWeapon = -1;
                    LList<int> weaponList;
                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade ) )
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeGrenade ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeGrenade );
                    }
                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeRocket ) )
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeRocket ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeRocket );
                    }

                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeAirStrike ) )
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeAirStrike ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeAirStrike );
                    }
                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeController ) )
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeController ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeController );
                    }

                    if( weaponList.Size() > 1 )
                    {
                        int oldWeapon = currentWeapon;
						if( g_inputManager.controlEvent( ControlWeaponCycleLeft ) )
                        {
                            currentWeapon--;
                            if( currentWeapon < 0 )
                            {
                                currentWeapon = weaponList.Size() - 1;
                            }
                        }

						if( g_inputManager.controlEvent( ControlWeaponCycleRight ) )
                        {
                            currentWeapon++;
                            if( currentWeapon >= weaponList.Size() )
                            {
                                currentWeapon = 0;
                            }
                        }

                        if( oldWeapon != currentWeapon )
                        {
                            g_app->m_clientToServer->RequestRunProgram( squad->m_teamId, weaponList[currentWeapon] );
                            g_app->m_soundSystem->TriggerOtherEvent( nullptr, "GestureBegin", SoundSourceBlueprint::TypeGesture );
                            g_app->m_soundSystem->TriggerOtherEvent( nullptr, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
                        }
                    }
                }
            }
        }
        else
        {
			g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
			g_app->m_camera->RequestFreeMovementMode();
		}
	}
}


void LocationInput::Advance()
{
	bool chatLog = g_app->m_sepulveda->ChatLogVisible();

    if( g_app->m_globalWorld->m_myTeamId != 255 &&
        EclGetWindows()->Size() == 0 &&
        !chatLog )
    {
        AdvanceTeamControl();
    }
}


void LocationInput::Render()
{
    g_editorFont.BeginText2D();
	if (g_app->m_location->GetMyTeam())
	{
        glColor4ubv(g_app->m_location->GetMyTeam()->m_colour.GetData());
//		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		g_editorFont.DrawText2D( 12, 19, DEF_FONT_SIZE,
			"You are TEAM %d", (int)g_app->m_globalWorld->m_myTeamId );
	}

    g_editorFont.EndText2D();
}
