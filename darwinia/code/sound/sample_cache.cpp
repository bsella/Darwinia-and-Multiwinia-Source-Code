#include "lib/resource.h"
#include "sound/sample_cache.h"
#include "sound/sound_stream_decoder.h"
#include "app.h"


CachedSampleManager g_cachedSampleManager;
bool g_deletingCachedSampleHandle = false;


//*****************************************************************************
// Class CachedSampleHandle
//*****************************************************************************

CachedSampleHandle::CachedSampleHandle(CachedSample *_sample)
:	m_nextSampleIndex(0),
	m_cachedSample(_sample)
{
}


CachedSampleHandle::~CachedSampleHandle()
{
	m_cachedSample = nullptr;
	m_nextSampleIndex = 0xffffffff;
}


unsigned int CachedSampleHandle::Read(signed short *_data, unsigned int _numSamples)
{
	unsigned int samplesRemaining = darw_CachedSampleNumSamples(m_cachedSample) - m_nextSampleIndex;
	if (_numSamples > samplesRemaining)
	{
		_numSamples = samplesRemaining;
	}

	darw_CachedSampleRead(m_cachedSample, _data, m_nextSampleIndex, _numSamples);
	m_nextSampleIndex += _numSamples;

	return _numSamples;
}


void CachedSampleHandle::Restart()
{
	m_nextSampleIndex = 0;
}



//*****************************************************************************
// Class CachedSampleManager
//*****************************************************************************

CachedSampleManager::~CachedSampleManager()
{
	for (unsigned int i = 0; i < m_cache.Size(); ++i)
	{
		darw_DeleteCachedSample(m_cache.GetData(i));
	}
}


CachedSampleHandle *CachedSampleManager::GetSample(char const *_sampleName)
{
	CachedSample *cachedSample = m_cache.GetData(_sampleName);

	if (!cachedSample)
	{
		cachedSample = darw_CreateCachedSample(_sampleName);
		m_cache.PutData(_sampleName, cachedSample);
    }

	CachedSampleHandle *rv = new CachedSampleHandle(cachedSample);
	return rv;
}


void CachedSampleManager::EmptyCache()
{
    m_cache.EmptyAndDelete();
}


int CachedSampleManager::GetMemoryUsage()
{
    int memoryUsage = 0;

    for( unsigned int i = 0; i < m_cache.Size(); ++i )
    {
        if( m_cache.ValidIndex(i) )
        {
            CachedSample *sample = m_cache.GetData(i);
            int sampleSize = sizeof(signed short) * darw_CachedSampleNumChannels(sample) * darw_CachedSampleNumSamples(sample);
            memoryUsage += sampleSize;
        }
    }

    return memoryUsage;
}
