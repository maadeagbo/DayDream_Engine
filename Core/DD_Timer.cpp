#include "DD_Timer.h"
#include "Pow2Assert.h"

#ifdef __linux__
#include <x86intrin.h>
#include <unistd.h>
#include <time.h>

inline u64 RDTSC()
{
	unsigned int hi, lo;
	// ask the compiler to read the assembler code
	// assign the register holding the lower part of the 64-bit number to low
	// and vice-versa to high
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((u64)hi << 32) | lo;
}

// Calculate cycles per nanosecond
double SetCyclesPerNanoSec()
{
	// pure c-struct need keyword
	struct timespec beginTS, endTS, diffTS;
	u64 _begin = 0, _end = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &beginTS);
	_begin = RDTSC();
	u64 i;
	for( i = 0; i < 1000000; i++ );
	_end = RDTSC();
	clock_gettime(CLOCK_MONOTONIC_RAW, &endTS);
	// calculate difference in time for "for loop" to run
	diffTS.tv_sec = endTS.tv_sec - beginTS.tv_sec;
	diffTS.tv_nsec = endTS.tv_nsec - beginTS.tv_nsec;
	const u64 elaspedNanoSecs = diffTS.tv_sec * 1000000000LL + diffTS.tv_nsec;

	return (double)(_end - _begin) / (double)elaspedNanoSecs;
}

namespace Timer {
	// return high resolution time in nanoseconds
	u64 GetHiResTime()
	{
		u64 hrTime = 0;
		timespec now;
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		hrTime = (now.tv_sec * 1000000000L) + now.tv_nsec;

		return hrTime;
	}

	// Put CPU to sleep
	void sleep(size_t milliseconds)
	{
		usleep(milliseconds);
	}
	// Convert positive floating point seconds to uint64 nanoseconds
	u64	SecsToNanoSecs(float seconds)
	{
		POW2_VERIFY_MSG(seconds >= 0.0f, "Cannot convert negative float", 0);
		float nanosecs = seconds * 1000000000.0f;
		return (u64)nanosecs;
	}
	// Converts uint64 nanoseconds to float seconds (WARNING: can overflow)
	float NanoSecsToSecs(u64 nanosecs)
	{
		u64 tempMicro = nanosecs / 1000LL;
		return (float)tempMicro / 1000000.0f;
	}
	// Converts uint64 nanoseconds to u64 milliseconds
	u64 NanoSecsToMilli64(u64 nanosecs)
	{
		u64 tempMicro = nanosecs / 1000LL;
		return tempMicro / 1000LL;
	}
}

DD_Timer::DD_Timer() :
	m_time_scale(1.0),
	m_time_sec(0.0f),
	m_avg_ft(0.f),
	m_is_paused(false),
	m_sethist(false),
	m_time_nano(0)
{
	m_start_time = Timer::GetHiResTime();
}

DD_Timer::~DD_Timer() {}

#else // windows

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>
#include <TimeAPI.h>

namespace {
	u64 TIMER_START;
}

namespace Timer {
	void LoadStart()
	{
		TIMER_START = GetHiResTime();
	}

	float GetDeltaTime()
	{
		u64 time_n = GetHiResTime() - TIMER_START;
		return NanoSecsToSecs(time_n);
	}

	// return cpu cycles
	u64 GetHiResTime()
	{
		LARGE_INTEGER now, freq;
		QueryPerformanceCounter(&now);
		QueryPerformanceFrequency(&freq);

		return ((u64)now.QuadPart * 1000000000LL) / (u64)freq.QuadPart;
	}

	// Put CPU to sleep
	void sleep(float seconds)
	{
		//Sleep((DWORD)milliseconds);
		std::this_thread::sleep_for(
			std::chrono::duration<float, std::ratio<1, 1000>>(seconds)
		);
	}

	// Convert positive floating point seconds to uint64 nanoseconds
	u64	SecsToNanoSecs(float seconds)
	{
		POW2_VERIFY_MSG(seconds >= 0.0f, "Cannot convert negative float", 0);
		float nanosecs = seconds * 1000000000.0f;
		return (u64)nanosecs;
	}
	// Converts uint64 nanoseconds to float seconds (WARNING: can overflow)
	float NanoSecsToSecs(u64 nanosecs)
	{
		u64 tempMicro = nanosecs / 1000LL;
		return (float)tempMicro / 1000000.0f;
	}
	// Converts uint64 nanoseconds to u64 milliseconds
	u64 NanoSecsToMilli64(u64 nanosecs)
	{
		u64 tempMicro = nanosecs / 1000LL;
		return tempMicro / 1000LL;
	}
}

DD_Timer::DD_Timer() :
	m_time_scale(1.0),
	m_time_sec(0.0f),
	m_avg_ft(0.f),
	m_is_paused(false),
	m_sethist(false),
	m_time_nano(0)
{
	m_start_time = Timer::GetHiResTime();
	timeBeginPeriod(1);
}

DD_Timer::~DD_Timer() { timeEndPeriod(1); }

#endif // if on windows platform

// called once per frame with real measured frame time in delta seconds
void DD_Timer::update(const float fixedrate)
{
	u64 temp = m_time_nano;
	if( !m_is_paused ) {
		m_time_nano = Timer::GetHiResTime() - m_start_time;
		m_time_sec = Timer::NanoSecsToSecs(m_time_nano);

		if( fixedrate < 0.f ) {
			float _t = Timer::NanoSecsToSecs(m_time_nano - temp);
			if( !m_sethist ) { m_sethist = true; initHistory(_t); }
			updateHistory(_t);
		}
		else {
			if( !m_sethist ) { m_sethist = true; initHistory(fixedrate); }
			updateHistory(fixedrate);
		}
	}
}

// Used for debugging
// Single step through time
void DD_Timer::singleStep()
{
	// add one "ideal" frame interval and scale by time scale
	if( m_is_paused ) {
		m_time_nano += Timer::SecsToNanoSecs((1.0f / 30.0f) * m_time_scale);
	}
}

void DD_Timer::initHistory(const float value)
{
	for( size_t i = 0; i < Timer::f_hist; i++ ) { m_ft[i] = value; }
}

void DD_Timer::updateHistory(const float frame_time)
{
	for( size_t i = Timer::f_hist - 1; i > 0; i-- ) { m_ft[i] = m_ft[i - 1]; }
	m_ft[0] = frame_time;
	// update avg
	m_avg_ft = 0;
	for( size_t i = 0; i < Timer::f_hist; i++ ) { m_avg_ft += m_ft[i]; }
	m_avg_ft /= Timer::f_hist;
}
