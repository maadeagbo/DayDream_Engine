#pragma once

#include "DD_Types.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#define MAX_CALLBACK_EVENTS 10
#define MAX_EVENT_ARGS 8

/// \brief class for parsing Varying data struct
enum class VType : unsigned { INT, BOOL, FLOAT, STRING };

/// \brief Data type for event values
template <unsigned str_size>
struct Varying {
  Varying() : v_strptr() {}
  VType type;

  union {
    cbuff<str_size> v_strptr;
    int32_t v_int;
    float v_float;
    bool v_bool;
  };
};

/// \brief Data type for events passed thru callbacks
struct DD_LEvent {
  // DD_LEvent() : args(0) {}
  struct KeyVal {
    cbuff<32> key;
    Varying<32> val;
  };

  cbuff<32> handle;
  KeyVal args[MAX_EVENT_ARGS];
  unsigned active = 0;
  unsigned delay = 0;
  unsigned priority = 0;
};

// system event handler signature
typedef std::function<void(DD_LEvent&)> SysEventHandler;

/// \brief Add argument to DD_LEvent
template <typename T>
bool add_arg_LEvent(DD_LEvent *levent, const char *key, T arg) {
  return false;
}

template <>
bool add_arg_LEvent<int>(DD_LEvent *levent, const char *key, int arg);
template <>
bool add_arg_LEvent<float>(DD_LEvent *levent, const char *key, float arg);
template <>
bool add_arg_LEvent<bool>(DD_LEvent *levent, const char *key, bool arg);
template <>
bool add_arg_LEvent<const char *>(DD_LEvent *levent, const char *key,
                                  const char *arg);

/// \brief Buffer to hold evens from callback function
struct DD_CallBackBuff {
  /// \brief Get new event from buffer (return nullptr if buffer is full)
  DD_LEvent *get_new_event();
  /// \brief Parse DD_CallBackBuff arguments to into values
  template <typename T>
  T *get_callback_val(const char *ckey) {
    return nullptr;
  }

  /// \brief resets DD_CallBackBuff
  void clear_callbackbuff();

  DD_LEvent buffer[MAX_CALLBACK_EVENTS];
  unsigned num_events = 0;
};

template <>
const char *DD_CallBackBuff::get_callback_val<const char>(const char *ckey);
template <>
bool *DD_CallBackBuff::get_callback_val<bool>(const char *ckey);
template <>
float *DD_CallBackBuff::get_callback_val<float>(const char *ckey);
template <>
int *DD_CallBackBuff::get_callback_val<int>(const char *ckey);

/// \brief Data type for passing function arguments to C++ from Lua
struct DD_LFuncArg {
  cbuff<32> arg_name;
  Varying<256> arg;
};

/// \brief Buffer to hold arguments from Lua invocations of C++ functions
struct DD_FuncBuff {
  /// \brief Get empty argument from buffer (return nullptr if buffer is full)
  DD_LFuncArg *get_next_arg();
  /// \brief return DD_FuncArgs values
  template <typename T>
  T *get_func_val(const char *ckey) {
    return nullptr;
  }

  DD_LFuncArg buffer[MAX_CALLBACK_EVENTS];
  unsigned num_args = 0;
};

template <>
int *DD_FuncBuff::get_func_val<int>(const char *ckey);
template <>
float *DD_FuncBuff::get_func_val<float>(const char *ckey);
template <>
bool *DD_FuncBuff::get_func_val<bool>(const char *ckey);
template <>
const char *DD_FuncBuff::get_func_val<const char>(const char *ckey);

/// \brief Initialize Lua VM
lua_State *init_lua_state();

/// \brief Parse the arguments from an error (lua) and end lua VM
template <typename... Args>
void parse_error(lua_State *L, const char *fmt, const Args &... args) {
  fprintf(stderr, fmt, args...);
  lua_close(L);
  exit(EXIT_FAILURE);
}

/// \brief Register instance into lua_extraspace
template <class T>
void register_instance_lua_xspace(lua_State *L, T &instance) {
  *static_cast<T **>(lua_getextraspace(L)) = &instance;
}

/// \brief This template wraps a member function into a C-style "free" function
/// compatible with lua.
template <typename T, int (T::*f)(lua_State *L)>
int dispatch_(lua_State *L) {
  T *ptr = *static_cast<T **>(lua_getextraspace(L));
  return ((*ptr).*f)(L);
}

/// \brief Append a path to lua package path to search for scripts
void append_package_path(lua_State *L, const char *path);

/// \brief Check lua stack value for nil (returns true if nil)
bool check_stack_nil(lua_State *L, int idx);

/// \brief Register class function w/ dispatch template
void register_callback_lua(lua_State *L, const char *func_sig,
                           lua_CFunction _func);

/// \brief Read and execute lua script file
bool parse_luafile(lua_State *L, const char *filename);

/// \brief Envoke lua callback function and return event
//void callback_lua(lua_State *L, const DD_LEvent &event, DD_CallBackBuff &cb,
//                  const char *func, const char *lclass = "");

/// \brief Envoke lua callback function with lua_ref pointer
void callback_lua(lua_State *L, const DD_LEvent &levent, int func_ref,
                  int global_ref = LUA_REFNIL, DD_CallBackBuff *cb = nullptr,
                  DD_FuncBuff *fb = nullptr);

/// \brief Push event arguments onto the stack
void push_args(lua_State *L, const DD_LEvent &levent, const int idx);

/// \brief Dump stack
void stack_dump(lua_State *L);

/// \brief Parse all events to callback buffer
void parse_lua_events(lua_State *L, DD_CallBackBuff &cb);

/// \brief Parse all events to function argument buffer
void parse_lua_events(lua_State *L, DD_FuncBuff &fb);

/// \brief Parse table into DD_LEvent
void parse_table(lua_State *L, DD_LEvent *levent, const int tabs = 0);

/// \brief Parse table into DD_LEvent
void parse_table(lua_State *L, DD_FuncBuff *fb, const int tabs = 0);

/// \brief Print tables for returned events (debug)
void print_table(lua_State *L, const int tabs = 0);

/// \brief Print out contents of DD_CallbackBuff
void print_callbackbuff(DD_CallBackBuff &cb);

/// \brief Store and return handle to lua function
int get_lua_ref(lua_State *L, const char *lclass, const char *func);

/// \brief Clear function reference
void clear_lua_ref(lua_State *L, int func_ref);