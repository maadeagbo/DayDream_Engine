#include "ddLevelPrototype.h"

/** \brief Reflection container */
std::map<string64, std::function<void(lua_State *)>> reflect_map;

void ddLvlPrototype::add_lua_function(const char *key,
                                      std::function<void(lua_State *)> func) {
  string64 temp = key;
  reflect_map[temp] = func;
}

const std::map<string64, std::function<void(lua_State *)>> &get_reflections() {
  return reflect_map;
}
