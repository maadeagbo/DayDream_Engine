#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddImport_Model.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

const unsigned ddModelData_ptr_size = sizeof(ddModelData *);

/**
 * \brief "New" function for ddModelData
 */
static int new_ddModelData(lua_State *L) {
  // create userdata for instance
  ddModelData **mdl = (ddModelData **)lua_newuserdata(L, ddModelData_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args == 0) {
    ddTerminal::post(
        "[error]ddModelData::Must provide id and ddm file at initialization");
    return 1;
  }

  // get path
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post("[error]ddModelData::Invalid 1st arg (ddm file : string)");
    return 1;
  }
  const char *ddm_file = lua_tostring(L, curr_arg);

  // create new model data
  (*mdl) = load_ddm(ddm_file);
  if (!(*mdl)) {
    ddTerminal::post("[error]ddModelData::Failed to allocate new model");
    return 1;
  }
	// skip loading if using opengl api and on separate thread
	bool skip = DD_GRAPHICS_API == 0 && ddAssets::load_screen_check();
	if (!skip) {
		// load to gpu
		ddAssets::load_model_to_gpu(*mdl);
	}

  // set metatable
  luaL_getmetatable(L, ddModelData_meta_name());
  lua_setmetatable(L, -2);

  return 1;
}

/**
 * \brief garbage collect ddModelData
 */
static int ddModelData_gc(lua_State *L) {
  ddModelData *mdl = *(ddModelData **)lua_touserdata(L, 1);
  if (mdl && mdl->buffers.size() > 0) {
    // deallocate buffers on GPU then delete
    DD_FOREACH(ddMeshBufferData *, buffer, mdl->buffers) {
      ddGPUFrontEnd::destroy_buffer_data(*buffer.ptr);
    }
    mdl->mesh_info.resize(0);
    destroy_ddModelData(mdl->id);
  }

  return 0;
}

// ddModelData library
static const struct luaL_Reg mdl_m2[] = {{"__gc", ddModelData_gc},
                                         {NULL, NULL}};
static const struct luaL_Reg mdl_lib[] = {{"new", new_ddModelData},
                                          {NULL, NULL}};

int luaopen_ddModelData(lua_State *L) {
	// get and log functions in metatable
	log_meta_ddModelData(L);
	luaL_setfuncs(L, mdl_m2, 0);

	// library functions
	luaL_newlib(L, mdl_lib);
	return 1;
}
