#pragma once

#include "LuaHooks.h"
#include "ddIncludes.h"

enum LightType { DIRECTION_L, POINT_L, SPOT_L };

struct ddLBulb {
  /**
   * \brief Engine identifier assigned at initialization
   */
  size_t id = 0;
  /**
   * \brief Parent index
   */
  size_t parent_id;
  /**
  * \brief Parent index
  */
  bool parent_set = false;
  /**
   * \brief Sets shader lighting equation
   */
  LightType type = LightType::DIRECTION_L;
  /**
   * \brief Normalized vector for directional and spot lights
   */
  glm::vec3 direction = glm::vec3(0.3f, -1.f, -0.3f);
  /**
   * \brief Location of light
   */
  glm::vec3 position = glm::vec3(0.f, 10.f, 0.f);
  /**
   * \brief Color of light
   */
  glm::vec3 color = glm::vec3(1.0f);
  /**
* \brief Light space matrix (set per frame if light produces shadows)
*/
  glm::mat4 l_s_m;
  /**
   * \brief Linear falloff compnent
   */
  float linear = 2.f;
  /**
   * \brief Quadratic falloff component
   */
  float quadratic = 1.f;
  /**
   * \brief Incedence angle cutoff spot light
   */
  float cutoff_i = glm::cos(glm::radians(9.5f));
  /**
   * \brief Out angle cutoff spot light
   */
  float cutoff_o = glm::cos(glm::radians(20.5f));
  /**
   * \brief Spot light exponent
   */
  float spot_exp = 40.0f;
  /**
   * \brief Marks light as active
   */
  bool active = false;
  /**
   * \brief Mark light for shadow calculation
   */
  bool shadow = false;
};

/** \brief Lua class instance metatable name */
const char *ddLBulb_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddLBulb(lua_State *L);
