#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Light:
*		-
*
*	TODO:	== Add set parent method
-----------------------------------------------------------------------------*/

#include "LuaHooks.h"
#include "ddIncludes.h"

enum LightType { DIRECTION_L, POINT_L, SPOT_L };

struct DD_Light {
  std::string m_ID;
  LightType m_type;
  glm::vec3 m_direction;
  glm::vec3 _position;
  glm::vec3 m_color;
  float m_linear;
  float m_quadratic;
  float m_cutoff_i;
  float m_cutoff_o;
  float m_spotExp;
  glm::mat4 parent_transform;
  bool m_flagShadow;
  bool light_obj;

  DD_Light()
      : m_type(LightType::DIRECTION_L),
        m_direction(glm::vec3(-1.0f)),
        m_color(glm::vec3(1.0f)),
        m_linear(0.0007f),
        m_quadratic(0.00002f),
        m_cutoff_i(glm::cos(glm::radians(9.5f))),
        m_cutoff_o(glm::cos(glm::radians(20.5f))),
        m_spotExp(40.0f),
        m_flagShadow(false),
        light_obj(false),
        _flag_parent(false) {}

  DD_Light(const char* ID)
      : m_ID(ID),
        m_type(LightType::DIRECTION_L),
        m_direction(glm::vec3(-1.0f)),
        m_color(glm::vec3(1.0f)),
        m_linear(0.0007f),
        m_quadratic(0.00002f),
        m_cutoff_i(glm::cos(glm::radians(9.5f))),
        m_cutoff_o(glm::cos(glm::radians(20.5f))),
        m_spotExp(40.0f),
        m_flagShadow(false),
        light_obj(false),
        _flag_parent(false) {}

  inline void SetParent(const char* parentID) {
    _parent_id = parentID;
    _parent_idx = -1;
    _flag_parent = true;
  }
  inline void unParent() {
    _parent_id = "";
    _parent_idx = -1;
    _flag_parent = false;
  }
  inline void SetFrameLSM(const glm::mat4 matrix) { LSM = matrix; }
  inline void SetParentIndex(const int index) { _parent_idx = index; }
  inline int parentIndex() const { return _parent_idx; }
  inline std::string parentID() const { return _parent_id; }
  inline bool isChild() const { return _flag_parent; }
  inline glm::mat4 GetLSM() const { return LSM; }

 private:
  glm::mat4 LSM;
  std::string _parent_id;
  int _parent_idx;
  bool _flag_parent;
};

namespace LightSpace {
float CalculateLightVolumeRadius(const DD_Light* lght);
void PrintInfo(const DD_Light& lght);
}

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
const char* ddLBulb_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddLBulb(lua_State* L);
