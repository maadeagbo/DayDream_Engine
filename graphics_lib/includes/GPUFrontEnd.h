#pragma once
#include <inttypes.h>
#include <glm/glm.hpp>
#include "Container.h"
#include "StringLib.h"

struct ddVAOData;
struct ddInstBufferData;
struct ddMeshBufferData;

struct Vertex {
  float position[3] = {0, 0, 0};
  float normal[3] = {0, 0, 0};
  float texCoords[2] = {0, 0};
  float tangent[3] = {0, 0, 0};
  float blendweight[4] = {0, 0, 0, 0};
  float joints[4] = {0, 0, 0, 0};
};

struct obj_mat {
  cbuff<32> mat_id;
  glm::vec4 diff_raw;
  cbuff<256> albedo_tex, specular_tex, metalness_tex, roughness_tex, normal_tex,
      emissive_tex, ao_tex;
  bool albedo_flag, spec_flag, metal_flag, rough_flag, norm_flag, emit_flag,
      ao_flag, multiplier;

  obj_mat() {
    diff_raw = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
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

struct DDM_Data {
  dd_array<unsigned> indices;
  dd_array<Vertex> data;
  obj_mat material_info;
  cbuff<256> path;
  glm::vec3 bb_min, bb_max;
  size_t mat_id;
};

namespace ddGPUFrontEnd {
// Wipe back-buffer screen
void clear_screen(const float r = 0.f, const float g = 0.f, const float b = 0.f,
                  const float a = 1.f);
// Initialize Graphics API library
bool load_api_library(const bool display_info = true);
// Create gpu buffer data
bool load_buffer_data(ddMeshBufferData *&buff_ptr, DDM_Data *ddm_ptr);
// Delete ddGBuffer data
void destroy_buffer_data(ddMeshBufferData *&buff_ptr);
// Create ddInstBufferData from instance size
bool load_instance_data(ddInstBufferData *&ibuff_ptr, const int inst_size);
// Delete ddInstBufferData
void destroy_instance_data(ddInstBufferData *&ibuff_ptr);
// Create empty ddVAOData object
bool create_vao(ddVAOData *&vbuff_ptr);
// Destroy ddVAOData object
void destroy_vao(ddVAOData *&vbuff_ptr);
// Bind ddMeshBufferData and/or ddInstBufferData buffer to ddVAOData object
void bind_object(ddVAOData *vbuff_ptr, ddInstBufferData *ibuff_ptr,
                 ddMeshBufferData *buff_ptr);
// Unbind provided buffers attached to ddVAOData object
void unbind_object(ddVAOData *vbuff_ptr, ddInstBufferData *ibuff_ptr,
                   ddMeshBufferData *buff_ptr);
}