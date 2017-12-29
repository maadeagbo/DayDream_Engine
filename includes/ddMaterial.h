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

#include "ddIncludes.h"
#include "ddTexture2D.h"

enum MaterialType { MULTIPLIER_MAT, DEFAULT_MAT };

// class DD_Material {
//  public:
//   DD_Material(const char* id = "")
//       : m_base_color(glm::vec4(0.5f, 0.5f, 0.5f, 1.f)),
//         shininess(0.1f),
//         m_albedo(false),
//         m_specular(false),
//         m_normal(false),
//         m_roughness(false),
//         m_metalness(false),
//         m_emissive(false),
//         m_ambient(false),
//         m_loaded_to_GPU(false),
//         m_textures(TextureType::NUM_T_TYPES),
//         m_matType(MaterialType::DEFAULT_MAT) {
//     m_ID = id;
//   }
//   ~DD_Material() {}

//   void AddTexture(DD_Texture2D* tex, TextureType type);
//   void SetMultiplierMaterial(TextureType type);
//   bool OpenGLBindMaterial();
//   MaterialType GetMatType() { return this->m_matType; }

//   std::string m_ID;
//   glm::vec4 m_base_color;
//   float shininess;
//   // texture info for material
//   bool m_albedo, m_specular, m_normal, m_roughness, m_metalness, m_emissive,
//       m_ambient, m_loaded_to_GPU;
//   dd_array<DD_Texture2D*> m_textures;

//  private:
//   MaterialType m_matType;
// };

/// \brief Container for material information
struct ddMat {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Diffuse color
  glm::vec4 base_color;
  /// \brief Attached textures (engine ids)
  dd_array<size_t> textures;
  /// \brief Specular component
  float spec_value;
  /// \brief Flag for activated textures (bit comparison w/ TexTypes)
  unsigned texture_flag = 0;
  /// \brief Flag that marks if the material is loaded on the gpu
  bool loaded_gpu = false;
  /// \brief Flag that marks if color can be modified thru instancing
  bool color_modifier = false;
};