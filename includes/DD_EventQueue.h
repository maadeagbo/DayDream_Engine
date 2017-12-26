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

#include <initializer_list>
#include "DD_Input.h"
#include "DD_Timer.h"
#include "DD_Types.h"

struct handler_sig {
  handler_sig() {}
  handler_sig(const int gid, const int fid) : global_id(gid), func_id(fid) {}
  // handler_sig(std::initializer_list<handler_sig>) {}
  int global_id = -1;
  int func_id = -1;
};

struct DD_Queue {
  DD_Queue(const size_t size = 1000)
      : events_current(size),
        events_future(size / 2),
        lvl_call("_lvl_call"),
        lvl_call_i("_lvl_init"),
        res_call("_load_resources"),
        check_future("_process_future"),
        check_lvl_async("_check_lvl_async"),
        check_res_async("_check_res_async"),
        m_head(0),
        m_tail(0),
        m_numEvents(0),
        f_head(0),
        f_tail(0),
        f_numEvents(0),
        shutdown(false) {}

  /// \brief Add events to queue
  /// \param _event DD_LEvent event
  /// \return True if queue succesfully added event
  bool push(const DD_LEvent &_event);
  /// \brief Push event to queue from scripts
  /// \return Number of returned values
  int push_lua(lua_State *_L);
  /// \brief Add callback function to event queue from scripts
  /// \return Number of returned values
  int register_lua_func(lua_State *_L);
  /// \brief Register callback function with events
  /// \return Number of returned values
  int subscribe_lua_func(lua_State *_L);
  /// \brief Un-register callback function with events
  /// \return Number of returned values
  int unsubscribe_lua_func(lua_State *_L);
  /// \brief Add internal callback function to queue from system functions
  /// \param _sig Systen callback function
  void register_sys_func(const size_t _sig, SysEventHandler handler);
  /// \brief Remove callback function from queue
  /// \param _sig callback function signature
  void unregister_func(const size_t _sig);
  /// \brief Subscribe to event
  /// \param event_sig id for event
  /// \param event_sig id for callback function
  void subscribe(const size_t event_sig, const size_t _sig);
  /// \brief Unsubscribe from event
  /// \param event_sig id for event
  /// \param event_sig id for callback function
  void unsubscribe(const size_t event_sig, const size_t _sig);
  /// \brief ONLY CALLED INTERNALLY BY DD_Engine. DO NOT USE
  void process_queue();
  /// \brief ONLY CALLED INTERNALLY BY DD_Engine. DO NOT USE
  inline void shutdown_queue() { shutdown = true; }
  /// \brief ONLY CALLED INTERNALLY BY DD_Engine. DO NOT USE
  inline void set_lua_ptr(lua_State *_L) { L = _L; }
  /// \brief ONLY CALLED INTERNALLY BY DD_Engine. DO NOT USE
  void init_level_scripts(const char *script_id);

  cbuff<32> lvl_call;
  cbuff<32> lvl_call_i;
  cbuff<32> res_call;
  cbuff<32> check_future;
  cbuff<32> check_lvl_async;
  cbuff<32> check_res_async;

 private:
  /// \brief Add events to current queue
  /// \param _event DD_LEvent event
  bool push_current(const DD_LEvent &_event);
  /// \brief Add events to future queue
  /// \param _event DD_LEvent event
  bool push_future(const DD_LEvent &_event);
  /// \brief Remove events from future queue
  /// \param _event DD_LEvent event
  bool pop_future(DD_LEvent &_event);
  /// \brief Get next event from current queue
  /// \param _event DD_LEvent event
  bool pop_current(DD_LEvent &_event);
  /// \brief Add callback to callback_funcs
  /// \param id DD_Agent id
  /// \param sig lua callback function
  void register_handler(const size_t id, const handler_sig &sig);
  /// \brief Update and add events to main queue from future queue
  void process_future();

  lua_State *L;
  dd_array<DD_LEvent> events_current;  //< events to be processed this frame
  dd_array<DD_LEvent> events_future;   //< events to be processed the future
  DD_FuncBuff fb;
  size_t m_head, m_tail, m_numEvents;
  size_t f_head, f_tail, f_numEvents;
  // pools
  /// \brief the number of registered functions is stored in the first index
  /// of each node
  std::map<size_t, dd_array<size_t>> registered_events;
  std::map<size_t, handler_sig> callback_funcs;
  std::map<size_t, SysEventHandler> sys_funcs;
  std::future<void> async_lvl_init;
  std::future<void> async_resource;
  handler_sig lvl_init;
  handler_sig lvl_update;
  handler_sig lvl_res;
  bool shutdown;
};

/// \brief Function pointer for pushing events to queue
typedef std::function<bool(DD_LEvent &)> PushFunc;
