#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddImport_SkinnedData.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

/**
 * \brief "New" function for ddAnimClip
 */
static int new_ddAnimClip(lua_State *L) {
  int num_args = lua_gettop(L);
  if (num_args != 2) {
    ddTerminal::post("[error]ddAnimClip::Must provide id and dda file");

    lua_pushboolean(L, false);
    return 1;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post("[error]ddAnimClip::Invalid 1st arg (id : string)");
    lua_pushboolean(L, false);
    return 1;
  }
  const char *id = lua_tostring(L, curr_arg);

  // get dda file
  curr_arg++;
  id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post("[error]ddAnimState::Invalid 2nd arg (dda file : string)");
    lua_pushboolean(L, false);
    return 1;
  }
  const char *dda_file = lua_tostring(L, curr_arg);

  // check if already exists (locked to ddAgent)
  ddAnimClip *a_clip = find_ddAnimClip(getCharHash(id));
  if (a_clip) {
    ddTerminal::post("[error]ddAnimClip::Returning already allocated clip");
    lua_pushboolean(L, true);
    return 1;
  }

  // create new clip
  a_clip = load_animation(dda_file, id);
  if (a_clip) {
    ddTerminal::f_post("ddAnimClip:: %llu :: %.3fs :: %u(frames) :: %.3f(fps)",
                       (long long unsigned)a_clip->id, a_clip->length,
                       a_clip->num_frames, a_clip->fps);
    lua_pushboolean(L, true);
    return 1;
  }

  lua_pushboolean(L, false);
  return 1;
}

// Animation library
static const struct luaL_Reg anim_lib[] = {{"load_clip", new_ddAnimClip},
                                           {NULL, NULL}};

int luaopen_Animation(lua_State *L) {
  // library functions
  luaL_newlib(L, anim_lib);
  return 1;
}