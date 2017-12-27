#include "Timer.h"
#include "Pow2Assert.h"

#define HISTORY_MAX 30

namespace {
float ftime[HISTORY_MAX];
float time_scale;
float time_in_secs;
float avg_ftime;
bool is_paused;
bool init_history;
uint64_t time_in_nano;
uint64_t time_start;
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

namespace {
uint64_t TIMER_START;
}

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
  time_start = GetHiResTime();
  time_scale = 1.f;
  time_in_nano = 0;
  time_in_secs = 0.f;
  avg_ftime = 0.f;
  is_paused = false;
  init_history = true;

  // for windows maybe:
  // timeBeginPeriod(1); // init
  // timeEndPeriod(1);   // exit
}

void ddTime::update() {
  auto update_hist = [&](const float frame_time) {
    for (size_t i = HISTORY_MAX - 1; i > 0; i--) {
      ftime[i] = ftime[i - 1];
    }
    ftime[0] = frame_time;
    // update avg
    avg_ftime = 0;
    for (size_t i = 0; i < HISTORY_MAX; i++) {
      avg_ftime += ftime[i];
    }
    avg_ftime /= HISTORY_MAX;
  };

  uint64_t temp = time_in_nano;
  if (!is_paused) {
    // calculate current time in seconds and nanoseconds
    time_in_nano = GetHiResTime() - time_start;
    time_in_secs = NanoSecsToSecs(time_in_nano);

    // update framerate history
    float _t = NanoSecsToSecs(time_in_nano - temp);
    if (init_history) {
      init_history = false;
      for (size_t i = 0; i < HISTORY_MAX; i++) {
        ftime[i] = _t;
      }
    }
    update_hist(_t);
  }
}

void ddTime::singleStep() {
  auto update_hist = [&](const float frame_time) {
    for (size_t i = HISTORY_MAX - 1; i > 0; i--) {
      ftime[i] = ftime[i - 1];
    }
    ftime[0] = frame_time;
    // update avg
    avg_ftime = 0;
    for (size_t i = 0; i < HISTORY_MAX; i++) {
      avg_ftime += ftime[i];
    }
    avg_ftime /= HISTORY_MAX;
  };
  // Used for debugging
  // Single step through time
  // add one "ideal" frame interval and scale by time scale
  if (is_paused) {
    time_in_nano += SecsToNanoSecs((1.0f / 60.0f) * time_scale);
    time_in_secs = NanoSecsToSecs(time_in_nano);
    // update framerate history
    update_hist((1.0f / 60.0f) * time_scale);
  }
}

void ddTime::pause() { is_paused = true; }

void ddTime::unpause() { is_paused = false; }

void ddTime::set_scale(const float _scale) { time_scale = _scale; }

uint64_t ddTime::get_time() { return time_in_nano; }

float ddTime::get_time_float() { return time_in_secs; }

float ddTime::get_frame_time() { return ftime[0]; }

float ddTime::get_avg_frame_fime() { return avg_ftime; }

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
