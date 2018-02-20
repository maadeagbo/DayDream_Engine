#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

const unsigned ddcam_ptr_size = sizeof(ddCam *);

/** \brief "New" function for ddCam */
static int new_ddCam(lua_State *L) {
  // create userdata for instance
  ddCam **cam = (ddCam **)lua_newuserdata(L, ddcam_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args != 3) {
    ddTerminal::post("[error]ddCam::Must provide id and parent agent id");
		return 1;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  // luaL_argcheck(L, id_flag, 2, "ddCam::Must give camera id");
  if (!id_flag) {
    ddTerminal::post("[error]ddCam::Invalid 1st arg (id : string)");
		return 1;
  }
  const char *id = lua_tostring(L, curr_arg);

  // assign parent id
  curr_arg++;
  size_t p_id = 0;
  bool p_flag = lua_isinteger(L, curr_arg);
  if (!p_flag) {
    ddTerminal::post("[error]ddCam::Invalid 2nd arg (parent id : int)");
		return 1;
  } else {
    p_id = (size_t)lua_tointeger(L, curr_arg);
    ddAgent *ag = find_ddAgent(p_id);
    if (!ag) {
      ddTerminal::post("[error]ddCam::Agent id does not exist");
			return 1;
    }
  }

  (*cam) = spawn_ddCam(getCharHash(id));
  if (!(*cam)) {
    ddTerminal::post("[error]ddCam::Failed to allocate new camera");
    return 1;
  }
  // initialize
  glm::uvec2 scr_dim = ddSceneManager::get_screen_dimensions();
  (*cam)->width = scr_dim.x;
  (*cam)->height = scr_dim.y;
  (*cam)->fovh = glm::radians(60.f);
  (*cam)->n_plane = 0.1f;
  (*cam)->f_plane = 50.f;
  (*cam)->parent = p_id;

  // set metatable
  luaL_getmetatable(L, ddCam_meta_name());
  lua_setmetatable(L, -2);

  return 1; /* light user data on stack */
}

/** \brief Camera direction */
static int get_cam_dir(lua_State *L) {
  ddCam *cam = *(ddCam **)lua_touserdata(L, 1);
  if (cam) {
    ddAgent *ag = find_ddAgent(cam->parent);
    if (ag) {
      glm::vec3 d = ddSceneManager::cam_forward_dir(cam, &ag->body);
      push_vec3_to_lua(L, d.x, d.y, d.z);
			return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

/** \brief garbage collect ddCam */
static int ddCam_gc(lua_State *L) {
  // ddCam** cam = (ddCam **)luaL_checkudata(L, 1, ddCam_meta_name());
  ddCam *cam = *(ddCam **)lua_touserdata(L, 1);
  if (cam) {
    destroy_ddCam(cam->id);
  }

  return 0;
}

// ddCam library
static const struct luaL_Reg cam_m2[] = {
    {"__gc", ddCam_gc}, {"dir", get_cam_dir}, {NULL, NULL}};
static const struct luaL_Reg cam_lib[] = {{"new", new_ddCam}, {NULL, NULL}};

int luaopen_ddCam(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddCam(L);
  luaL_setfuncs(L, cam_m2, 0);

  // library functions
  luaL_newlib(L, cam_lib);
  return 1;
}
