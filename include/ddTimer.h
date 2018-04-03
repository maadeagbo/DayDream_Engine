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

/** \brief Interace for retrieving time information w/ nano-second granularity
 */
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

/**
 * \brief Get time in in nanoseconds
 * \return raw 64-bit nanoseconds since initialization
 */
uint64_t get_time();
/**
 * \brief Get time in seconds
 * \return
 */
float get_time_float();
/**
 * \brief Get delta time since last frame render
 * \return
 */
float get_frame_time();
/**
 * \brief Get average frame time (average of last 30 frames)
 * \return
 */
float get_avg_frame_time();
}  // namespace ddTime
