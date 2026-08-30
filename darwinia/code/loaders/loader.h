
#ifndef _included_loader_h
#define _included_loader_h


class Loader
{
public:
    enum
    {
        TypeSpeccyLoader,
        TypeMatrixLoader,
        TypeFodderLoader,
        TypeRayTraceLoader,
        TypeSoulLoader,
        TypeGameOfLifeLoader,
        TypeGameOfLifeLoaderGlow,
		TypeCreditsLoader,
        TypeAmigaLoader,
        NumLoaders
    };

protected:
    // Generic rendering code

    int  SetupFor2D     ( int _screenW );               // screenH is returned
    virtual void FlipBuffers    ();

    // Music / sound playing code

    void AdvanceSound   ();

    static int GetRandomLoaderIndex();

public:
    Loader();
    virtual ~Loader();

    virtual void Run();

    static Loader *CreateLoader         ( int _type );
	static const char *GetLoaderName          ( int _index );
	static int GetLoaderIndex           ( const char *_name );
};


extern "C"
{
    Loader* darw_CreateLoader( int type );
    void darw_DeleteLoader(Loader*);
    void darw_RunLoader(Loader*);
    
    const char* darw_GetLoaderName( int index );
    int darw_GetLoaderIndex( const char *name );
}


#endif
