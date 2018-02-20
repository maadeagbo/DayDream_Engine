#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

const unsigned ddMat_ptr_size = sizeof(ddMat *);

/**
 * \brief "New" function for ddMat
 */
static int new_ddMat(lua_State *L) {
  // create userdata for instance
  ddMat **mat = (ddMat **)lua_newuserdata(L, ddMat_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args == 0) {
    ddTerminal::post("[error]ddMat::Must provide id at initialization");
		return 1;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post("[error]ddMat::Invalid 1st arg (id : string)");
		return 1;
  }
  const char *id = lua_tostring(L, curr_arg);

  // check if already exists
  (*mat) = find_ddMat(getCharHash(id));
  if (*mat) {
    ddTerminal::post("[error]ddMat::Returning already allocated material");
    return 1;
  }

  // create new
  (*mat) = spawn_ddMat(getCharHash(id));
  if (!(*mat)) {
    ddTerminal::post("[error]ddMat::Failed to allocate new material");
    return 1;
  }

  // set metatable
  luaL_getmetatable(L, ddMat_meta_name());
  lua_setmetatable(L, -2);

  return 1;
}

/**
 * \brief garbage collect ddMat
 */
static int ddMat_gc(lua_State *L) {
  ddMat *mat = *(ddMat **)lua_touserdata(L, 1);
  if (mat) {
    destroy_ddMat(mat->id);
  }

  return 0;
}

// ddMat library
static const struct luaL_Reg mat_m2[] = {{"__gc", ddMat_gc}, {NULL, NULL}};
static const struct luaL_Reg mat_lib[] = {{"new", new_ddMat}, {NULL, NULL}};

int luaopen_ddMat(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddMat(L);
  luaL_setfuncs(L, mat_m2, 0);

  // library functions
  luaL_newlib(L, mat_lib);
  return 1;
}
