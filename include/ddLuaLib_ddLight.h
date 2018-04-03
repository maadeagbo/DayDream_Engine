#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

const unsigned ddLBulb_ptr_size = sizeof(ddLBulb *);

/**
 * \brief "New" function for ddLBulb
 */
static int new_ddLBulb(lua_State *L) {
  // create userdata for instance
  ddLBulb **blb = (ddLBulb **)lua_newuserdata(L, ddLBulb_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args == 0) {
    ddTerminal::post("[error]ddLBulb::Must provide id at initialization");
    return 1;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post("[error]ddLBulb::Invalid 1st arg (id : string)");
    return 1;
  }
  const char *id = lua_tostring(L, curr_arg);

  // check if already exists
  (*blb) = find_ddLBulb(getCharHash(id));
  if (*blb) {
    ddTerminal::post("[error]ddLBulb::Returning already allocated material");
    return 1;
  }

  // create new
  (*blb) = spawn_ddLBulb(getCharHash(id));
  if (!(*blb)) {
    ddTerminal::post("[error]ddLBulb::Failed to allocate new material");
    return 1;
  }

  // set metatable
  luaL_getmetatable(L, ddLBulb_meta_name());
  lua_setmetatable(L, -2);

  return 1;
}

/**
 * \brief garbage collect ddLBulb
 */
static int ddLBulb_gc(lua_State *L) {
  ddLBulb *blb = *(ddLBulb **)lua_touserdata(L, 1);
  if (blb) {
    destroy_ddLBulb(blb->id);
  }

  return 0;
}

// ddLBulb library
static const struct luaL_Reg blb_m2[] = {{"__gc", ddLBulb_gc}, {NULL, NULL}};
static const struct luaL_Reg blb_lib[] = {{"new", new_ddLBulb}, {NULL, NULL}};

int luaopen_ddLBulb(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddLBulb(L);
  luaL_setfuncs(L, blb_m2, 0);

  // library functions
  luaL_newlib(L, blb_lib);
  return 1;
}
