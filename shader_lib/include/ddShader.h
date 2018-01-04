#pragma once

#include <glm/glm.hpp>
#include <map>
#include "StringLib.h"

struct ddShaderHandle;

struct ddQueryInfo {
  ddQueryInfo() {}
  ddQueryInfo(const int loc, const char* n, unsigned t)
      : location(loc), type(t), name(n) {}
  int location = -1;
  unsigned type;
  cbuff<64> name;
};
const char* get_uniform_type(unsigned type);

struct ddShader {
  ~ddShader();

  /// \brief Initialize shader program
  void init();
  /// \brief Create vertex shader
  void create_vert_shader(const char* filePath);
  /// \brief Create fragment shader
  void create_frag_shader(const char* filePath);
  /// \brief Create compute shader
  void create_comp_shader(const char* filePath);
  /// \brief Create geometry shader
  void create_geom_shader(const char* filePath);
  /// \brief Retrieve attributes from the shader program
  dd_array<ddQueryInfo> query_shader_attributes();
  /// \brief
  dd_array<ddQueryInfo> query_uniforms();
  /// \brief Make this shader active
  void use();

  cbuff<256> id;
  cbuff<256> vs;
  cbuff<256> fs;
  cbuff<256> gm;
  cbuff<256> cm;
  ddShaderHandle* handle = nullptr;
  std::map<cbuff<64>, unsigned> uniform_loc;
  unsigned get_uniform_loc(const char* name);
};
