#ifndef INCLUDED_SOUND_LIBRARY_2D_SDL
#define INCLUDED_SOUND_LIBRARY_2D_SDL

#include "sound_library_2d.h"
#include "Fake_ASWSDLRingBuffer.h"

//*****************************************************************************
// Class SoundLibrary2dSDL
//*****************************************************************************

class SoundLibrary2dSDL : public SoundLibrary2d
{
private:
	class RingBuffer {
	public:
		RingBuffer();
		~RingBuffer();
		void Reset();
		void Resize(int _len);
		int Read(StereoSample *_buffer, int _len);
		int Write(StereoSample *_buffer, int _len);
		int SpaceAvailable();

	private:

		StereoSample *m_stream;
		StereoSample *m_readPos, *m_writePos, *m_endPos;
	};

#ifndef INVOKE_CALLBACK_FROM_SOUND_THREAD
	StereoSample	 m_callbackBuffer[44100];
	ASWSDLRingBuffer *m_ringBuffer;
	double			 m_lastSampleTime;
#endif
	void			(*m_callback) (StereoSample *buf, unsigned int numSamples);
	FILE            *m_wavOutput;

public:

	//NetMutex		m_callbackLock;

	void			AudioCallback(StereoSample *buf, unsigned int numBytes);

	virtual void Stop()override;

public:
	SoundLibrary2dSDL();
	~SoundLibrary2dSDL();

	virtual void SetCallback(void (*_callback) (StereoSample *, unsigned int)) override;
	virtual void TopupBuffer() override;

	void StartRecordToFile(char const *_filename)override;
	void EndRecordToFile()override;
};


extern SoundLibrary2d *g_soundLibrary2d;


#endif
