#pragma once

// simple template container library
#include "Container.h"
// simple string library
#include <inttypes.h>
#include <functional>
#include "StringLib.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#define MAX_ARG_BUFFER_SIZE 16
#define MAX_EVENT_ARGS 8

/** \brief class for parsing Varying data struct */
enum class VType : unsigned { INT, BOOL, FLOAT, STRING };

/**
 * \brief Data type for event values
 */
template <unsigned str_size>
struct Varying {
  Varying() : v_strptr() {}
  VType type;

  union {
    cbuff<str_size> v_strptr;
    int64_t v_int;
    float v_float;
    bool v_bool;
  };
};

/**
 * \brief Data type for events passed thru callbacks
 */
struct DD_LEvent {
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

/** system event handler signature */
typedef std::function<void(DD_LEvent &)> SysEventHandler;

/**
 * \brief Add argument to DD_LEvent
 */
template <typename T>
bool add_arg_LEvent(DD_LEvent *levent, const char *key, T arg) {
  return false;
}

template <>
bool add_arg_LEvent<int64_t>(DD_LEvent *levent, const char *key, int64_t arg);
template <>
bool add_arg_LEvent<float>(DD_LEvent *levent, const char *key, float arg);
template <>
bool add_arg_LEvent<bool>(DD_LEvent *levent, const char *key, bool arg);
template <>
bool add_arg_LEvent<const char *>(DD_LEvent *levent, const char *key,
                                  const char *arg);

/**
 * brief Get argument from DD_LEvent
 */
template <typename T>
T *get_arg_LEvent(DD_LEvent *levent, const char *key) {
  return nullptr;
}

template <>
int64_t *get_arg_LEvent<int64_t>(DD_LEvent *levent, const char *key);
template <>
float *get_arg_LEvent<float>(DD_LEvent *levent, const char *key);
template <>
bool *get_arg_LEvent<bool>(DD_LEvent *levent, const char *key);
template <>
const char *get_arg_LEvent<const char>(DD_LEvent *levent, const char *key);

/**
 * \brief Data type for passing information between C++ and Lua
 */
struct DD_LFuncArg {
  cbuff<32> arg_name;
  Varying<256> arg;
};

/**
 * \brief Buffer to hold arguments from Lua invocations of C++ functions
 */
struct DD_FuncBuff {
  /**
   * \brief Get empty argument from buffer (return nullptr if buffer is full)
   */
  DD_LFuncArg *get_next_arg();
  /**
   * \brief return DD_FuncArgs values
   */
  template <typename T>
  T *get_func_val(const char *ckey) {
    return nullptr;
  }

  DD_LFuncArg buffer[MAX_ARG_BUFFER_SIZE];
  unsigned num_args = 0;
};

template <>
int64_t *DD_FuncBuff::get_func_val<int64_t>(const char *ckey);
template <>
float *DD_FuncBuff::get_func_val<float>(const char *ckey);
template <>
bool *DD_FuncBuff::get_func_val<bool>(const char *ckey);
template <>
const char *DD_FuncBuff::get_func_val<const char>(const char *ckey);

/**
 * \brief Initialize Lua VM
 */
lua_State *init_lua_state();

/**
 * \brief Parse the arguments from an error (lua) and end lua VM
 */
template <typename... Args>
void parse_error(lua_State *L, const char *fmt, const Args &... args) {
  fprintf(stderr, fmt, args...);
  lua_close(L);
  exit(EXIT_FAILURE);
}

/**
 * \brief Register instance into lua_extraspace
 */
template <class T>
void register_instance_lua_xspace(lua_State *L, T &instance) {
  *static_cast<T **>(lua_getextraspace(L)) = &instance;
}

/**
 * \brief This template wraps a member function into a C-style "free" function
 * compatible with lua.
 */
template <typename T, int (T::*f)(lua_State *L)>
int dispatch_(lua_State *L) {
  T *ptr = *static_cast<T **>(lua_getextraspace(L));
  return ((*ptr).*f)(L);
}

/**
 * \brief Append a path to lua package path to search for scripts
 */
void append_package_path(lua_State *L, const char *path);

/**
 * \brief Check lua stack value for nil (returns true if nil)
 * \return True if nil
 */
bool check_stack_nil(lua_State *L, int idx);

/**
 * \brief Register class function w/ dispatch template
 */
void register_callback_lua(lua_State *L, const char *func_sig,
                           lua_CFunction _func);

/**
 * \brief Read and execute lua script file
 * \return Ture if successfully opened
 */
bool parse_luafile(lua_State *L, const char *filename);

/**
 * \brief Envoke lua callback function with lua_ref pointer
 */
void callback_lua(lua_State *L, const DD_LEvent levent, int func_ref,
                  DD_FuncBuff &fb, int global_ref = LUA_REFNIL);

/**
 * \brief Push event arguments onto the stack
 */
void push_args(lua_State *L, const DD_LEvent &levent, const int idx);

/**
 * \brief Dump stack
 */
void stack_dump(lua_State *L);

/**
 * \brief Parse all events to function argument buffer
 */
void parse_lua_events(lua_State *L, DD_FuncBuff &fb);

/**
 * \brief Parse table into DD_LEvent
 */
void parse_table(lua_State *L, DD_LEvent *levent, const int tabs = 0);

/**
 * \brief Parse table into DD_LEvent
 */
void parse_table(lua_State *L, DD_FuncBuff *fb, const int tabs = 0);

/**
 * \brief Print tables for returned events (debug)
 */
void print_table(lua_State *L, const int tabs = 0);

/**
 * \brief Print out contents of buffer
 */
void print_buffer(DD_FuncBuff &fb);

/**
 * \brief Store and return handle to lua function
 * \return Integer handle to function or class
 */
int get_lua_ref(lua_State *L, const char *lclass, const char *func);

/**
 * \brief Store and return handle to lua function
 * \return Integer handle to function or class
 */
int get_lua_ref(lua_State *L, int lclass, const char *func);

/**
 * \brief Clear function reference
 */
void clear_lua_ref(lua_State *L, int func_ref);

/**
 * \brief Set integer global in lua scripts
 */
inline void set_lua_global(lua_State *L, const char *name, const int64_t val) {
  lua_pushinteger(L, val);
  lua_setglobal(L, name);
}
/**
 * \brief Set float global in lua scripts
 */
inline void set_lua_global(lua_State *L, const char *name, const float val) {
  lua_pushnumber(L, val);
  lua_setglobal(L, name);
}
/**
 * \brief Set boolean global in lua scripts
 */
inline void set_lua_global(lua_State *L, const char *name, const bool val) {
  lua_pushboolean(L, val);
  lua_setglobal(L, name);
}
/**
 * \brief Set string global in lua scripts
 */
inline void set_lua_global(lua_State *L, const char *name, const char *val) {
  lua_pushstring(L, val);
  lua_setglobal(L, name);
}

/**
 * \brief Get lua object (table) from top of stack and return pointer
 * \return Lua reference pointer
 */
int get_lua_object(lua_State *L);

/**
 * \brief Add simple c/c++ functions to lua scripts
 */
void add_func_to_scripts(lua_State *L, lua_CFunction func, const char *name);

/**
 * \brief Add glm::vec3 to stack
 */
void push_vec3_to_lua(lua_State *L, const float x, const float y,
                      const float z);

/**
 * \brief Add glm::vec4 to stack
 */
void push_vec4_to_lua(lua_State *L, const float x, const float y, const float z,
                      const float w);

/**
 * \brief Add glm::ivec3 to stack
 */
void push_ivec3_to_lua(lua_State *L, const int64_t x, const int64_t y,
                       const int64_t z);

/**
 * \brief Add glm::ivec3 to stack
 */
void push_ivec4_to_lua(lua_State *L, const int64_t x, const int64_t y,
                       const int64_t z, const int64_t w);

/**
 * \brief Read in floats from lua
 */
template <typename T>
void read_buffer_from_lua(lua_State *L, dd_array<T> &buffer) {
  return;
}

template <>
void read_buffer_from_lua<float>(lua_State *L, dd_array<float> &buffer);

template <>
void read_buffer_from_lua<int64_t>(lua_State *L, dd_array<int64_t> &buffer);
