﻿#ifndef _included_soundsystem_h
#define _included_soundsystem_h

#include "lib/fast_darray.h"
#include "lib/llist.h"
#include "lib/text_stream_readers.h"
#include "lib/vector3.h"

#include "sound/sound_instance.h"
#include "sound/sound_parameter.h"

#include "worldobject/worldobject.h"


class Entity;
class Building;
class TextReader;
class SoundInstance;
class Profiler;
class FileWriter;


//*****************************************************************************
// Class SoundEventBlueprint
//*****************************************************************************

class SoundEventBlueprint
{
public:
    char            *m_eventName;
    SoundInstance   *m_instance;

public:
    SoundEventBlueprint();

    void    SetEventName( char *_name );
};


//*****************************************************************************
// Class SoundSourceBlueprint
//*****************************************************************************

class SoundSourceBlueprint
{
public:
    enum                    // Other types of Sound Source
    {
        TypeLaser,
        TypeGrenade,
        TypeRocket,
        TypeAirstrikeBomb,
        TypeSpirit,
        TypeSepulveda,
        TypeGesture,
        TypeAmbience,
        TypeMusic,
        TypeInterface,
        NumOtherSoundSources
    };

public:
    LList       <SoundEventBlueprint *> m_events;

    static int   GetSoundSoundType ( char const *_name );
	static const char *GetSoundSourceName( int _type );
	static void  ListSoundEvents   ( int _type, LList<const char *> *_list );
};


//*****************************************************************************
// Class DspParameterBlueprint
//*****************************************************************************

class DspParameterBlueprint
{
public:
    char    m_name[256];
    float   m_min;
    float   m_max;
    float   m_default;
    int     m_dataType;                     // 0 = float, 1 = long
};


//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

class DspBlueprint
{
public:
    char    m_name[256];
    DArray  <DspParameterBlueprint *> m_params;

public:
    char *GetParameter   ( int _param, float *_min=NULL, float *_max=NULL, float *_default=NULL, int *_dataType=NULL );
};


//*****************************************************************************
// Class SampleGroup
//*****************************************************************************

class SampleGroup
{
public:
    char    m_name[256];
    LList   <char *> m_samples;

public:
	~SampleGroup            ();
	void    SetName         ( const char *_name );
	void    AddSample       ( const char *_sample );
};


//*****************************************************************************
// Class SoundSystem
//*****************************************************************************

class SoundSystem
{
public:
	enum
	{
		SoundSourceNoError,											// Remember to update g_soundSourceErrors in soundsystem.cpp
		SoundSourceNotMono,
		SoundSourceFileDoesNotExist,
		SoundSourceFilenameHasSpace,
		SoundSourceNumErrors
	};

    float           m_timeSync;
    bool            m_propagateBlueprints;                          // If true, looping sounds will sync with blueprints
    Profiler        *m_mainProfiler;
	Profiler		*m_eventProfiler;
	bool			m_quitWithoutSave;

    Vector3         m_editorPos;
    SoundInstanceId m_editorInstanceId;

    FastDArray      <SoundInstance *> m_sounds;						// All the sounds that want to play
	SoundInstance	*m_music;										// There can only be one piece of music at a time
	SoundInstance	*m_requestedMusic;

    SoundInstanceId *m_channels;
    int             m_numChannels;

    DArray      <SoundSourceBlueprint *> m_entityBlueprints;        // Indexed on Entity::m_type
    DArray      <SoundSourceBlueprint *> m_buildingBlueprints;      // Indexed on Building::m_type
    DArray      <SoundSourceBlueprint *> m_otherBlueprints;         // Indexed on SoundSourceBlueprint
    DArray      <DspBlueprint *> m_filterBlueprints;        // Indexed on SoundLibrary3d::FX types

    DArray      <SampleGroup *> m_sampleGroups;

protected:
    void ParseSoundEvent		( TextReader *_in, SoundSourceBlueprint *_source, char const *_entityName );
    void ParseSoundEffect		( TextReader *_in, SoundEventBlueprint *_blueprint );
    void ParseSampleGroup       ( TextReader *_in, SampleGroup *_group );

    void WriteSoundEvent		( FileWriter *_file, SoundEventBlueprint *_event );
    void WriteSampleGroup       ( FileWriter *_file, SampleGroup *_group );

    void SaveBlueprints			( char const *_filename );

    int FindBestAvailableChannel();

    static bool SoundLibraryMainCallback( unsigned int _channel,
                                          signed short *_data,
                                          unsigned int _numSamples,
                                          int *_silenceRemaining );
    static bool SoundLibraryMusicCallback(signed short *_data,
                                          unsigned int _numSamples,
                                          int *_silenceRemaining );

public:
    SoundSystem();
    ~SoundSystem();

	void Initialise				();
    void RestartSoundLibrary    ();

    void LoadEffects			();
    void LoadBlueprints			();
    void SaveBlueprints			();
	bool AreBlueprintsModified	();

    void Advance();

    bool InitialiseSound        ( SoundInstance *_instance );                   // Sets up sound, adds to instance list
    void ShutdownSound          ( SoundInstance *_instance );                   // Stops / deletes sound + removes refs

    int  IsSoundPlaying         ( SoundInstanceId _id );
	int  NumInstancesPlaying    ( WorldObjectId _id, const char *_eventName );
	int  NumInstances           ( WorldObjectId _id, const char *_eventName );

    int  NumSoundInstances      ();
    int  NumChannelsUsed        ();
    int  NumSoundsDiscarded     ();

	void TriggerEntityEvent     ( Entity *_entity,      const char *_eventName );
	void TriggerBuildingEvent   ( Building *_building,  const char *_eventName );
	void TriggerOtherEvent      ( WorldObject *_other,  const char *_eventName, int _type );

	void StopAllSounds          ( WorldObjectId _id, const char *_eventName=NULL );        // Pass in NULL to stop every event.
                                                                                     // Full event name required, eg "Darwinian SeenThreat"

    void StopAllDSPEffects      ();

    void TriggerDuplicateSound  ( SoundInstance *_instance );

    void PropagateBlueprints    ();                                             // Call this to update all looping sounds
    void RuntimeVerify          ();												// Verifies that the sound system has screwed it's own datastructures
	void LoadtimeVerify			();												// Verifies that the data load from sounds.txt is OK
	char const *IsSoundSourceOK	(char const *_soundName);						// Tests that file names and file formats are OK, returns an error code from the SoundSource enum
	bool IsSampleUsed           (char const *_soundName);                       // Looks to see if that sound name is used in any blueprints

	SampleGroup *GetSampleGroup     ( const char *_name );
	SampleGroup *NewSampleGroup     ( const char *_name=NULL );
	bool        RenameSampleGroup   ( const char *_oldName, const char *_newName );

    SoundInstance *GetSoundInstance( SoundInstanceId id );
};


#endif
