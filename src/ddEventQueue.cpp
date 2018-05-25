#include "ddEventQueue.h"
#include "ddTerminal.h"

namespace {
string32 event_id_hash("event_id");
string32 delay_hash("delay");
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
      _event.delay = *fb.get_func_val<int64_t>(delay_hash.str());
    }
    // log remaining arguments
    else {
      switch (fb.buffer[i].arg.type) {
        case VType::BOOL:
          add_arg_LEvent<bool>(&_event, fb.buffer[i].arg_name.str(),
                               fb.buffer->arg.v_bool);
          break;
        case VType::INT:
          add_arg_LEvent<int64_t>(&_event, fb.buffer[i].arg_name.str(),
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
  // attempt to add event to queue
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
  string32 key;
  int global_ref = LUA_REFNIL, func_ref = LUA_REFNIL;

  int top = lua_gettop(_L); /* number of events */
  for (int i = 1; i <= top; i++) {
    int t = lua_type(_L, i);
    switch (t) {
      case LUA_TSTRING:
        // grab key to object from top of stack
        key = lua_tostring(_L, i);
        break;
      case LUA_TTABLE:
        // grab lua table object from top of stack
        global_ref = get_lua_object(_L);
        if (global_ref != LUA_REFNIL) {
          func_ref = get_lua_ref(_L, global_ref, "update");
        }
        break;
      default:
        break;
    }
  }
  // clear stack
  top = lua_gettop(L);
  lua_pop(_L, top);

  if (*key.str() && func_ref != LUA_REFNIL) {
    handler_sig _sig = {global_ref, func_ref};
    register_handler(key.gethash(), _sig);
  }
  return 0;
}

int ddQueue::subscribe_lua_func(lua_State *_L) {
  parse_lua_events(_L, fb);

  // grab event key
  const char *k_val = fb.get_func_val<const char>("key");
  // grab signature key
  const char *e_val = fb.get_func_val<const char>("event");

  if (e_val && k_val) {
    subscribe(StrLib::get_char_hash(e_val), StrLib::get_char_hash(k_val));
  }
  return 0;
}

int ddQueue::unsubscribe_lua_func(lua_State *_L) {
  parse_lua_events(_L, fb);

  // grab event key
  int64_t *e_val = fb.get_func_val<int64_t>("event.key");
  // grab signature key
  int64_t *s_val = fb.get_func_val<int64_t>("sig.key");

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

const string32 frame_enter_hash("frame_init");

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
    // check if event is lvl init call
    else if (_event.handle == lvl_call_i) {
      _event.handle = "init";
      // async level init (spawn new lua thread)
      lua_State *L1 = lua_newthread(L);
      async_lvl_init =
          std::async(std::launch::async, callback_lua, L1, _event,
                     lvl_init.func_id, std::ref(fb), lvl_init.global_id);
      // send delay event
      DD_LEvent a_event;
      a_event.handle = check_lvl_async;
      a_event.delay = 30;
      push_future(a_event);
    }
    // check if event is resource load call
    else if (_event.handle == res_call) {
      // async resources load (spawn new lua thread)
      lua_State *L1 = lua_newthread(L);
      async_resource =
          std::async(std::launch::async, callback_lua, L1, _event,
                     lvl_res.func_id, std::ref(fb), lvl_res.global_id);
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
            // pop spawned lua thread
            lua_pop(L, 1);
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
            // pop spawned lua thread
            lua_pop(L, 1);
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
        // system call
        if (sys_funcs.count(func_sigs[i]) > 0) {
          SysEventHandler &handle = sys_funcs[func_sigs[i]];
          handle(_event);
        } else {
          // scripts
          if (callback_funcs.count(func_sigs[i]) > 0) {
            handler_sig &handle = callback_funcs[func_sigs[i]];
            callback_lua(L, _event, handle.func_id, fb, handle.global_id);
          }
        }
      }
    }
  }
}

void ddQueue::setup_lua(lua_State *_L) {
  L = _L;
  // add event queue instance to lua space to register functions
  register_instance_lua_xspace<ddQueue>(L, *this);
  // register functions
  register_callback_lua(L, "dd_register_callback",
                        &dispatch_<ddQueue, &ddQueue::register_lua_func>);
  register_callback_lua(L, "dd_subscribe",
                        &dispatch_<ddQueue, &ddQueue::subscribe_lua_func>);
  register_callback_lua(L, "dd_push", &dispatch_<ddQueue, &ddQueue::push_lua>);
}

void ddQueue::init_level_scripts(const char *script_id, const bool runtime) {
  // find level functions
  string256 file_name;
  file_name.format("%s/%s/%s_world.lua", PROJECT_DIR, script_id, script_id);
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

        // subscribe for certain callback events
        handler_sig _sig = {global_ref, func_ref};
        size_t curr_lvl_id = StrLib::get_char_hash("lvl_update");
        register_handler(curr_lvl_id, _sig);

        subscribe(lvl_call.gethash(), curr_lvl_id);
        subscribe(physics_tick.gethash(), curr_lvl_id);
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
  if (!runtime) {
    // find load function
    file_name.format("%s/%s/%s_assets.lua", PROJECT_DIR, script_id, script_id);
    // file_name.format("%s/scripts/%s_assets.lua", RESOURCE_DIR, script_id);
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
}

bool ddQueue::push_current(const DD_LEvent &_event) {
  size_t qsize = events_current.size();
  if (m_numEvents == qsize) {
    return false;
  }
  events_current[m_tail] = _event;
  m_tail = (m_tail + 1) % qsize;
  m_numEvents++;
  return true;
}

bool ddQueue::push_future(const DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == qsize) {
    return false;
  }

  events_future[f_tail] = _event;
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
