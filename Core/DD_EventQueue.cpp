#include "DD_EventQueue.h"
#include "DD_Terminal.h"

bool DD_Queue::push(const DD_LEvent &_event) {
  size_t qsize = events_current.size();
  if (m_numEvents == qsize) {
    return false;
  }
  events_current[m_tail] = std::move(_event);
  m_tail = (m_tail + 1) % qsize;
  m_numEvents++;
  return true;
}

int DD_Queue::register_lua_func(lua_State *L) {
  parse_lua_events(L, fb);

  // grab signature key
  int *s_val = fb.get_func_val<int>("sig.key");
  // grab global key
  const char *g_val = fb.get_func_val<const char>("global.name");
  // grab function key
  const char *f_val = fb.get_func_val<const char>("func.name");

  // cannot add new callback w/out function and signature
  if (f_val && s_val) {
    handler_sig callback;
    // add class function
    if (g_val) {
      callback.global_id = get_lua_ref(L, "", g_val);
      callback.func_id = get_lua_ref(L, g_val, f_val);
    } else {
      callback.func_id = get_lua_ref(L, "", f_val);
    }
    register_handler((size_t)*s_val, callback);
  }

  return 0;
}

int DD_Queue::subscribe_lua_func(lua_State *L) {
  parse_lua_events(L, fb);

  // grab event key
  int *e_val = fb.get_func_val<int>("event.key");
  // grab signature key
  int *s_val = fb.get_func_val<int>("sig.key");

  if (e_val && s_val) {
    subscribe((size_t)*e_val, (size_t)*s_val);
  }
  return 0;
}

int DD_Queue::unsubscribe_lua_func(lua_State *L) {
  parse_lua_events(L, fb);

  // grab event key
  int *e_val = fb.get_func_val<int>("event.key");
  // grab signature key
  int *s_val = fb.get_func_val<int>("sig.key");

  if (e_val && s_val) {
    unsubscribe((size_t)*e_val, (size_t)*s_val);
  }
  return 0;
}

bool DD_Queue::pop(DD_LEvent &_event) {
  size_t qsize = events_current.size();
  if (m_numEvents == 0) {
    return false;
  }
  _event = std::move(events_current[m_head]);
  m_head = (m_head + 1) % qsize;
  m_numEvents--;
  return true;
}

void DD_Queue::register_sys_func(syshandler_sig _sig) {
  bool registered = false;
  for (size_t i = 0; i < sys_callback_funcs.size() && !registered; i++) {
    // look for next slot
    if (sys_callback_funcs[i].sig < 0) {
      registered = true;
      sys_callback_funcs[i] = _sig;
    }
  }
}

void DD_Queue::unregister_func(const size_t _sig) {
  callback_funcs.erase(_sig);
}

void DD_Queue::subscribe(const size_t event_sig, const size_t _sig) {
  if (registered_events.find(event_sig) == registered_events.end()) {
    // if it doesn't exist, create new container
    registered_events[event_sig] = dd_array<size_t>(5);
    registered_events[event_sig][1] = _sig;
    registered_events[event_sig][0] = 1;  // store number of registered funcs
  } else {
    size_t full_q = registered_events[event_sig].size() - 1;
    if (registered_events[event_sig][0] == full_q) {
      // no more space, create more
      dd_array<size_t> temp(full_q + 1 + 10);
      temp = registered_events[event_sig];
      registered_events[event_sig] = std::move(temp);
    }
    // register new handler to event
    size_t &idx = registered_events[event_sig][0];
    registered_events[event_sig][idx + 1] = _sig;
    idx++;
  }
}

void DD_Queue::register_handler(const size_t id, const handler_sig &sig) {
  callback_funcs[id] = sig;
}

void DD_Queue::process_callback_buff() {
  for (unsigned i = 0; i < cb.num_events; i++) {
    DD_LEvent &_event = cb.buffer[i];
    if (_event.delay > 0) {  // add to future queue
      push_future(_event);
    } else {
      push(_event);
    }
  }
}

void DD_Queue::process_future() {
  unsigned num_events = (unsigned)f_numEvents;
  for (unsigned i = 0; i < num_events; i++) {
    DD_LEvent _event;
    pop_future(_event);

    if (_event.delay == 0) {
      push(_event);
    } else {
      _event.delay--;
      push_future(_event);
    }
  }
}

void DD_Queue::unsubscribe(const size_t event_sig, const size_t _sig) {
  if (registered_events.find(event_sig) != registered_events.end()) {
    dd_array<size_t> &func_sigs = registered_events[event_sig];
    bool removed = false;
    for (size_t i = 1; i <= func_sigs[0] && !removed; i++) {
      // use replacement and decrement to remove
      if (func_sigs[i] == _sig) {
        removed = true;
        func_sigs[i] = func_sigs[func_sigs[0]];
        func_sigs[0]--;
      }
    }
  }
}

void DD_Queue::process_queue() {
  while (!shutdown) {
    DD_LEvent _event;
    pop(_event);

    size_t e_sig = _event.handle.gethash();
    // check if event is a system call
    if (_event.handle == sys_call) {
      if (_event.active == 1 && _event.args[0].val.type == VType::STRING) {
        // get key
        size_t idx = _event.args[0].val.v_strptr.gethash();
        for (size_t i = 0; i < sys_callback_funcs.size(); i++) {
          // call system function if key matches
          if (sys_callback_funcs[i].sig == idx) {
            sys_callback_funcs[i].func(_event);
          }
        }
      }
    }
    // check if event is to process backed-up/future events
    else if (_event.handle == check_future) {
      process_future();
    }
    // check if event is level update call
    else if (_event.handle == lvl_call) {
      callback_lua(L, _event, lvl_update.func_id, lvl_update.global_id, &cb);
      process_callback_buff();
    }
    // check if event is level initialize call
    else if (_event.handle == lvl_call_i) {
      callback_lua(L, _event, lvl_init.func_id, lvl_init.global_id, &cb);
      process_callback_buff();
    }
    // event can be processed by scripts
    else if (registered_events.find(e_sig) != registered_events.end()) {
      dd_array<size_t> &func_sigs = registered_events[e_sig];

      for (size_t i = 1; i <= func_sigs[0]; i++) {
        handler_sig &handle = callback_funcs[func_sigs[i]];
        callback_lua(L, _event, handle.func_id, handle.global_id, &cb);
        process_callback_buff();
      }
    }
  }
}

void DD_Queue::push_future(const DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == qsize) {
    return;
  }

  events_future[f_tail] = std::move(_event);
  f_tail = (f_tail + 1) % qsize;
  f_numEvents++;
}

void DD_Queue::pop_future(DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == 0) {
    return;
  }

  _event = std::move(events_future[f_head]);
  f_head = (f_head + 1) % qsize;
  f_numEvents--;
}
