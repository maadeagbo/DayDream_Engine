// level script for BBoxSkinnedMesh cpp implementations
#include "ddLevelPrototype.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

#include "BBoxGraphics.h"
#include "BBoxLuaStructs.h"

//*****************************************************************************
//*****************************************************************************

// log lua function that can be called in scripts thru this function
void BBoxSkinnedMesh_func_register(lua_State *L);

// initialize gpu stuff ( run once during level update )
int init_gpu_stuff(lua_State *L);

// Proxy struct that enables reflection
struct BBoxSkinnedMesh_reflect : public ddLvlPrototype {
  BBoxSkinnedMesh_reflect() {
    add_lua_function("BBoxSkinnedMesh", BBoxSkinnedMesh_func_register);

    generate_grid();

    // set up reference bounding box
    BoundingBox ref_bbox = BoundingBox(glm::vec3(-0.5), glm::vec3(0.5));
    fill_buffer(ref_bbox);
  }
};

void BBoxSkinnedMesh_func_register(lua_State *L) {
  // log functions using register_callback_lua
	register_callback_lua(L, "init_gpu_stuff", init_gpu_stuff);
	register_callback_lua(L, "check_controls", render_sk_controls);

  // log bounding box libraries
  luaL_requiref(L, "ddBBox", luaopen_bbox, 1);
  luaL_requiref(L, "Lvlctrl", luaopen_lvlctrl, 1);

  // clear stack
  int top = lua_gettop(L);
  lua_pop(L, top);
}

// log reflection
BBoxSkinnedMesh_reflect BBoxSkinnedMesh_proxy;
