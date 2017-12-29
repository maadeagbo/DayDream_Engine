#include "ddEventQueue.h"
#include "ddTerminal.h"

namespace {
cbuff<32> event_id_hash("event_id");
cbuff<32> delay_hash("delay");
}  // namespace

bool ddQueue::push(const DD_LEvent &_event) {
  switch (_event.delay) {
    case 0:
      return push_current(_event);
    default:
      return push_future(_event);
  }
}

int ddQueue::push_lua(lua_State *_L) {
  parse_lua_events(_L, fb);

  DD_LEvent _event;
  // set up event
  bool push_flag = false;
  for (unsigned i = 0; i < fb.num_args; i++) {
    // log event_id
    if (fb.buffer[i].arg_name == event_id_hash) {
      push_flag = true;
      _event.handle = fb.get_func_val<const char>(event_id_hash.str());
    }
    // log delay
    else if (fb.buffer[i].arg_name == delay_hash) {
      _event.delay = *fb.get_func_val<int>(delay_hash.str());
    }
    // log remaining arguments
    else {
      switch (fb.buffer[i].arg.type) {
        case VType::BOOL:
          add_arg_LEvent<bool>(&_event, fb.buffer[i].arg_name.str(),
                               fb.buffer->arg.v_bool);
          break;
        case VType::INT:
          add_arg_LEvent<int>(&_event, fb.buffer[i].arg_name.str(),
                              fb.buffer->arg.v_int);
          break;
        case VType::FLOAT:
          add_arg_LEvent<float>(&_event, fb.buffer[i].arg_name.str(),
                                fb.buffer->arg.v_float);
          break;
        case VType::STRING:
          add_arg_LEvent<const char *>(&_event, fb.buffer[i].arg_name.str(),
                                       fb.buffer->arg.v_strptr.str());
          break;
      }
    }
  }
  if (push_flag) {
    if (!push(_event)) {
      // failed to add event
      ddTerminal::f_post("[error]push_lua::Failed to add event <%s>",
                         _event.handle.str());
    }
  }
  return 0;
}

int ddQueue::register_lua_func(lua_State *_L) {
  // grab lua table object from top of stack
  int global_ref = LUA_REFNIL, func_ref = LUA_REFNIL;
  global_ref = get_lua_object(L);
  if (global_ref != LUA_REFNIL) {
    func_ref = get_lua_ref(L, global_ref, "update");
  }
  // grab key to object from top of stack
  parse_lua_events(_L, fb);

  int *key = fb.get_func_val<int>("key");
  if (key && func_ref != LUA_REFNIL) {
    handler_sig _sig = {global_ref, func_ref};
    register_handler((size_t)*key, _sig);
  }
  return 0;
}

int ddQueue::subscribe_lua_func(lua_State *_L) {
  parse_lua_events(_L, fb);

  // grab event key
  int *e_val = fb.get_func_val<int>("event.key");
  // grab signature key
  int *s_val = fb.get_func_val<int>("sig.key");

  if (e_val && s_val) {
    subscribe((size_t)*e_val, (size_t)*s_val);
  }
  return 0;
}

int ddQueue::unsubscribe_lua_func(lua_State *_L) {
  parse_lua_events(_L, fb);

  // grab event key
  int *e_val = fb.get_func_val<int>("event.key");
  // grab signature key
  int *s_val = fb.get_func_val<int>("sig.key");

  if (e_val && s_val) {
    unsubscribe((size_t)*e_val, (size_t)*s_val);
  }
  return 0;
}

bool ddQueue::pop_current(DD_LEvent &_event) {
  size_t qsize = events_current.size();
  if (m_numEvents == 0) {
    return false;
  }
  _event = std::move(events_current[m_head]);
  m_head = (m_head + 1) % qsize;
  m_numEvents--;
  return true;
}

void ddQueue::register_sys_func(const size_t _sig, SysEventHandler handler) {
  sys_funcs[_sig] = handler;
}

void ddQueue::unregister_func(const size_t _sig) {
  // can only unregister lua functions
  if (callback_funcs.find(_sig) != callback_funcs.end()) {
    callback_funcs.erase(_sig);
  }
}

void ddQueue::subscribe(const size_t event_sig, const size_t _sig) {
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

void ddQueue::register_handler(const size_t id, const handler_sig &sig) {
  callback_funcs[id] = sig;
}

void ddQueue::process_future() {
  unsigned num_events = (unsigned)f_numEvents;
  for (unsigned i = 0; i < num_events; i++) {
    DD_LEvent _event;
    pop_future(_event);

    if (_event.delay == 0) {  // add to normal queue
      push(_event);
    } else {  // update then send back to future queue
      _event.delay--;
      push_future(_event);
    }
  }
}

void ddQueue::unsubscribe(const size_t event_sig, const size_t _sig) {
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

void ddQueue::process_queue() {
  while (!shutdown) {
    DD_LEvent _event;
    bool event_present = pop_current(_event);
    if (!event_present) continue;

    size_t e_sig = _event.handle.gethash();
    // check if event is to process backed-up/future events
    if (_event.handle == check_future) {
      process_future();
    }
    // check if event is level call
    else if (_event.handle == lvl_call) {
      callback_lua(L, _event, fb, lvl_update.func_id, lvl_update.global_id);
      // produce event after call (blank event or spatial event)
    }
    // check if event is lvl init call
    else if (_event.handle == lvl_call_i) {
      // async level init
      async_lvl_init =
          std::async(std::launch::async, callback_lua, L, std::ref(_event),
                     std::ref(fb), lvl_init.func_id, lvl_init.global_id);
      // send delay event
      DD_LEvent a_event;
      a_event.handle = check_lvl_async;
      a_event.delay = 30;
      push_future(a_event);
    }
    // check if event is resource load call
    else if (_event.handle == res_call) {
      // async resources load
      async_resource =
          std::async(std::launch::async, callback_lua, L, std::ref(_event),
                     std::ref(fb), lvl_res.func_id, lvl_res.global_id);
      // send delay event
      DD_LEvent a_event;
      a_event.handle = check_res_async;
      a_event.delay = 30;
      push_future(a_event);
    }
    // check if async lvl call is complete
    else if (_event.handle == check_lvl_async ||
             _event.handle == check_res_async) {
      std::chrono::seconds timespan(0);
      DD_LEvent a_event;
      const int async_flag = (_event.handle == check_lvl_async) ? 0 : 1;

      switch (async_flag) {
        case 0:  // check level init function
          if (async_lvl_init.wait_for(timespan) == std::future_status::ready) {
            // create completion event
            a_event.handle = "_lvl_init_done";
            push(a_event);
          } else {
            // create delay event
            a_event.handle = check_lvl_async;
            a_event.delay = 30;
            push_future(a_event);
          }
          break;
        case 1:  // check resource load function
          if (async_resource.wait_for(timespan) == std::future_status::ready) {
            // create completion event
            a_event.handle = "_load_resource_done";
            push(a_event);
          } else {
            // create delay event
            a_event.handle = check_res_async;
            a_event.delay = 30;
            push_future(a_event);
          }
          break;
        default:
          break;
      }
    }
    // event can be processed by scripts or system calls
    else if (registered_events.find(e_sig) != registered_events.end()) {
      dd_array<size_t> &func_sigs = registered_events[e_sig];

      for (size_t i = 1; i <= func_sigs[0]; i++) {
        // script
        if (callback_funcs.count(func_sigs[i]) > 0) {
          handler_sig &handle = callback_funcs[func_sigs[i]];
          callback_lua(L, _event, fb, handle.func_id, handle.global_id);
        }
        // system call
        if (sys_funcs.count(func_sigs[i]) > 0) {
          SysEventHandler &handle = sys_funcs[func_sigs[i]];
          handle(_event);
        }
      }
    }
  }
}

void ddQueue::init_level_scripts(const char *script_id) {
  // find level functions
  cbuff<256> file_name;
  file_name.format("%sscripts/%s.lua", ROOT_DIR, script_id);
  bool file_found = parse_luafile(L, file_name.str());
  if (file_found) {
    int global_ref = get_lua_ref(L, nullptr, script_id);
    if (global_ref != LUA_REFNIL) {
      // get init function
      int func_ref = get_lua_ref(L, global_ref, "init");
      if (func_ref != LUA_REFNIL) {
        // add queue handle
        lvl_init = {global_ref, func_ref};
      } else {
        ddTerminal::f_post("init_level_scripts::Failed to find: %s::init",
                           script_id);
      }
      // get update function
      func_ref = get_lua_ref(L, global_ref, "update");
      if (func_ref != LUA_REFNIL) {
        // add queue handle
        lvl_update = {global_ref, func_ref};
      } else {
        ddTerminal::f_post("init_level_scripts::Failed to find: %s::update",
                           script_id);
      }
    } else {
      ddTerminal::f_post("init_level_scripts::Failed to find: %s", script_id);
    }
  } else {
    ddTerminal::f_post("init_level_scripts::Failed to open <%s>",
                       file_name.str());
  }
  // find load function
  file_name.format("%sscripts/%s_assets.lua", ROOT_DIR, script_id);
  file_found = parse_luafile(L, file_name.str());
  if (file_found) {
    int func_ref = get_lua_ref(L, nullptr, "load");
    if (func_ref != LUA_REFNIL) {
      // add queue handle
      lvl_res = {-1, func_ref};
    } else {
      ddTerminal::f_post("init_level_scripts::Failed to find: %s_asset::load",
                         script_id);
    }
  } else {
    ddTerminal::f_post("init_level_scripts::Failed to open <%s>",
                       file_name.str());
  }
}

bool ddQueue::push_current(const DD_LEvent &_event) {
  size_t qsize = events_current.size();
  if (m_numEvents == qsize) {
    return false;
  }
  events_current[m_tail] = std::move(_event);
  m_tail = (m_tail + 1) % qsize;
  m_numEvents++;
  return true;
}

bool ddQueue::push_future(const DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == qsize) {
    return false;
  }

  events_future[f_tail] = std::move(_event);
  f_tail = (f_tail + 1) % qsize;
  f_numEvents++;
  return true;
}

bool ddQueue::pop_future(DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == 0) {
    return false;
  }

  _event = std::move(events_future[f_head]);
  f_head = (f_head + 1) % qsize;
  f_numEvents--;
  return true;
}
