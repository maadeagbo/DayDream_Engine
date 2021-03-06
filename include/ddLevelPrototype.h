#include "LuaHooks.h"
#include "ddIncludes.h"

/** \brief Prototype class to add lua functions to level scripts */
struct ddLvlPrototype {
  /** \brief Adds functions to reflection container */
  void add_lua_function(const char *key, std::function<void(lua_State *)> func);
};

/** \brief Retrieve reflection container */
const std::map<string64, std::function<void(lua_State *)>> &get_reflections();
