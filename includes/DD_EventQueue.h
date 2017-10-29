#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Queue:
*		- Defines a circular queue (template)
*		- Defines events for messaging
*			- events will be specified by enums (bitwise flags)
*	TODO:
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"
#include "DD_Input.h"
#include "DD_Timer.h"

struct handler_sig
{
	EventHandler func;
	cbuff<64> sig;
};
typedef dd_array<handler_sig> callbackContainer;

struct DD_Queue
{
	DD_Queue(const size_t size = 100) :
		main_clock(nullptr),
		m_data(size),
		m_head(0),
		m_tail(0),
		m_numEvents(0),
		m_postLimit(0),
		m_postHandlers(50),
		m_flagRenderer(false)
	{}

	inline size_t head() const { return m_head; }
	inline size_t tail() const { return m_tail; }
	inline size_t numEvents() const { return m_numEvents; }

	bool push(const DD_Event& _event);
	bool pop(DD_Event& _event);
	bool RegisterHandler(EventHandler& handler, 
						 const char* ticket, 
						 const char* sig = "");
	bool RegisterPoster(EventHandler& handler, const char* sig = "");
	void GetPosts(const char* postID, const float dt, const float totalTime);
	void ProcessQueue();

	DD_Timer* main_clock;
	dd_array<DD_Event> m_data;
	size_t m_head, m_tail, m_numEvents, m_numPosted, m_postLimit;
	// pools
	std::map<std::string, callbackContainer> handlers;
	std::map<std::string, size_t> m_counter;
	callbackContainer m_postHandlers;

	bool m_flagRenderer;
};
