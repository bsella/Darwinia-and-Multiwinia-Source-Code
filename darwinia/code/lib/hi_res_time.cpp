#include "lib/universal_include.h"

#ifdef TARGET_MSVC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <math.h>
#include "main.h"

#include "hi_res_time.h"
#include <chrono>


#define USE_PENTIUM_COUNTER 0


static unsigned int g_ticksPerSec = 0;
static double g_tickInterval = 1.0;
static double g_lastGetHighResTime = 0.0;
static double g_timeShift = 0.0;
static bool g_usingFakeTimeMode = false;
static double g_fakeTime;


#pragma warning (push)
#pragma warning (disable : 4035)	// disable no return value warning
inline  unsigned int GetPentiumCounter()
{
#ifdef TARGET_OS_LINUX
	asm("_emit 0x0F		        // Call RDTSC (Read time stamp counter) this will"
		"_emit 0x31				// put a 64 bit clock cycle count in edx;eax"

		"shr eax,6				// 64 bits are too much and the resolution is too"
		"						// high, so throw away the bottom 6 bits of eax"
		"shl edx,26				// Now throw away everything other than the top 6 bits of edx"
		"or eax,edx				// Shove the edx data into the gap in eax"
		"						// Return value in eax, which apparently VC is happy with"
	);
#else
	__asm
	{
		_emit 0x0F		        // Call RDTSC (Read time stamp counter) this will
		_emit 0x31				// put a 64 bit clock cycle count in edx;eax

		shr eax,6				// 64 bits are too much and the resolution is too
								// high, so throw away the bottom 6 bits of eax
		shl edx,26				// Now throw away everything other than the top 6 bits of edx
		or eax,edx				// Shove the edx data into the gap in eax
								// Return value in eax, which apparently VC is happy with
	}
#endif
}
#pragma warning (pop)



// *** InitialiseHighResTime
void InitialiseHighResTime()
{
#ifdef TARGET_MSVC
	// Start be getting the frequency the Performance Counter uses.
	// We need to use the Performance Counter to calibrate the Pentium
	// Counter, if we are going to use it.
    LARGE_INTEGER count;
    QueryPerformanceFrequency(&count);
    g_tickInterval = 1.0 / (double)count.QuadPart;
#else
	///TODO
#endif

#if USE_PENTIUM_COUNTER
	// Get start times
	QueryPerformanceCounter(&count);
	unsigned int pentiumCounterStart = GetPentiumCounter();
	double perfCounterStart = count.QuadPart * g_tickInterval;

	// Wait for roughly one second
	Sleep(1000);

	// Get end times
	QueryPerformanceCounter(&count);
	unsigned int pentiumCounterEnd = GetPentiumCounter();
	double perfCounterEnd = count.QuadPart * g_tickInterval;

	double perfCounterTicks = perfCounterEnd - perfCounterStart;
	double perfCounterTime = perfCounterTicks;
	double pentiumTicks = pentiumCounterEnd - pentiumCounterStart;

	g_ticksPerSec = pentiumTicks / perfCounterTime;
	g_tickInterval = 1.0 / (double)g_ticksPerSec;
#endif
}


inline double GetLowLevelTime()
{
	///TODO

	using namespace std::chrono;
	static auto startTime = high_resolution_clock::now();

	high_resolution_clock::time_point t = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t- startTime);
	return time_span.count();

}


// *** GetHighResTime
double GetHighResTime()
{
    if (g_usingFakeTimeMode)
    {
	    g_lastGetHighResTime = g_fakeTime;
        return g_fakeTime;
    }

	double timeNow = GetLowLevelTime();
    timeNow -= g_timeShift;

//    if( timeNow > g_lastGetHighResTime + 1.0f )
//    {
//        g_timeShift += timeNow - g_lastGetHighResTime;
//        timeNow -= timeNow - g_lastGetHighResTime;
//    }

    g_lastGetHighResTime = timeNow;
    return timeNow;
}


void SetFakeTimeMode()
{
	g_fakeTime = GetHighResTime();
    g_usingFakeTimeMode = true;
}


void SetRealTimeMode()
{
    g_usingFakeTimeMode = false;
	double realTime = GetLowLevelTime();
	g_timeShift = realTime - g_fakeTime;
}


void IncrementFakeTime(double _increment)
{
    g_fakeTime += _increment;
}
