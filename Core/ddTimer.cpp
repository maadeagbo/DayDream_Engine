#include "ddTimer.h"
#include "Pow2Assert.h"

#define TIME_HISTORY_MAX 30

namespace {
float dd_ftime[TIME_HISTORY_MAX];
float dd_time_scale;
float dd_time_in_secs;
float dd_avg_ftime;
bool dd_is_paused;
bool dd_init_history;
uint64_t dd_time_in_nano;
uint64_t dd_time_start;
}

#ifdef __linux__
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

inline uint64_t RDTSC() {
  unsigned int hi, lo;
  // ask the compiler to read the assembler code
  // assign the register holding the lower part of the 64-bit number to low
  // and vice-versa to high
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

// Calculate cycles per nanosecond
double SetCyclesPerNanoSec() {
  // pure c-struct need keyword
  struct timespec beginTS, endTS, diffTS;
  uint64_t _begin = 0, _end = 0;
  clock_gettime(CLOCK_MONOTONIC_RAW, &beginTS);
  _begin = RDTSC();
  uint64_t i;
  for (i = 0; i < 1000000; i++)
    ;
  _end = RDTSC();
  clock_gettime(CLOCK_MONOTONIC_RAW, &endTS);
  // calculate difference in time for "for loop" to run
  diffTS.tv_sec = endTS.tv_sec - beginTS.tv_sec;
  diffTS.tv_nsec = endTS.tv_nsec - beginTS.tv_nsec;
  const uint64_t elaspedNanoSecs =
      diffTS.tv_sec * 1000000000LL + diffTS.tv_nsec;

  return (double)(_end - _begin) / (double)elaspedNanoSecs;
}

// ddTimer::ddTimer()
//     : m_time_scale(1.0),
//       m_time_sec(0.0f),
//       m_avg_ft(0.f),
//       m_is_paused(false),
//       m_sethist(false),
//       m_time_nano(0) {
//   m_start_time = Timer::GetHiResTime();
// }

// ddTimer::~ddTimer() {}

namespace ddTime {
// return high resolution time in nanoseconds
uint64_t GetHiResTime() {
  uint64_t hrTime = 0;
  timespec now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  hrTime = (now.tv_sec * 1000000000L) + now.tv_nsec;

  return hrTime;
}

// Put CPU to sleep
void sleep(size_t milliseconds) { usleep(milliseconds); }
// Convert positive floating point seconds to uint64 nanoseconds
uint64_t SecsToNanoSecs(float seconds) {
  POW2_VERIFY_MSG(seconds >= 0.0f, "Cannot convert negative float", 0);
  float nanosecs = seconds * 1000000000.0f;
  return (uint64_t)nanosecs;
}
// Converts uint64 nanoseconds to float seconds (WARNING: can overflow)
float NanoSecsToSecs(uint64_t nanosecs) {
  uint64_t tempMicro = nanosecs / 1000LL;
  return (float)tempMicro / 1000000.0f;
}
// Converts uint64 nanoseconds to uint64_t milliseconds
uint64_t NanoSecsToMilli64(uint64_t nanosecs) {
  uint64_t tempMicro = nanosecs / 1000LL;
  return tempMicro / 1000LL;
}
}

#else  // windows

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <TimeAPI.h>
#include <windows.h>


namespace ddTime {

// return cpu cycles
uint64_t GetHiResTime() {
  LARGE_INTEGER now, freq;
  QueryPerformanceCounter(&now);
  QueryPerformanceFrequency(&freq);

  return ((uint64_t)now.QuadPart * 1000000000LL) / (uint64_t)freq.QuadPart;
}

// Put CPU to sleep
void sleep(float seconds) {
  // Sleep((DWORD)milliseconds);
  std::this_thread::sleep_for(
      std::chrono::duration<float, std::ratio<1, 1000> >(seconds));
}

// Convert positive floating point seconds to uint64 nanoseconds
uint64_t SecsToNanoSecs(float seconds) {
  POW2_VERIFY_MSG(seconds >= 0.0f, "Cannot convert negative float", 0);
  float nanosecs = seconds * 1000000000.0f;
  return (uint64_t)nanosecs;
}
// Converts uint64 nanoseconds to float seconds (WARNING: can overflow)
float NanoSecsToSecs(uint64_t nanosecs) {
  uint64_t tempMicro = nanosecs / 1000LL;
  return (float)tempMicro / 1000000.0f;
}
// Converts uint64 nanoseconds to uint64_t milliseconds
uint64_t NanoSecsToMilli64(uint64_t nanosecs) {
  uint64_t tempMicro = nanosecs / 1000LL;
  return tempMicro / 1000LL;
}
}

// ddTimer::ddTimer()
//     : m_time_scale(1.0),
//       m_time_sec(0.0f),
//       m_avg_ft(0.f),
//       m_is_paused(false),
//       m_sethist(false),
//       m_time_nano(0) {
//   m_start_time = Timer::GetHiResTime();
//   timeBeginPeriod(1);
// }

// ddTimer::~ddTimer() { timeEndPeriod(1); }

#endif  // if on windows platform

void ddTime::initialize() {
  dd_time_start = GetHiResTime();
  dd_time_scale = 1.f;
  dd_time_in_nano = 0;
  dd_time_in_secs = 0.f;
  dd_avg_ftime = 0.f;
  dd_is_paused = false;
  dd_init_history = true;

  // for windows maybe:
  // timeBeginPeriod(1); // init
  // timeEndPeriod(1);   // exit
}

void ddTime::update() {
  auto update_hist = [&](const float frame_time) {
    for (size_t i = TIME_HISTORY_MAX - 1; i > 0; i--) {
      dd_ftime[i] = dd_ftime[i - 1];
    }
    dd_ftime[0] = frame_time;
    // update avg
    dd_avg_ftime = 0;
    for (size_t i = 0; i < TIME_HISTORY_MAX; i++) {
      dd_avg_ftime += dd_ftime[i];
    }
    dd_avg_ftime /= TIME_HISTORY_MAX;
  };

  uint64_t temp = dd_time_in_nano;
  if (!dd_is_paused) {
    // calculate current time in seconds and nanoseconds
    dd_time_in_nano = GetHiResTime() - dd_time_start;
    dd_time_in_secs = NanoSecsToSecs(dd_time_in_nano);

    // update framerate history
    float _t = NanoSecsToSecs(dd_time_in_nano - temp);
    if (dd_init_history) {
      dd_init_history = false;
      for (size_t i = 0; i < TIME_HISTORY_MAX; i++) {
        dd_ftime[i] = _t;
      }
    }
    update_hist(_t);
  }
}

void ddTime::singleStep() {
  auto update_hist = [&](const float frame_time) {
    for (size_t i = TIME_HISTORY_MAX - 1; i > 0; i--) {
      dd_ftime[i] = dd_ftime[i - 1];
    }
    dd_ftime[0] = frame_time;
    // update avg
    dd_avg_ftime = 0;
    for (size_t i = 0; i < TIME_HISTORY_MAX; i++) {
      dd_avg_ftime += dd_ftime[i];
    }
    dd_avg_ftime /= TIME_HISTORY_MAX;
  };
  // Used for debugging
  // Single step through time
  // add one "ideal" frame interval and scale by time scale
  if (dd_is_paused) {
    dd_time_in_nano += SecsToNanoSecs((1.0f / 60.0f) * dd_time_scale);
    dd_time_in_secs = NanoSecsToSecs(dd_time_in_nano);
    // update framerate history
    update_hist((1.0f / 60.0f) * dd_time_scale);
  }
}

void ddTime::pause() { dd_is_paused = true; }

void ddTime::unpause() { dd_is_paused = false; }

void ddTime::set_scale(const float _scale) { dd_time_scale = _scale; }

uint64_t ddTime::get_time() { return dd_time_in_nano; }

float ddTime::get_time_float() { return dd_time_in_secs; }

float ddTime::get_frame_time() { return dd_ftime[0]; }

float ddTime::get_avg_frame_fime() { return dd_avg_ftime; }

// // called once per frame with real measured frame time in delta seconds
// void ddTimer::update(const float fixedrate) {
//   uint64_t temp = m_time_nano;
//   if (!m_is_paused) {
//     m_time_nano = Timer::GetHiResTime() - m_start_time;
//     m_time_sec = Timer::NanoSecsToSecs(m_time_nano);

//     if (fixedrate < 0.f) {
//       float _t = Timer::NanoSecsToSecs(m_time_nano - temp);
//       if (!m_sethist) {
//         m_sethist = true;
//         initHistory(_t);
//       }
//       updateHistory(_t);
//     } else {
//       if (!m_sethist) {
//         m_sethist = true;
//         initHistory(fixedrate);
//       }
//       updateHistory(fixedrate);
//     }
//   }
// }

// // Used for debugging
// // Single step through time
// void ddTimer::singleStep() {
//   // add one "ideal" frame interval and scale by time scale
//   if (m_is_paused) {
//     m_time_nano += Timer::SecsToNanoSecs((1.0f / 30.0f) * m_time_scale);
//   }
// }

// void ddTimer::initHistory(const float value) {
//   for (size_t i = 0; i < Timer::f_hist; i++) {
//     m_ft[i] = value;
//   }
// }

// void ddTimer::updateHistory(const float frame_time) {
//   for (size_t i = Timer::f_hist - 1; i > 0; i--) {
//     m_ft[i] = m_ft[i - 1];
//   }
//   m_ft[0] = frame_time;
//   // update avg
//   m_avg_ft = 0;
//   for (size_t i = 0; i < Timer::f_hist; i++) {
//     m_avg_ft += m_ft[i];
//   }
//   m_avg_ft /= Timer::f_hist;
// }
