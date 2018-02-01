#include "LuaHooks.h"

namespace {
cbuff<64> _key;
cbuff<256> _val;
}  // namespace

// ********************************************************************
// Template definitions
// ********************************************************************

template <>
bool add_arg_LEvent<bool>(DD_LEvent *levent, const char *key, bool arg) {
  if (!levent || levent->active >= MAX_EVENT_ARGS) {
    return false;
  }
  levent->args[levent->active].key = key;
  levent->args[levent->active].val.type = VType::BOOL;
  levent->args[levent->active].val.v_bool = arg;
  levent->active += 1;
  return true;
}

template <>
bool add_arg_LEvent<int64_t>(DD_LEvent *levent, const char *key, int64_t arg) {
  if (!levent || levent->active >= MAX_EVENT_ARGS) {
    return false;
  }
  levent->args[levent->active].key = key;
  levent->args[levent->active].val.type = VType::INT;
  levent->args[levent->active].val.v_int = arg;
  levent->active += 1;
  return true;
}

template <>
bool add_arg_LEvent<float>(DD_LEvent *levent, const char *key, float arg) {
  if (!levent || levent->active >= MAX_EVENT_ARGS) {
    return false;
  }
  levent->args[levent->active].key = key;
  levent->args[levent->active].val.type = VType::FLOAT;
  levent->args[levent->active].val.v_float = arg;
  levent->active += 1;
  return true;
}

template <>
bool add_arg_LEvent<const char *>(DD_LEvent *levent, const char *key,
                                  const char *arg) {
  if (!levent || levent->active >= MAX_EVENT_ARGS) {
    return false;
  }
  levent->args[levent->active].key = key;
  levent->args[levent->active].val.type = VType::STRING;
  levent->args[levent->active].val.v_strptr = arg;
  levent->active += 1;
  return true;
}

template <>
int64_t *get_arg_LEvent<int64_t>(DD_LEvent *levent, const char *key) {
  if (!levent) {
    return nullptr;
  }

  Varying<32> *v = nullptr;
  cbuff<32> *k = nullptr;
  for (unsigned i = 0; i < levent->active; i++) {
    k = &levent->args[i].key;
    v = &levent->args[i].val;
    if (k->compare(key) == 0 && v->type == VType::INT) {
      return &v->v_int;
    }
  }
  return nullptr;
}

template <>
float *get_arg_LEvent<float>(DD_LEvent *levent, const char *key) {
  if (!levent) {
    return nullptr;
  }

  Varying<32> *v = nullptr;
  cbuff<32> *k = nullptr;
  for (unsigned i = 0; i < levent->active; i++) {
    k = &levent->args[i].key;
    v = &levent->args[i].val;
    if (k->compare(key) == 0 && v->type == VType::FLOAT) {
      return &v->v_float;
    }
  }
  return nullptr;
}

template <>
bool *get_arg_LEvent<bool>(DD_LEvent *levent, const char *key) {
  if (!levent) {
    return nullptr;
  }

  Varying<32> *v = nullptr;
  cbuff<32> *k = nullptr;
  for (unsigned i = 0; i < levent->active; i++) {
    k = &levent->args[i].key;
    v = &levent->args[i].val;
    if (k->compare(key) == 0 && v->type == VType::BOOL) {
      return &v->v_bool;
    }
  }
  return nullptr;
}

template <>
const char *get_arg_LEvent<const char>(DD_LEvent *levent, const char *key) {
  if (!levent) {
    return nullptr;
  }

  Varying<32> *v = nullptr;
  cbuff<32> *k = nullptr;
  for (unsigned i = 0; i < levent->active; i++) {
    k = &levent->args[i].key;
    v = &levent->args[i].val;
    if (k->compare(key) == 0 && v->type == VType::STRING) {
      return v->v_strptr.str();
    }
  }
  return nullptr;
}

template <>
int64_t *DD_FuncBuff::get_func_val<int64_t>(const char *ckey) {
  for (unsigned i = 0; i < num_args; i++) {
    bool param_check = buffer[i].arg_name.compare(ckey) == 0;
    if (param_check && buffer[i].arg.type == VType::INT) {
      return &buffer[i].arg.v_int;
    }
  }
  return nullptr;
}

template <>
float *DD_FuncBuff::get_func_val<float>(const char *ckey) {
  for (unsigned i = 0; i < num_args; i++) {
    bool param_check = buffer[i].arg_name.compare(ckey) == 0;
    if (param_check && buffer[i].arg.type == VType::FLOAT) {
      return &buffer[i].arg.v_float;
    }
  }
  return nullptr;
}

template <>
bool *DD_FuncBuff::get_func_val<bool>(const char *ckey) {
  for (unsigned i = 0; i < num_args; i++) {
    bool param_check = buffer[i].arg_name.compare(ckey) == 0;
    if (param_check && buffer[i].arg.type == VType::BOOL) {
      return &buffer[i].arg.v_bool;
    }
  }
  return nullptr;
}

template <>
const char *DD_FuncBuff::get_func_val<const char>(const char *ckey) {
  for (unsigned i = 0; i < num_args; i++) {
    bool param_check = buffer[i].arg_name.compare(ckey) == 0;
    if (param_check && buffer[i].arg.type == VType::STRING) {
      return buffer[i].arg.v_strptr.str();
    }
  }
  return nullptr;
}

// ********************************************************************
// ********************************************************************

bool check_stack_nil(lua_State *L, int idx) {
  if (lua_type(L, idx) == LUA_TNIL) {
    return true;
  }
  return false;
}

lua_State *init_lua_state() {
  lua_State *L = luaL_newstate();  // opens lua
  if (L) {
    luaL_openlibs(L);  // opens standard libraries
    cbuff<256> s_dir;
    s_dir.format("%s/", ROOT_DIR);
    append_package_path(L, s_dir.str());  // add root path to scripts

    // Add global variables for use in scripts
    set_lua_global(L, "ROOT_DIR", ROOT_DIR);
    s_dir.format("%s/%s", RESOURCE_DIR, "scripts");
    set_lua_global(L, "SCRIPTS_DIR", s_dir.str());
    s_dir.format("%s/%s", ROOT_DIR, "Projects");
    set_lua_global(L, "PROJECTS_DIR", s_dir.str());

    s_dir.format("%s/", RESOURCE_DIR);
    append_package_path(L, s_dir.str());

#ifdef _WIN32
    set_lua_global(L, "WIN32", true);
    set_lua_global(L, "LINUX", false);
#elif __linux__
    set_lua_global(L, "WIN32", false);
    set_lua_global(L, "LINUX", true);
#endif
  }
  return L;
}

bool parse_luafile(lua_State *L, const char *filename) {
  int err_num = 0;
  /// \brief quick print out error func
  auto handle_error = [&]() {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  };

  cbuff<256> file = filename;
  if (!file.contains(".lua")) {
    printf("Invalid file type <%s>\n", file.str());
  }

  // read file then execute
  if ((err_num = luaL_loadfile(L, filename)) != 0) {
    handle_error();
    return false;
  }
  if ((err_num = lua_pcall(L, 0, 0, 0)) != 0) {
    handle_error();
    // return false; // continue executing file thru errors
  }
  return true;
}

void push_args(lua_State *L, const DD_LEvent &levent, const int idx) {
  // assumes new table is on top of stack before call
  const int _idx = idx - 1;

  luaL_checkstack(L, 2, "too many arguments");
  switch (levent.args[_idx].val.type) {
    case VType::BOOL:
      lua_pushinteger(L, levent.args[_idx].val.v_bool ? 1 : 0);
      lua_setfield(L, -2, levent.args[_idx].key.str());
      break;
    case VType::STRING:
      lua_pushstring(L, levent.args[_idx].val.v_strptr.str());
      lua_setfield(L, -2, levent.args[_idx].key.str());
      break;
    case VType::FLOAT:
      lua_pushnumber(L, levent.args[_idx].val.v_float);
      lua_setfield(L, -2, levent.args[_idx].key.str());
      break;
    case VType::INT:
      lua_pushinteger(L, levent.args[_idx].val.v_int);
      lua_setfield(L, -2, levent.args[_idx].key.str());
      break;
    default:
      break;  // set nothing
  }
}

DD_LFuncArg *DD_FuncBuff::get_next_arg() {
  if (num_args == MAX_ARG_BUFFER_SIZE) {
    return nullptr;
  }
  unsigned idx = num_args++;
  return &buffer[idx];
}

void register_callback_lua(lua_State *L, const char *func_sig,
                           lua_CFunction _func) {
  lua_register(L, func_sig, _func);
}

void callback_lua(lua_State *L, const DD_LEvent levent, int func_ref,
                  DD_FuncBuff &fb, int global_ref) {
  /// \brief print out error func
  int err_num = 0;
  auto handle_error = [&]() {
    fprintf(stderr, "callback_lua::%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  };

  // retrieve function
  if (global_ref > 0) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
    if (check_stack_nil(L, -1)) {
      printf("<%d> class function doesn't exist.\n", func_ref);
      return;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, global_ref);
    if (check_stack_nil(L, -1)) {
      printf("<%d> global doesn't exist.\n", global_ref);
      return;
    }
  } else {  // find global function
    lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
    if (check_stack_nil(L, -1)) {
      printf("<%d> global function doesn't exist.\n", func_ref);
      return;
    }
  }

  // push event arguments (in the form of a table)
  lua_pushstring(L, levent.handle.str());  // push event handle
  lua_newtable(L);  // create new table and put on top of stack
  if (levent.active > 0) {
    for (unsigned i = 0; i < levent.active; i++) {
      push_args(L, levent, i + 1);  // push arguments
    }
  }
  lua_pushinteger(L, (int64_t)levent.active);  // push # of arguments

  // debug
  // stack_dump(L);

  // call function
  int num_args = global_ref > 0 ? 4 : 3;
  if ((err_num = lua_pcall(L, num_args, LUA_MULTRET, 0)) != 0) {
    handle_error();
  }

  // get returned events and fill buffer
  parse_lua_events(L, fb);
}

void stack_dump(lua_State *L) {
  printf("Stack trace:\t");
  int top = lua_gettop(L);         /* depth of the stack */
  for (int i = 1; i <= top; i++) { /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {
      case LUA_TSTRING: /* strings */
        printf("'%s'", lua_tostring(L, i));
        break;
      case LUA_TBOOLEAN: /* Booleans */
        printf(lua_toboolean(L, i) ? "true" : "false");
        break;
      case LUA_TNUMBER:          /* numbers */
        if (lua_isinteger(L, i)) /* integer? */
          printf("%lld", lua_tointeger(L, i));
        else /* float */
          printf("%g", lua_tonumber(L, i));
        break;
      default: /* other values */
        printf("%s", lua_typename(L, t));
        break;
    }
    printf("  "); /* put a separator */
  }
  printf("\n"); /* end the listing */
}

void parse_lua_events(lua_State *L, DD_FuncBuff &fb) {
  fb.num_args = 0;
  // events must be in the form of a table
  int top = lua_gettop(L); /* number of events */
                           // printf("# of returns: %d\n", top);
  for (int i = 1; i <= top; i++) {
    int t = lua_type(L, i);
    switch (t) {
      case LUA_TTABLE: {
        parse_table(L, &fb);
        lua_pop(L, 1);  // Pop table
        break;
      }
      default:
        printf("%s\n", lua_typename(L, t));
        lua_pop(L, 1);  // Pop
        break;
    }
  }
}

void parse_table(lua_State *L, DD_LEvent *levent, const int tabs) {
  lua_pushnil(L);                 // push key on stack for table access
  while (lua_next(L, -2) != 0) {  // adds value to the top of the stack
    int t = lua_type(L, -1);      // get value type
    switch (t) {
      case LUA_TSTRING: {
        _val.format("%s", lua_tostring(L, -1));
        lua_pushvalue(L, -2);  // copy the key
        _key.format("%s", lua_tostring(L, -1));
        lua_pop(L, 1);  // remove copy key

        if (_key.compare("event_id") == 0) {
          levent->handle = _val.str();
        } else {
          bool arg_set = add_arg_LEvent(levent, _key.str(), _val.str());
          if (!arg_set) {
            printf("A\n");
            printf("No more arg slots available\n");
          }
        }
        break;
      }
      case LUA_TNUMBER: {
        if (lua_isinteger(L, -1)) {
          int64_t val = (int64_t)lua_tointeger(L, -1);
          lua_pushvalue(L, -2);  // copy the key
          _key.format("%s", lua_tostring(L, -1));
          lua_pop(L, 1);  // remove copy key

          bool arg_set = add_arg_LEvent(levent, _key.str(), val);
          if (!arg_set) {
            printf("C\n");
            printf("No more args available\n");
          }
        } else {
          float val = (float)lua_tonumber(L, -1);
          lua_pushvalue(L, -2);  // copy the key
          _key.format("%s", lua_tostring(L, -1));
          lua_pop(L, 1);  // remove copy key

          bool arg_set = add_arg_LEvent(levent, _key.str(), val);
          if (!arg_set) {
            printf("D\n");
            printf("No more args available\n");
          }
        }
        break;
      }
      case LUA_TBOOLEAN: {
        bool lbool = lua_toboolean(L, -1);
        lua_pushvalue(L, -2);  // copy the key
        _key.format("%s", lua_tostring(L, -1));
        lua_pop(L, 1);  // remove copy key

        bool arg_set = add_arg_LEvent(levent, _key.str(), lbool);
        if (!arg_set) {
          printf("B\n");
          printf("No more args available\n");
        }

        break;
      }
      case LUA_TTABLE: {
        parse_table(L, levent, tabs + 1);
        break;
      }
      default:
        break;
    }
    lua_pop(L, 1);  // remove value
  }
}

void parse_table(lua_State *L, DD_FuncBuff *fb, const int tabs) {
  lua_pushnil(L);                 // push key on stack for table access
  while (lua_next(L, -2) != 0) {  // adds value to the top of the stack
    DD_LFuncArg *f_arg = fb->get_next_arg();
    int t = lua_type(L, -1);  // get value type
    switch (t) {
      case LUA_TSTRING: {
        _val.format("%s", lua_tostring(L, -1));
        lua_pushvalue(L, -2);  // copy the key
        _key.format("%s", lua_tostring(L, -1));
        lua_pop(L, 1);  // remove copy key

        if (f_arg) {
          f_arg->arg_name = _key.str();
          f_arg->arg.type = VType::STRING;
          f_arg->arg.v_strptr = _val.str();
        }
        break;
      }
      case LUA_TBOOLEAN: {
        bool lbool = lua_toboolean(L, -1);
        lua_pushvalue(L, -2);  // copy the key
        _key.format("%s", lua_tostring(L, -1));
        lua_pop(L, 1);  // remove copy key

        if (f_arg) {
          f_arg->arg_name = _key.str();
          f_arg->arg.type = VType::BOOL;
          f_arg->arg.v_bool = lbool;
        }
        break;
      }
      case LUA_TNUMBER: {
        if (lua_isinteger(L, -1)) {
          int64_t val = (int64_t)lua_tointeger(L, -1);
          lua_pushvalue(L, -2);  // copy the key
          _key.format("%s", lua_tostring(L, -1));
          lua_pop(L, 1);  // remove copy key

          if (f_arg) {
            f_arg->arg_name = _key.str();
            f_arg->arg.type = VType::INT;
            f_arg->arg.v_int = val;
          }
        } else {
          float val = (float)lua_tonumber(L, -1);
          lua_pushvalue(L, -2);  // copy the key
          _key.format("%s", lua_tostring(L, -1));
          lua_pop(L, 1);  // remove copy key

          if (f_arg) {
            f_arg->arg_name = _key.str();
            f_arg->arg.type = VType::FLOAT;
            f_arg->arg.v_float = val;
          }
        }
        break;
      }
      default:
        break;
    }
    lua_pop(L, 1);  // remove value
  }
}

void print_table(lua_State *L, const int tabs) {
  // assume table is already on stack
  lua_pushnil(L);                 // push key on stack for table access
  while (lua_next(L, -2) != 0) {  // adds value to the top of the stack
    // copy the key so that lua_tostring does not modify the original
    lua_pushvalue(L, -2);
    /* uses 'key' (at index -1) and 'value' (at index -2) */
    if (lua_isboolean(L, -2)) {  // boolean
      bool lbool = lua_toboolean(L, -2);
      printf("key(bool) : %s\n", lua_tostring(L, -1));
      printf("\t%s \n", lbool ? "t" : "f");
      lua_pop(L, 1);  // remove copy key
    } else if (lua_isnumber(L, -2)) {
      if (lua_isinteger(L, -2)) {  // integer
        int64_t val = (int64_t)lua_tointeger(L, -2);
        printf("key(int) : %s\n", lua_tostring(L, -1));
        printf("\t%ld \n", val);
        lua_pop(L, 1);  // remove copy key
      } else {          // float
        float val = (float)lua_tonumber(L, -2);
        printf("key(float) : %s\n", lua_tostring(L, -1));
        printf("\t%.4f \n", val);
        lua_pop(L, 1);  // remove copy key
      }
    } else if (lua_isstring(L, -2)) {  // string
      _key.format("%s", lua_tostring(L, -2));
      printf("key(str) : %s\n", lua_tostring(L, -1));
      printf("\t%s \n", _key.str());
      lua_pop(L, 1);  // remove copy key
    } else if (lua_istable(L, -2)) {
      printf("--key (array - %d) : %s\n", tabs, lua_tostring(L, -1));
      lua_pop(L, 1);  // remove copy key
      // printf("-->%d\t", tabs); stack_dump(L);	// check
      // entrance
      print_table(L, tabs + 1);
      printf("--array - %d : done\n", tabs);
    }
    lua_pop(L, 1);  // remove value
  }
  // printf("<--%d\t", tabs); stack_dump(L);			// check exit
}

void print_buffer(DD_FuncBuff &fb) {
  for (unsigned i = 0; i < fb.num_args; i++) {
    cbuff<32> &k = fb.buffer[i].arg_name;
    Varying<256> &v = fb.buffer[i].arg;
    switch (v.type) {
      case VType::BOOL:
        printf("\t%s : %s\n", k.str(), v.v_bool ? "true" : "false");
        break;
      case VType::FLOAT:
        printf("\t%s : %.3f\n", k.str(), v.v_float);
        break;
      case VType::INT:
        printf("\t%s : %ld\n", k.str(), v.v_int);
        break;
      case VType::STRING:
        printf("\t%s : %s\n", k.str(), v.v_strptr.str());
        break;
      default:
        break;
    }
  }
}

int get_lua_ref(lua_State *L, const char *lclass, const char *func) {
  bool lclass_flag = lclass && *lclass;

  if (lclass_flag) {  // find class function
    lua_getglobal(L, lclass);
    if (check_stack_nil(L, -1)) {
      printf("<%s> global doesn't exist.\n", lclass);
    }

    lua_getfield(L, -1, func);
    if (check_stack_nil(L, -1)) {
      printf("<%s> : <%s> function doesn't exist.\n", lclass, func);
    } else {
      // remove global table from stack once class function found
      lua_rotate(L, 1, -1);
      lua_pop(L, 1);
    }
  } else {  // find global
    lua_getglobal(L, func);
    if (check_stack_nil(L, -1)) {
      printf("<%s> global function/class doesn't exist.\n", func);
    }
  }

  int t = lua_type(L, -1);
  if (t == LUA_TFUNCTION || t == LUA_TTABLE) {
    return luaL_ref(L, LUA_REGISTRYINDEX);  // store reference
  } else {
    lua_pop(L, 1);  // remove nil from stack
  }
  return LUA_REFNIL;
}

int get_lua_ref(lua_State *L, int lclass, const char *func) {
  if (lclass == LUA_REFNIL) {  // invalid class object
    return LUA_REFNIL;
  }
  // add class object to the stack
  lua_rawgeti(L, LUA_REGISTRYINDEX, lclass);
  if (!check_stack_nil(L, -1)) {  // make sure class object exists
                                  // get function
    lua_getfield(L, -1, func);
    if (!check_stack_nil(L, -1)) {
      // remove global table from stack once class function found
      lua_rotate(L, 1, -1);
      lua_pop(L, 1);
      // make sure value retrieved is a function
      int t = lua_type(L, -1);
      if (t == LUA_TFUNCTION) {
        return luaL_ref(L, LUA_REGISTRYINDEX);
      }
    }
  }
  return LUA_REFNIL;
}

void clear_lua_ref(lua_State *L, int func_ref) {
  luaL_unref(L, LUA_REGISTRYINDEX, func_ref);
}

int get_lua_object(lua_State *L) {
  if (lua_gettop(L) > 0) {          // stack non-empty
    if (!check_stack_nil(L, -1)) {  // non-nil object
      return luaL_ref(L, LUA_REGISTRYINDEX);
    }
  }
  return LUA_REFNIL;
}

void add_func_to_scripts(lua_State *L, lua_CFunction func, const char *name) {
  lua_pushcfunction(L, func);
  lua_setglobal(L, name);
}

void push_vec3_to_lua(lua_State *L, const float x, const float y,
                      const float z) {
  lua_newtable(L);  // create new table and put on top of stack

  luaL_checkstack(L, 2, "too many arguments");  // check stack size

  // set fields
  lua_pushnumber(L, x);
  lua_setfield(L, -2, "x");
  lua_pushnumber(L, y);
  lua_setfield(L, -2, "y");
  lua_pushnumber(L, z);
  lua_setfield(L, -2, "z");
}

void push_vec4_to_lua(lua_State *L, const float x, const float y, const float z,
                      const float w) {
  lua_newtable(L);  // create new table and put on top of stack

  luaL_checkstack(L, 2, "too many arguments");  // check stack size

  // set fields
  lua_pushnumber(L, x);
  lua_setfield(L, -2, "x");
  lua_pushnumber(L, y);
  lua_setfield(L, -2, "y");
  lua_pushnumber(L, z);
  lua_setfield(L, -2, "z");
  lua_pushnumber(L, w);
  lua_setfield(L, -2, "w");
}

void append_package_path(lua_State *L, const char *path) {
  cbuff<512> curr_path, new_path;
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  curr_path = lua_tostring(L, -1);  // grab path string from top of stack
  new_path.format(";%s/?.lua", path);

  lua_pop(L, 1);  // get rid of the old path string on the stack
  lua_pushstring(L, new_path.str());  // push the new one
  lua_setfield(L, -2, "path");        // set the field "path"
  lua_pop(L, 1);                      // pop package table
}
