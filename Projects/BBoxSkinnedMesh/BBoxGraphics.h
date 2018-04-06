#pragma once

#include <map>
#include "ddParticleSystem.h"
#include "ddSceneManager.h"
#include "ddShader.h"

#define NUM_GRID_LINES 11
#define NUM_GRID_AXIS 4
#define MAX_DIMEN_VAL 10

const unsigned num_grid_points = NUM_GRID_LINES * NUM_GRID_AXIS * 2 * 3;
const unsigned num_grid_indices = NUM_GRID_LINES * NUM_GRID_AXIS * 2;

struct BBTransform {
  glm::vec3 pos = glm::vec3(0.f);
  glm::vec3 scale = glm::vec3(1.f);
  glm::vec3 rot = glm::vec3(0.f);
  glm::uvec3 mirror = glm::uvec3(0.f);
  glm::ivec2 joint_ids = glm::ivec2(-1);
};

struct BBoxGraphics {
  ddPTask draw_grid;
  ddPTask draw_bbox;

  ddShader bbox_sh;

  // grid buffers
  ddVAOData *grid_vao = nullptr;
  ddStorageBufferData *grid_ssbo = nullptr;
  ddIndexBufferData *grid_ebo = nullptr;

  // bbox buffers
  ddVAOData *bbox_vao = nullptr;
  ddStorageBufferData *bbox_ssbo = nullptr;
  ddIndexBufferData *bbox_ebo = nullptr;
  float bbox_buffer[8 * 3];

  // bbox container
  std::map<unsigned, BoundingBox> bbox_container;
  std::map<unsigned, BBTransform> bbox_trans;

  // bbox indices
  unsigned bbox_indices[12 * 2] = {
      // front
      // tl_f -> tr_f
      0, 1,
      // tr_f -> br_f
      1, 2,
      // br_f -> bl_f
      2, 3,
      // bl_f -> tl_f
      3, 0,

      // back
      // tl_b -> tr_b
      4, 5,
      // tr_b -> br_b
      5, 6,
      // br_b -> bl_b
      6, 7,
      // bl_b -> tl_b
      7, 4,

      // sides
      // tl_f -> tl_b
      0, 4,
      // tr_f -> tr_b
      3, 7,
      // br_f -> br_b
      2, 6,
      // bl_f -> bl_b
      1, 5};

  // 3 sets of gridlines w/ 2 points (3 vertices) each
  float grid_points[num_grid_points];
  // 3 sets of grid lines w/ 2 indices each (all indices are unique)
  unsigned grid_indices[num_grid_indices];

  // generate grid line points and indices
  void generate_grid();
};
