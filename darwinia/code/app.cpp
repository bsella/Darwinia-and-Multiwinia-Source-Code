#include <stdlib.h>
#include <stdio.h>
//#include <shlobj.h>

#include "network/clienttoserver.h"

#include "lib/language_table.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/system_info.h"
#include "lib/text_renderer.h"
#include "lib/filesys_utils.h"
#include "lib/bitmap.h"
#include "interface/prefs_other_window.h"

#include "sound/sound_stream_decoder.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "gesture.h"
#include "global_world.h"
#include "helpsystem.h"
#include "tutorial.h"
#include "location.h"
#include "location_input.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#include "sepulveda.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface_gestures.h"
#include "taskmanager_interface_icons.h"
#include "gamecursor.h"
#include "level_file.h"
#include "control_help.h"
#include "game_menu.h"

void SetPreferenceOverrides(); // See main.cpp

App *g_app = nullptr;


#ifdef DEMO2
    #define GAMEDATAFILE "game_demo2.txt"
#else
    #ifdef DEMOBUILD
        #define GAMEDATAFILE "game_demo.txt"
    #else
        #define GAMEDATAFILE "game.txt"
    #endif
#endif

App::App()
:
	m_userInput(nullptr),
	m_resource(nullptr),
	m_soundSystem(nullptr),
	m_particleSystem(nullptr),
	m_langTable(nullptr),
	m_aviGenerator(nullptr),
	m_profiler(nullptr),
	m_globalWorld(nullptr),
	m_location(nullptr),
	m_locationId(-1),
	m_camera(nullptr),
    m_server(nullptr),
    m_clientToServer(nullptr),
    m_renderer(nullptr),
	m_locationInput(nullptr),
	m_locationEditor(nullptr),
	m_helpSystem(nullptr),
	m_tutorial(nullptr),
	m_effectProcessor(nullptr),
    m_taskManager(nullptr),
    m_gesture(nullptr),
    m_script(nullptr),
    m_sepulveda(nullptr),
    m_testHarness(nullptr),
	m_startSequence(nullptr),
	m_demoEndSequence(nullptr),
	m_attractMode(nullptr),
	m_controlHelpSystem(nullptr),
	m_gameMenu(nullptr),
	m_negativeRenderer(false),
	m_difficultyLevel(0),
	m_largeMenus(false),
    m_paused(false),
	m_editing(false),
	m_requestedLocationId(-1),
	m_requestToggleEditing(false),
	m_requestQuit(false),
	m_levelReset(false),
    m_atMainMenu(false),
    m_gameMode(GameModeNone)
{
    g_app = this;

	// Load resources

    m_resource = new Resource();
    m_resource->ParseArchive( "main.dat", nullptr );
    m_resource->ParseArchive( "sounds.dat", nullptr );
	m_resource->ParseArchive( "patch.dat", nullptr );
    m_resource->ParseArchive( "language.dat", nullptr );

	g_prefsManager = new PrefsManager(App::GetPreferencesPath());
	SetPreferenceOverrides();

    m_bypassNetworking  = g_prefsManager->GetInt("BypassNetwork") ? true : false;

    m_negativeRenderer  = g_prefsManager->GetInt("RenderNegative", 0) ? true : false;
    if( m_negativeRenderer )    m_backgroundColour.Set(255,255,255,255);
    else                        m_backgroundColour.Set(0,0,0,0);

	UpdateDifficultyFromPreferences();


#ifdef PROFILER_ENABLED
    m_profiler          = new Profiler();
#endif

    m_renderer          = new Renderer();
    m_renderer->Initialise();

	// Make sure that resources are now available - either the .dat files
	// or the data directory must exist

	SoundStreamDecoder *ssd = m_resource->GetSoundStreamDecoder("sounds/ablaster");
	DarwiniaReleaseAssert(ssd,
		"Couldn't find sound resources. This is probably because\n"
		"sounds.dat isn't in the working directory.");
	delete ssd;

	//int textureId =
	m_resource->GetTexture("textures/editor_font_normal.bmp");

    m_gameCursor        = new GameCursor();
    m_soundSystem       = new SoundSystem();
    m_clientToServer    = new ClientToServer();
    m_userInput         = new UserInput();
//    m_location          = new Location();
//    m_locationInput		= new LocationInput();


	m_camera            = new Camera();
    m_gameMenu          = new GameMenu();

    strcpy( m_gameDataFile, "game.txt" );



    //
    // Determine default language if possible

	const char *language = g_prefsManager->GetString("TextLanguage");
    if( stricmp(language, "unknown") == 0 )
    {
		const char *defaultLang = g_systemInfo->m_localeInfo.m_language.c_str();
        char langFilename[512];
        sprintf( langFilename, "data/language/%s.txt", defaultLang );
        if( DoesFileExist(langFilename) )
        {
            g_prefsManager->SetString( "TextLanguage", defaultLang );
        }
        else
        {
            g_prefsManager->SetString( "TextLanguage", "english" );
        }
    }
    language = g_prefsManager->GetString("TextLanguage");

    SetLanguage( language, g_prefsManager->GetInt( "TextLanguageTest", 0 ) );



#ifdef SOUND_EDITOR
	m_effectProcessor   = new EffectProcessor();
#endif // SOUND_EDITOR

    SetProfileName( g_prefsManager->GetString("UserProfile", "none") );

    m_helpSystem        = new HelpSystem();
	m_particleSystem	= new ParticleSystem();
    m_taskManager       = new TaskManager();
    m_gesture           = new Gesture("gestures.txt");
    m_sepulveda         = new Sepulveda();
    m_script            = new Script();
#ifdef ATTRACTMODE_ENABLED
	m_attractMode		= new AttractMode();
#endif
    m_controlHelpSystem = new ControlHelpSystem();

    if( g_prefsManager->GetInt( "ControlMethod" ) == 0 )
    {
        m_taskManagerInterface = new TaskManagerInterfaceGestures();
    }
    else
    {
        m_taskManagerInterface = new TaskManagerInterfaceIcons();
    }

	m_soundSystem->Initialise();

#ifdef DEMOBUILD
    #ifdef DEMO2
        m_tutorial = new Demo2Tutorial();
    #else
        m_tutorial = new Demo1Tutorial();
    #endif
#endif

    int menuOption = g_prefsManager->GetInt( OTHER_LARGEMENUS, 0 );
	if( menuOption == 2 ) // (todo) or is running in media center and tenFootMode == -1
	{
		m_largeMenus = true;
	}

    //
    // Load mods

	const char *modName = g_prefsManager->GetString("Mod", "none" );
    if( stricmp( modName, "none" ) != 0 )
    {
        g_app->m_resource->LoadMod( modName );
    }


    //
    // Load save games

	LoadProfile();
}

App::~App()
{
	delete m_globalWorld;
	delete m_langTable;
#ifdef DEMOBUILD
	delete m_tutorial;
#endif
	delete m_taskManagerInterface;
	delete m_controlHelpSystem;
#ifdef ATTRACTMODE_ENABLED
	delete m_attractMode;
#endif
	delete m_script;
	delete m_sepulveda;
	delete m_gesture;
	delete m_taskManager;
	delete m_particleSystem;
	delete m_helpSystem;
#ifdef SOUND_EDITOR
	delete m_effectProcessor;
#endif // SOUND_EDITOR
	delete m_camera;
	delete m_userInput;
	delete m_clientToServer;
	delete m_soundSystem;
	delete m_gameCursor;
	delete m_renderer;
#ifdef PROFILER_ENABLED
	delete m_profiler;
#endif
	delete g_prefsManager;
	delete m_resource;
}

void App::UpdateDifficultyFromPreferences()
{
	// This method is called to make sure that the difficulty setting
	// used to control the game play (g_app->m_difficultyLevel) is
	// consistent with the user preferences.

	// Preferences value is 1-based, m_difficultyLevel is 0-based.
	m_difficultyLevel = g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
	if( m_difficultyLevel < 0 )
	{
		m_difficultyLevel = 0;
	}
}


void App::SetLanguage( const char *_language, bool _test )
{
    //
    // Delete existing language data

    if( m_langTable )
    {
        delete m_langTable;
        m_langTable = nullptr;
    }


    //
    // Load the language text file

    char langFilename[256];
#if defined(TARGET_OS_LINUX) && defined(TARGET_DEMOGAME)
	sprintf( langFilename, "language/%s_demo.txt", _language );
#else
    sprintf( langFilename, "language/%s.txt", _language );
#endif

	m_langTable = new LangTable(langFilename);

    if( _test )
    {
        m_langTable->TestAgainstEnglish();
    }

    //
    // Load the MOD language file if it exists

    sprintf( langFilename, "strings_%s.txt", _language );
    TextReader *modLangFile = g_app->m_resource->GetTextReader(langFilename);
    if( !modLangFile )
    {
        sprintf( langFilename, "strings_default.txt" );
        modLangFile = g_app->m_resource->GetTextReader(langFilename);
    }

    if( modLangFile )
    {
        delete modLangFile;
        m_langTable->ParseLanguageFile( langFilename );
    }

    //
    // Load localised fonts if they exist

    char fontFilename[256];
    sprintf( fontFilename, "textures/speccy_font_%s.bmp", _language );
    if( !g_app->m_resource->DoesTextureExist( fontFilename ) )
    {
        sprintf( fontFilename, "textures/speccy_font_normal.bmp" );
    }
    g_gameFont.Initialise( fontFilename );


    sprintf( fontFilename, "textures/editor_font_%s.bmp", _language );
    if( !g_app->m_resource->DoesTextureExist( fontFilename ) )
    {
        sprintf( fontFilename, "textures/editor_font_normal.bmp" );
    }
    g_editorFont.Initialise( fontFilename );

	//if ( g_inputManager )
	//	m_langTable->RebuildTables();
}


void App::SetProfileName( const char *_profileName )
{
    strcpy( m_userProfileName, _profileName );

	if( stricmp( _profileName, "AttractMode" ) != 0 )
	{
		g_prefsManager->SetString( "UserProfile", m_userProfileName );
		g_prefsManager->Save();
	}
}


#if defined(TARGET_OS_LINUX) || defined(TARGET_OS_MACOSX)
#include <sys/types.h>
#include <sys/stat.h>
#endif

const char *App::GetProfileDirectory()
{
#if defined(TARGET_OS_LINUX)

	static char userdir[256];
	const char *home = getenv("HOME");
	if (home != nullptr) {
		sprintf(userdir, "%s/.darwinia", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/.darwinia/%s/", home, DARWINIA_GAMETYPE);
		mkdir(userdir, 0777);
		return userdir;
	}
	else // Current directory if no home
		return "";

#elif defined(TARGET_OS_MACOSX)

	static char userdir[256];
	const char *home = getenv("HOME");
	if (home != nullptr) {
		sprintf(userdir, "%s/Library", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support/Darwinia", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support/Darwinia/%s/",
				home, DARWINIA_GAMETYPE);
		mkdir(userdir, 0777);

		return userdir;
	}
	else // Current directory if no home
		return "";

#else
	{
        return "";
    }
#endif
}

const char *App::GetPreferencesPath()
{
	// good leak #1
	static char *path = nullptr;

	if (path == nullptr) {
		const char *profileDir = GetProfileDirectory();
		path = new char[strlen(profileDir) + 32];
		sprintf( path, "%spreferences.txt", profileDir );
	}

	return path;
}

const char *App::GetScreenshotDirectory()
{
    return "";
}

bool App::LoadProfile()
{
    DebugOut( "Loading profile %s\n", m_userProfileName );

    if( (stricmp( m_userProfileName, "AccessAllAreas" ) == 0 ||
		stricmp( m_userProfileName, "AttractMode" ) == 0 ) &&
        g_app->m_gameMode != GameModePrologue )
    {
        // Cheat username that opens all locations
        // aimed at beta testers who've completed the game already

        if( m_globalWorld )
        {
            delete m_globalWorld;
            m_globalWorld = nullptr;
        }

        m_globalWorld = new GlobalWorld();
        m_globalWorld->LoadGame( "game_unlockall.txt" );
        for( int i = 0; i < m_globalWorld->m_buildings.Size(); ++i )
        {
            GlobalBuilding *building = m_globalWorld->m_buildings[i];
            if( building && building->m_type == Building::TypeTrunkPort )
            {
                building->m_online = true;
            }
        }
        for( int i = 0; i < m_globalWorld->m_locations.Size(); ++i )
        {
            GlobalLocation *loc = m_globalWorld->m_locations[i];
            loc->m_available = true;
        }
    }
    else
    {
        if( m_globalWorld )
        {
            delete m_globalWorld;
            m_globalWorld = nullptr;
        }

        m_globalWorld = new GlobalWorld();
        m_globalWorld->LoadGame( m_gameDataFile );
    }

    return true;
}

bool App::SaveProfile( bool _global, bool _local )
{
    if( stricmp( m_userProfileName, "none" ) == 0 ) return false;
    if( stricmp( m_userProfileName, "AccessAllAreas" ) == 0 ) return false;
	if( stricmp( m_userProfileName, "AttractMode" ) == 0 ) return false;

    DebugOut( "Saving profile %s\n", m_userProfileName );

    char folderName[512];
    sprintf( folderName, "%susers/", GetProfileDirectory() );
    bool success = CreateDirectory( folderName );
    if( !success )
    {
        DebugOut( "failed to create folder %s\n", folderName );
        return false;
    }

    sprintf( folderName, "%susers/%s", GetProfileDirectory(), m_userProfileName );
    success = CreateDirectory( folderName );
    if( !success )
    {
        DebugOut( "failed to create folder %s\n", folderName );
        return false;
    }

    if( _global )
    {
        m_globalWorld->SaveGame( m_gameDataFile );
        DebugOut( "Saved global data for profile %s\n", m_userProfileName );
    }

    if( _local && g_app->m_location )
    {
        if( m_levelReset )
        {
            m_levelReset = false;
            return false;
        }

        g_app->m_location->m_levelFile->GenerateInstantUnits();
        g_app->m_location->m_levelFile->GenerateDynamicBuildings();
        char *missionFilename = m_location->m_levelFile->m_missionFilename;
        m_location->m_levelFile->SaveMissionFile( missionFilename );
        DebugOut( "Saved level %s for profile %s\n", missionFilename, m_userProfileName );
    }
    return true;
}


void App::ResetLevel( bool _global )
{
    if( m_location )
    {
        m_requestedLocationId = -1;
	    m_requestedMission[0] = '\0';
	    m_requestedMap[0] = '\0';

        if( g_app->m_tutorial ) g_app->m_tutorial->Restart();


        //
        // Delete the saved mission file

        char *missionFilename = m_location->m_levelFile->m_missionFilename;
        char saveFilename[256];
        sprintf( saveFilename, "%susers/%s/%s", GetProfileDirectory(), m_userProfileName, missionFilename );

        DeleteThisFile( saveFilename );

        m_levelReset = true;


        //
        // Delete the game file if required

        if( _global )
        {
            sprintf( saveFilename, "%susers/%s/%s", GetProfileDirectory(), m_userProfileName, m_gameDataFile );

            DeleteThisFile( saveFilename );

            if( m_globalWorld )
            {
                delete m_globalWorld;
                m_globalWorld = nullptr;
            }

            m_globalWorld = new GlobalWorld();
            m_globalWorld->LoadGame( m_gameDataFile );
        }
    }
}

bool App::HasBoughtGame()
{
#if defined(DEMOBUILD)
	return false;
#else
	return true;
#endif
}

void App::LoadPrologue()
{
    m_gameMode = App::GameModePrologue;

    m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

    strcpy( m_gameDataFile, "game_demo2.txt" );
    LoadProfile();

    m_requestedLocationId = m_globalWorld->GetLocationId("launchpad");
    GlobalLocation *gloc = m_globalWorld->GetLocation(m_requestedLocationId);
    strcpy(m_requestedMap, gloc->m_mapFilename);
    strcpy(m_requestedMission, gloc->m_missionFilename);

	m_tutorial = new Demo2Tutorial();

    m_atMainMenu = false;

    g_prefsManager->SetInt( "RenderSpecialLighting", 1 );
	g_prefsManager->SetInt( "CurrentGameMode", 0 );
	g_prefsManager->Save();
}

void App::LoadCampaign()
{
    if( m_tutorial )
    {
        delete m_tutorial;
        m_tutorial = nullptr;
    }

    m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

    //m_atMainMenu = false;

    strcpy( m_gameDataFile, "game.txt" );
    LoadProfile();
    m_gameMode = App::GameModeCampaign;
    m_requestedLocationId = -1;
    g_prefsManager->SetInt( "RenderSpecialLighting", 0 );
    g_prefsManager->SetInt( "CurrentGameMode", 1 );
    g_prefsManager->Save();
}

