#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	obj_vec:
*		- 3-float array container
*			- can be accessed w/ x(), y(), and z()
*			- can be accessed w/ data[]
*			- overloaded + & - operators
*			- returns normailized obj_vec
*	Vertex:
*		- Container for:
*			- 3-float position array
*			- 3-float normal array
*			- 3-float tangent array
*			- 2-float uv array
*	obj_mat:
*		- Container for material information
*			- ID --> material id
*			- directory --> location for texture images
*			- Boolean flag and file names for these textures:
*				- albedo
*				- albedo (multiplier)
*				- specular
*				- normal
*				- emittion
*				- metalness
*				- ambient occlusion
*	MeshData:
*		- Container for all mesh information in OpenGL format
*			- indices --> for drawing
*			- data --> array of Vertex structs
*			- material info (obj_mat) and ID (for quick access)
*			- Bounding box of mesh (min and max corners)
*	DD_LineAgent:
*		- Container for all Line rendering elements (uses LinePoint &&
XZLine)
*	Bounding Box:
*		-
*
*	TODO:
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"

struct obj_vec3 {
  float data[3];
  obj_vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
    data[0] = x;
    data[1] = y;
    data[2] = z;
  }

  obj_vec3(float _data[3]) {
    data[0] = _data[0];
    data[1] = _data[1];
    data[2] = _data[2];
  }

  float& x() { return data[0]; }
  float const& x() const { return data[0]; }
  float& y() { return data[1]; }
  float const& y() const { return data[1]; }
  float& z() { return data[2]; }
  float const& z() const { return data[2]; }

  inline obj_vec3 operator+(const obj_vec3 other) const {
    return obj_vec3(data[0] + other.data[0], data[1] + other.data[1],
                    data[2] + other.data[2]);
  }

  inline obj_vec3 operator-(const obj_vec3 other) const {
    return obj_vec3(data[0] - other.data[0], data[1] - other.data[1],
                    data[2] - other.data[2]);
  }

  inline obj_vec3 normalize() const {
    float mag =
        std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]);
    return obj_vec3(data[0] / mag, data[1] / mag, data[2] / mag);
  }
};

struct obj_mat {
  std::string ID;
  std::string directory;
  obj_vec3 diffuseRaw;
  std::string albedo_tex, specular_tex, metalness_tex, roughness_tex,
      normal_tex, emissive_tex, ao_tex;
  bool albedo_flag, spec_flag, metal_flag, rough_flag, norm_flag, emit_flag,
      ao_flag, multiplier;

  obj_mat() {
    diffuseRaw = obj_vec3(0.5f, 0.5f, 0.5f);
    albedo_flag = false;
    spec_flag = false;
    ao_flag = false;
    metal_flag = false;
    rough_flag = false;
    norm_flag = false;
    emit_flag = false;
    multiplier = false;
  }
};

struct Vertex {
  float position[3] = {0, 0, 0};
  float normal[3] = {0, 0, 0};
  float texCoords[2] = {0, 0};
  float tangent[3] = {0, 0, 0};
  float blendweight[4] = {0, 0, 0, 0};
  float joints[4] = {0, 0, 0, 0};
};

struct MeshData {
  dd_array<int> indices;
  dd_array<Vertex> data;
  obj_mat material_info;
  std::string material_ID;
  obj_vec3 bbox_min, bbox_max;
};

struct MeshContainer {
  dd_array<Vertex> data;
  glm::vec3 bbox_min;
  glm::vec3 bbox_max;
  dd_array<obj_mat> material_info;
  dd_array<unsigned> indices;
  dd_2Darray<unsigned> mesh_idx;
};

struct LinePoint {
  glm::vec4 pos01, pos02;
};

struct DD_LineAgent {
  dd_array<LinePoint> lines;
  dd_array<glm::vec4> m_buffer;
  glm::vec4 color = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
  std::string m_ID = "";
  bool flag_render = true, m_flagXZ = true;
  DD_LineAgent(const size_t size = 0) : lines(size) {}
  inline void FlushLines() {
    lines.resize(0);
    m_buffer.resize(0);
  }
};

struct BoundingBox {
  glm::vec3 min, max;
  glm::vec3 corner1, corner2, corner3, corner4, corner5, corner6, corner7,
      corner8;
  dd_array<glm::vec4> buffer = dd_array<glm::vec4>(12 * 2);

  void SetCorners();
  BoundingBox transformCorners(const glm::mat4 transMat);
  glm::vec3 UpdateAABB_min();
  glm::vec3 UpdateAABB_max();
  glm::vec3 GetFrustumPlaneMin(glm::vec3 norm);
  glm::vec3 GetFrustumPlaneMax(glm::vec3 norm);
  void SetLineBuffer();
};

/// \brief Container for model information
struct ModelIDs {
	float _near = 0.f, _far = 100.f;
	cbuff<32> model;
};

/// \brief Container for manipulating transforms
struct DD_Body {
	btRigidBody *body;
	btTransform t_f;
	dd_array<BoundingBox> BBox;
};

// Useful glm functions
glm::vec4 getVec4f(const char* str);

glm::uvec4 getVec4u(const char* str);

glm::vec3 getVec3f(const char* str);

glm::vec2 getVec2f(const char* str);

glm::quat getQuat(const char* str);

std::string Vec4f_Str(const glm::vec4 vIn);

glm::mat4 createMatrix(const glm::vec3& pos, const glm::vec3& rot,
                       const glm::vec3& scale);

void printGlmMat(glm::mat4 mat);
