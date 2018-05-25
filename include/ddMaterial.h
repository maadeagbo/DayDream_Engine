#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Material:
*		-
*
-----------------------------------------------------------------------------*/

#include "LuaHooks.h"
#include "ddIncludes.h"
#include "ddTexture2D.h"

enum MaterialType { MULTIPLIER_MAT, DEFAULT_MAT };

/// \brief Container for material information
struct ddMat {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Diffuse color
  glm::vec4 base_color;
  /// \brief Attached textures (indexed w/ TexType enum)
  dd_array<size_t> textures;
  /// \brief Specular component
  float spec_value = 0.0f;
  /// \brief Flag for activated textures (bit comparison w/ TexTypes)
  TexType texture_flag = TexType::NULL_T;
  /// \brief Flag that marks if the material is loaded on the gpu
  bool loaded_gpu = false;
  /// \brief Flag that marks if color can be modified thru instancing
  bool color_modifier = false;
};

/** \brief Lua class instance metatable name */
const char *ddMat_meta_name();
/** \brief DO NOT USE. ONLY TO BE CALLED BY ddAssetsLuaClass */
void log_meta_ddMat(lua_State *L);
