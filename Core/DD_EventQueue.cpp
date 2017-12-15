#include "DD_EventQueue.h"
#include "DD_Terminal.h"

bool DD_Queue::push(const DD_LEvent &_event) {
  switch (_event.delay) {
		case 0:
			return push_current(_event);
		default:
			return push_future(_event);
	}
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

bool DD_Queue::pop_current(DD_LEvent &_event) {
  size_t qsize = events_current.size();
  if (m_numEvents == 0) {
    return false;
  }
  _event = std::move(events_current[m_head]);
  m_head = (m_head + 1) % qsize;
  m_numEvents--;
  return true;
}

void DD_Queue::register_sys_func(const size_t _sig, SysEventHandler handler) {
	sys_funcs[_sig] = handler;
}

void DD_Queue::unregister_func(const size_t _sig) {
	// can only unregister lua functions
	if (callback_funcs.find(_sig) != callback_funcs.end()) {
		callback_funcs.erase(_sig);
	}
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
		push(cb.buffer[i]);
  }
}

void DD_Queue::process_future() {
  unsigned num_events = (unsigned)f_numEvents;
  for (unsigned i = 0; i < num_events; i++) {
    DD_LEvent _event;
    pop_future(_event);

    if (_event.delay == 0) {	// add to normal queue
      push(_event);
    } else {									// update then send back to future queue
      _event.delay--;
      push_future(_event);
    }
  }
}

int DD_Queue::next_async_slot() {
	for (unsigned i = 0; i < (unsigned)ASYNC_BUFF_MAX; i++) {
		if (async_free_list[i]) {
			async_free_list[i] = false;
			return i;
		}
	}
	return -1;
}

void DD_Queue::process_async_call(const unsigned idx, const char *tag,
																	const char *reciever) {
	if (idx >= ASYNC_BUFF_MAX && idx >= 0 && !tag) {
		// check status of async
		std::future<void> &call = async_buffer[idx];
		if (call.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			// create finish event
			DD_LEvent a_event;
			a_event.handle = reciever;
			add_arg_LEvent<const char*>(&a_event, "tag", tag);
			push(a_event);
		} else {
			// create delay event
			DD_LEvent a_event;
			a_event.handle = "_check_async";
			add_arg_LEvent<int>(&a_event, "idx", idx);
			add_arg_LEvent<const char*>(&a_event, "tag", tag);
			add_arg_LEvent<const char*>(&a_event, "reciever", reciever);
			a_event.delay = 30;
			push_future(a_event);
		}
	} else {
		// print error
		const char *_t = (tag) ? tag : "nullptr";
		DD_Terminal::f_post("[error] Async processing failure. %d::%s::%s",
												idx, _t, reciever);
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
		pop_current(_event);

    size_t e_sig = _event.handle.gethash();
    // check if event is to process backed-up/future events
    if (_event.handle == check_future) {
      process_future();
    }
    // check if event is level call
    else if (_event.handle == lvl_call) {
      callback_lua(L, _event, lvl_update.func_id, lvl_update.global_id, &cb);
      process_callback_buff();
    }
    // check if event is system async call
    else if (_event.handle == async_call) {
			const char *_tag = get_arg_LEvent<const char>(&_event, "tag");
			const char *_reciever = get_arg_LEvent<const char>(&_event, "reciever");
			int idx = next_async_slot();
			// async level init
			if (idx >= 0 && _tag && _reciever && lvl_call_i.compare(_tag) == 0) {
				async_buffer[idx] = std::async(std::launch::async, callback_lua, L, 
																			 _event, lvl_init.func_id, 
																			 lvl_init.global_id, &cb, nullptr);
				// send delay event
				DD_LEvent a_event;
				a_event.handle = "_check_async";
				add_arg_LEvent<int>(&a_event, "idx", idx);
				add_arg_LEvent<const char*>(&a_event, "tag", "lvl init done");
				add_arg_LEvent<const char*>(&a_event, "reciever", _reciever);
				a_event.delay = 30;
				push_future(a_event);
			} else {
				// print error message to terminal
				_tag = (_tag) ? _tag : "(t)nullptr";
				_reciever = (_reciever) ? _reciever : "(r)nullptr";
				DD_Terminal::f_post("[error] Async init failure. %d::%s::%s",
														idx, _tag, _reciever);
				// free index
				if (idx >= 0) { async_free_list[idx] = true; }
			}
    }
		// check if event is async call check
		else if (_event.handle == check_async) {
			const char *_tag = get_arg_LEvent<const char>(&_event, "tag");
			const char *_reciever = get_arg_LEvent<const char>(&_event, "reciever");
			int *idx = get_arg_LEvent<int>(&_event, "idx");

			process_async_call(*idx, _tag, _reciever);
		}
    // event can be processed by scripts or system calls
    else if (registered_events.find(e_sig) != registered_events.end()) {
      dd_array<size_t> &func_sigs = registered_events[e_sig];

      for (size_t i = 1; i <= func_sigs[0]; i++) {
				// script
				if (callback_funcs.count(func_sigs[i]) > 0) {
					handler_sig &handle = callback_funcs[func_sigs[i]];
					callback_lua(L, _event, handle.func_id, handle.global_id, &cb);
					process_callback_buff();
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

bool DD_Queue::push_current(const DD_LEvent & _event) {
	size_t qsize = events_current.size();
	if (m_numEvents == qsize) {
		return false;
	}
	events_current[m_tail] = std::move(_event);
	m_tail = (m_tail + 1) % qsize;
	m_numEvents++;
	return true;
}

bool DD_Queue::push_future(const DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == qsize) {
    return false;
  }

  events_future[f_tail] = std::move(_event);
  f_tail = (f_tail + 1) % qsize;
  f_numEvents++;
	return true;
}

bool DD_Queue::pop_future(DD_LEvent &_event) {
  size_t qsize = events_future.size();
  if (f_numEvents == 0) {
    return false;
  }

  _event = std::move(events_future[f_head]);
  f_head = (f_head + 1) % qsize;
  f_numEvents--;
	return true;
}
