#pragma once

/*
 * Copyright (c) 2016, Moses Adeagbo
 * All rights reserved.
 */

/*-----------------------------------------------------------------------------
*
*	DD_Texture2D:
*		- Ports 2D texture from file into OpenGL useable format
*
-----------------------------------------------------------------------------*/

#include "GPUFrontEnd.h"
#include "ddIncludes.h"

// enum TextureType {
//   ALBEDO,
//   SPECULAR,
//   NORMAL,
//   ROUGH,
//   METAL,
//   AO,
//   EMISSIVE,
//   NUM_T_TYPES
// };

enum class TexType : unsigned {
  ALBEDO = 0x1,
  SPEC = 0x2,
  NORMAL = 0x4,
  ROUGH = 0x8,
  METAL = 0x10,
  EMISSIVE = 0x20,
  AMBIENT = 0x40,
  CUBE = 0x80,
  NULL_T = 0x100
};
ENABLE_BITMASK_OPERATORS(TexType)

// struct DD_Texture2D {
//   std::string m_ID;
//   std::string path;
//   GLuint handle;
//   TextureType type;
//   int Width, Height;
//   GLuint Internal_Format;         // texture object formet
//   GLuint Image_Format;            // image format
//   GLuint Wrap_S, Wrap_T, Wrap_R;  // wrapping mode along axis
//   GLuint Filter_Min;              // filter when pixels < screen pixels
//   GLuint Filter_Max;              // filter when pixels > screen pixels

//   DD_Texture2D()
//       : handle(0),
//         Internal_Format(GL_RGBA8),
//         Image_Format(GL_RGBA),
//         Wrap_S(GL_REPEAT),
//         Wrap_T(GL_REPEAT),
//         Filter_Min(GL_LINEAR_MIPMAP_LINEAR),
//         Filter_Max(GL_LINEAR),
//         loaded_to_GPU(false) {}

//   ~DD_Texture2D();
//   bool Generate(const char* full_path);
//   bool Refill(unsigned char* data);
//   bool Refill(const char* full_path);
//   inline bool loaded() { return loaded_to_GPU; }

//  private:
//   bool loaded_to_GPU;
// };

// struct DD_Skybox {
//   ~DD_Skybox() {
//     if (activated) {
//       glDeleteTextures(1, &handle);
//     }
//   }
//   std::string m_ID, right, left, top, bottom, front, back;
//   GLuint handle;
//   int m_width = 0, m_height = 0;

//   inline bool isActive() { return activated; }
//   bool Generate();
//   void GenerateNull(const int w, const int h);

//  private:
//   bool activated = false;
// };

/// \brief Container for 2D textures
struct ddTex2D {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Type of texture
  // TexType type = TexType::NULL_T;
  /// \brief Image information on GPU and RAM
  ImageInfo image_info;
};
