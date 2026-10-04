#ifndef _INCLUDED_APP_H
#define _INCLUDED_APP_H

#include "lib/rgb_colour.h"

class Camera;
class Location;
class Server;
class ClientToServer;
class Renderer;
class UserInput;
class Resource;
class SoundSystem;
class LocationInput;
class LangTable;
class EffectProcessor;
class GlobalWorld;
class HelpSystem;
class Tutorial;
class ParticleSystem;
class TaskManager;
class TaskManagerInterface;
class Gesture;
class Script;
class Sepulveda;
class TestHarness;
class Profiler;
class AVIGenerator;
class LocationEditor;
class MouseCursor;
class GameCursor;
class GameMenu;
class StartSequence;
class DemoEndSequence;
class AttractMode;
class ControlHelpSystem;
class BitmapRGBA;
class GameMenu;



class App
{
public:
	// Library Code Objects
    UserInput           *m_userInput;
    Resource            *m_resource;
    SoundSystem         *m_soundSystem;
	ParticleSystem		*m_particleSystem;
	LangTable			*m_langTable;
    AVIGenerator        *m_aviGenerator;
    Profiler            *m_profiler;

	// Things that are the world
    GlobalWorld         *m_globalWorld;
    Location            *m_location;
	int					m_locationId;

	// Everything else
    Camera              *m_camera;
	Server              *m_server;                  // Server process, can be nullptr if client
    ClientToServer      *m_clientToServer;          // Clients connection to Server
    Renderer            *m_renderer;
	LocationInput		*m_locationInput;
	LocationEditor		*m_locationEditor;
    HelpSystem          *m_helpSystem;
    Tutorial            *m_tutorial;
	EffectProcessor		*m_effectProcessor;
    TaskManager         *m_taskManager;
    TaskManagerInterface *m_taskManagerInterface;
    Gesture             *m_gesture;
    Script              *m_script;
    Sepulveda           *m_sepulveda;
    TestHarness         *m_testHarness;
    GameCursor          *m_gameCursor;
    StartSequence       *m_startSequence;
    DemoEndSequence     *m_demoEndSequence;
	AttractMode			*m_attractMode;
    ControlHelpSystem   *m_controlHelpSystem;
    GameMenu            *m_gameMenu;

    bool                m_bypassNetworking;
    bool                m_negativeRenderer;
	int					m_difficultyLevel;			// Cached from preferences
	bool				m_largeMenus;

	// State flags
    bool                m_paused;
	bool                m_editing;

	// Requested state flags
	int					m_requestedLocationId;		// -1 for global world
	bool                m_requestToggleEditing;
	bool				m_requestQuit;

    char                m_userProfileName[256];
    char                m_requestedMission[256];
    char                m_requestedMap[256];
    bool                m_levelReset;
    char                m_gameDataFile[256];

	RGBAColour			m_backgroundColour;

    bool                m_atMainMenu;       // true when the player is viewing the darwinia/mutliwinia menu
    int                 m_gameMode;

    enum
    {
        GameModeNone,
        GameModePrologue,
        GameModeCampaign,
        GameModeMultiwinia,
        NumGameModes
    };

public:
    App ();
	~App();


	void    SetProfileName  ( const char *_profileName );
    bool    LoadProfile     ();
    bool    SaveProfile     ( bool _global, bool _local );
    void    ResetLevel      ( bool _global );

	void    SetLanguage     ( const char *_language, bool _test );

	bool	HasBoughtGame	();

    void    LoadPrologue    ();
    void    LoadCampaign    ();

	static const char *GetProfileDirectory();
	static const char *GetPreferencesPath();
    static const char *GetScreenshotDirectory();

	void	UpdateDifficultyFromPreferences();
};

extern App *g_app;

#endif
