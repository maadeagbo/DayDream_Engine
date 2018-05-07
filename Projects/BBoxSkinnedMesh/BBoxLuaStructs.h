#include "LuaHooks.h"
#include "ddIncludes.h"

/// \brief level control flags
enum class LvlCtrlEnums { TRANSLATE, ROTATE, SCALE, X, Y, Z, NUM_FLAGS };

/// \brief level controls
struct LvlCtrl {
  bool flags[(unsigned)LvlCtrlEnums::NUM_FLAGS];
  cbuff<32> output = "default";
};

/** \brief Transforms */
struct BBTransform {
  glm::vec3 pos = glm::vec3(0.f);
  glm::vec3 scale = glm::vec3(1.f);
  glm::vec3 rot = glm::vec3(0.f);
  glm::uvec3 mirror = glm::uvec3(0.f);
  glm::ivec2 joint_ids = glm::ivec2(-1);
};

std::map<unsigned, BBTransform> &get_all_transforms();

int luaopen_bbox(lua_State *L);

int luaopen_lvlctrl(lua_State *L);

// render controls && skeleton viewer
int render_sk_controls(lua_State *L);
