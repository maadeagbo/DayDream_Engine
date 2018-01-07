#pragma once

/*
 * Copyright (c) 2016, Moses Adeagbo
 * All rights reserved.
 */

/*-----------------------------------------------------------------------------
*
*	ddTimer:
*		- Reports high resolution clock time (in nanoseconds)
*
-----------------------------------------------------------------------------*/

#include "ddIncludes.h"

namespace Timer {
// void LoadStart();
// float GetDeltaTime();

// uint64_t GetHiResTime();
// void sleep(float seconds);
// uint64_t SecsToNanoSecs(float seconds);
// float NanoSecsToSecs(uint64_t nanosecs);
// uint64_t NanoSecsToMilli64(uint64_t nanosecs);
// const size_t f_hist = 30;
}  // namespace Timer

// class ddTimer {
//  public:
//   ddTimer();
//   ~ddTimer();

//   void update(const float fixedrate = -1.f);
//   void singleStep();
//   inline void pause() { m_is_paused = true; }
//   inline void unpause() { m_is_paused = false; }
//   inline void setScale(const float _scale) { m_time_scale = _scale; }
//   inline uint64_t getTime() const { return m_time_nano; }
//   inline float getTimeFloat() const { return m_time_sec; }
//   inline float getFrameTime() const { return m_ft[0]; }
//   inline float getAvgFrameTime() const { return m_avg_ft; }

//  private:
//   void initHistory(const float value);
//   void updateHistory(const float frame_time);

//   float m_time_scale, m_time_sec, m_avg_ft;
//   bool m_is_paused, m_sethist;
//   uint64_t m_time_nano, m_start_time;
//   float m_ft[Timer::f_hist];
// };

namespace ddTime {

uint64_t GetHiResTime(const bool start_end = true);
void sleep(float seconds);
uint64_t SecsToNanoSecs(float seconds);
float NanoSecsToSecs(uint64_t nanosecs);
uint64_t NanoSecsToMilli64(uint64_t nanosecs);

/// \brief DO NOT CALL. ONLY TO BE CALLED BY ddEngine
void initialize();
/// \brief DO NOT CALL. ONLY TO BE CALLED BY ddEngine
void update();
/// \brief DO NOT CALL. ONLY TO BE CALLED BY ddEngine
void pause();
/// \brief DO NOT CALL. ONLY TO BE CALLED BY ddEngine
void unpause();
/// \brief DO NOT CALL. ONLY TO BE CALLED BY ddEngine
void set_scale(const float _scale);
/// \brief DO NOT CALL. ONLY TO BE CALLED BY ddEngine
void singleStep();

/// \brief Get time in in nanoseconds
/// \return raw 64-bit nanoseconds since initialization
uint64_t get_time();
/// \brief Get time in seconds
/// \return
float get_time_float();
/// \brief Get delta time since last frame render
/// \return
float get_frame_time();
/// \brief Get average frame time (average of last 30 frames)
/// \return
float get_avg_frame_time();
}  // namespace ddTime
