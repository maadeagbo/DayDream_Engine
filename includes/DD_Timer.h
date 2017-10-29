#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Timer:
*		- Reports high resolution clock time (in nanoseconds)
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"

namespace Timer {
	void	LoadStart();
	float	GetDeltaTime();

	u64		GetHiResTime();
	void 	sleep(float seconds);
	u64		SecsToNanoSecs(float seconds);
	float	NanoSecsToSecs(u64 nanosecs);
	u64		NanoSecsToMilli64(u64 nanosecs);
	const size_t f_hist = 30;
}

class DD_Timer
{
public:
	DD_Timer();
	~DD_Timer();

	void	update(const float fixedrate = -1.f);
	void	singleStep();
	inline void pause() { m_is_paused = true; }
	inline void unpause() { m_is_paused = false; }
	inline void setScale(const float _scale) { m_time_scale = _scale; }
	inline u64 getTime() const { return m_time_nano; }
	inline float getTimeFloat() const { return m_time_sec; }
	inline float getFrameTime() const { return m_ft[0]; }
	inline float getAvgFrameTime() const { return m_avg_ft; }
private:
	void	initHistory(const float value);
	void	updateHistory(const float frame_time);

	float	m_time_scale, m_time_sec, m_avg_ft;
	bool	m_is_paused, m_sethist;
	u64		m_time_nano, m_start_time;
	float	m_ft[Timer::f_hist];
};
