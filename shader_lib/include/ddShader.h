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

  void set_uniform(const int loc, const int data);
  void set_uniform(const int loc, const float data);
  void set_uniform(const int loc, const bool flag);
  void set_uniform(const int loc, const glm::vec2& data);
  void set_uniform(const int loc, const glm::vec3& data);
  void set_uniform(const int loc, const glm::vec4& data);
  void set_uniform(const int loc, const glm::mat3& data);
  void set_uniform(const int loc, const glm::mat4& data);

  cbuff<256> id;
  cbuff<256> vs;
  cbuff<256> fs;
  cbuff<256> gs;
  cbuff<256> cs;
  ddShaderHandle* handle = nullptr;
  int get_uniform_loc(const char* name);
};
