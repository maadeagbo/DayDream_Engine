#include "BBoxGraphics.h"
#include "BBoxLuaStructs.h"
#include "bbox_shader_enums.h"
#include "ddSceneManager.h"
#include "imgui.h"

#define NUM_GRID_LINES 11
#define NUM_GRID_AXIS 4
#define MAX_DIMEN_VAL 10

namespace {
const unsigned num_grid_points = NUM_GRID_LINES * NUM_GRID_AXIS * 2 * 3;
const unsigned num_grid_indices = NUM_GRID_LINES * NUM_GRID_AXIS * 2;

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
}  // namespace

void generate_grid() {
  // generate points (x axis, then y axis, then z axis)
  float curr_value = MAX_DIMEN_VAL / 2;
  const unsigned stride = NUM_GRID_AXIS * 3;
  for (unsigned i = 0; i < NUM_GRID_LINES; i++) {
    // x-axis
    grid_points[i * stride] = curr_value;
    grid_points[i * stride + 1] = 0;
    grid_points[i * stride + 2] = MAX_DIMEN_VAL / 2;

    // y-axis 1
    grid_points[i * stride + 3] = 0;
    grid_points[i * stride + 3 + 1] = MAX_DIMEN_VAL / 2;
    grid_points[i * stride + 3 + 2] = curr_value;

    // y-axis 2
    grid_points[i * stride + 9] = 0;
    grid_points[i * stride + 9 + 1] = curr_value;
    grid_points[i * stride + 9 + 2] = MAX_DIMEN_VAL / 2;

    // z-axis
    grid_points[i * stride + 6] = MAX_DIMEN_VAL / 2;
    grid_points[i * stride + 6 + 1] = 0;
    grid_points[i * stride + 6 + 2] = curr_value;

    // update curr_value
    curr_value -= 1.f;
  }

  curr_value = MAX_DIMEN_VAL / 2;
  const unsigned offset = NUM_GRID_LINES * NUM_GRID_AXIS * 3;
  for (unsigned i = 0; i < NUM_GRID_LINES; i++) {
    // x-axis
    grid_points[offset + i * stride] = curr_value;
    grid_points[offset + i * stride + 1] = 0;
    grid_points[offset + i * stride + 2] = -MAX_DIMEN_VAL / 2;

    // y-axis 1
    grid_points[offset + i * stride + 3] = 0;
    grid_points[offset + i * stride + 3 + 1] = -MAX_DIMEN_VAL / 2;
    grid_points[offset + i * stride + 3 + 2] = curr_value;

    // y-axis 2
    grid_points[offset + i * stride + 9] = 0;
    grid_points[offset + i * stride + 9 + 1] = curr_value;
    grid_points[offset + i * stride + 9 + 2] = -MAX_DIMEN_VAL / 2;

    // z-axis
    grid_points[offset + i * stride + 6] = -MAX_DIMEN_VAL / 2;
    grid_points[offset + i * stride + 6 + 1] = 0;
    grid_points[offset + i * stride + 6 + 2] = curr_value;

    // update curr_value
    curr_value -= 1.f;
  }

  // generate indices
  const unsigned offset_2 = NUM_GRID_LINES * NUM_GRID_AXIS;
  for (unsigned i = 0; i < (NUM_GRID_LINES * NUM_GRID_AXIS); i++) {
    grid_indices[i * 2] = i;
    grid_indices[i * 2 + 1] = i + offset_2;
  }
}

void render_grid() {
  // bind and render lines of grid
  ddCam *cam = ddSceneManager::get_active_cam();

  if (cam) {
    bbox_sh.use();

    // get camera matrices
    const glm::mat4 v_mat = ddSceneManager::calc_view_matrix(cam);
    const glm::mat4 p_mat = ddSceneManager::calc_p_proj_matrix(cam);

    // set uniforms
    bbox_sh.set_uniform((int)RE_Line::MVP_m4x4, p_mat * v_mat);
    bbox_sh.set_uniform((int)RE_Line::color_v4, glm::vec4(1.f, 1.f, 1.f, 0.5));

    // draw lines
    ddGPUFrontEnd::draw_indexed_lines_vao(grid_vao, num_grid_indices, 0);
  }
}

void fill_buffer(const BoundingBox &bbox) {
  auto set_val = [](const unsigned idx, const glm::vec3 &corner) {
    bbox_buffer[idx * 3] = corner.x;
    bbox_buffer[idx * 3 + 1] = corner.y;
    bbox_buffer[idx * 3 + 2] = corner.z;
  };

  for (unsigned i = 0; i < 8; i++) {
    switch (i) {
      case 0:
        set_val(i, bbox.corner1);
        break;
      case 1:
        set_val(i, bbox.corner2);
        break;
      case 2:
        set_val(i, bbox.corner3);
        break;
      case 3:
        set_val(i, bbox.corner4);
        break;
      case 4:
        set_val(i, bbox.corner5);
        break;
      case 5:
        set_val(i, bbox.corner6);
        break;
      case 6:
        set_val(i, bbox.corner7);
        break;
      case 7:
        set_val(i, bbox.corner8);
        break;
      default:
        break;
    }
  }
}

void render_bbox() {
  // bind and render lines of bounding box
  ddCam *cam = ddSceneManager::get_active_cam();

  if (cam) {
    bbox_sh.use();

    // get camera matrices
    const glm::mat4 v_mat = ddSceneManager::calc_view_matrix(cam);
    const glm::mat4 p_mat = ddSceneManager::calc_p_proj_matrix(cam);

    unsigned idx = 0;

    // loop thru transforms
    const std::map<unsigned, BBTransform> all_trans = get_all_transforms();
    for (auto &box : all_trans) {
      glm::mat4 m_mat =
          createMatrix(box.second.pos, box.second.rot, box.second.scale);
      // set uniforms
      bbox_sh.set_uniform((int)RE_Line::MVP_m4x4, p_mat * v_mat * m_mat);
      bbox_sh.set_uniform((int)RE_Line::color_v4,
                          glm::vec4(1.f, 0.f, 0.f, 1.f));
      ddGPUFrontEnd::draw_indexed_lines_vao(bbox_vao, 24, 0);

      // mirror
      const glm::uvec3 mirror = box.second.mirror;
      if (mirror.x == 1 || mirror.y == 1 || mirror.z == 1) {
        // color
        bbox_sh.set_uniform((int)RE_Line::color_v4,
                            glm::vec4(0.f, 0.f, 1.f, 1.f));

        // mirror the model matrix
        glm::vec3 m_vec(1.f);
        m_vec.x = mirror.x == 1 ? -1 : 1;
        m_vec.y = mirror.y == 1 ? -1 : 1;
        m_vec.z = mirror.z == 1 ? -1 : 1;
        m_mat = glm::scale(glm::mat4(), m_vec) * m_mat;

        bbox_sh.set_uniform((int)RE_Line::MVP_m4x4, p_mat * v_mat * m_mat);
        ddGPUFrontEnd::draw_indexed_lines_vao(bbox_vao, 24, 0);
      }
      idx++;
    }
  }
}

int init_gpu_stuff(lua_State *L) {
  // shader init
  cbuff<256> fname;
  bbox_sh.init();

  fname.format("%s/BBoxSkinnedMesh/%s", PROJECT_DIR, "LineRend_V.vert");
  bbox_sh.create_vert_shader(fname.str());
  fname.format("%s/BBoxSkinnedMesh/%s", PROJECT_DIR, "LineRend_F.frag");
  bbox_sh.create_frag_shader(fname.str());

  // grid lines vao
  ddGPUFrontEnd::create_vao(grid_vao);

  // grid line storage buffer
  ddGPUFrontEnd::create_storage_buffer(grid_ssbo,
                                       num_grid_points * sizeof(float));

  // grid line index buffer
  ddGPUFrontEnd::create_index_buffer(
      grid_ebo, num_grid_indices * sizeof(unsigned), grid_indices);

  // bind vao and set storage data
  ddGPUFrontEnd::bind_storage_buffer_atrribute(grid_vao, grid_ssbo,
                                               ddAttribPrimitive::FLOAT, 0, 3,
                                               3 * sizeof(float), 0);
  ddGPUFrontEnd::set_storage_buffer_contents(
      grid_ssbo, num_grid_points * sizeof(float), 0, grid_points);
  ddGPUFrontEnd::bind_index_buffer(grid_vao, grid_ebo);

  // bounding box (8 vec3 floats & 24 floats) storage buffer allocation
  ddGPUFrontEnd::create_vao(bbox_vao);
  ddGPUFrontEnd::create_storage_buffer(bbox_ssbo, 8 * sizeof(glm::vec3));
  ddGPUFrontEnd::set_storage_buffer_contents(bbox_ssbo, 8 * 3 * sizeof(float),
                                             0, bbox_buffer);
  ddGPUFrontEnd::create_index_buffer(bbox_ebo, 8 * 3 * sizeof(unsigned),
                                     bbox_indices);
  ddGPUFrontEnd::bind_storage_buffer_atrribute(bbox_vao, bbox_ssbo,
                                               ddAttribPrimitive::FLOAT, 0, 3,
                                               3 * sizeof(float), 0);
  ddGPUFrontEnd::bind_index_buffer(bbox_vao, bbox_ebo);

  // set up particle task for grid
  draw_grid.lifespan = 1.f;
  draw_grid.remain_on_q = true;
  draw_grid.rfunc = render_grid;

  ddParticleSys::add_task(draw_grid);

  // set up particle task for bounding boxes
  draw_bbox.lifespan = 1.f;
  draw_bbox.remain_on_q = true;
  draw_bbox.rfunc = render_bbox;

  ddParticleSys::add_task(draw_bbox);

  // set background color
  ddCam *cam = ddSceneManager::get_active_cam();
  if (cam) {
    cam->background_color = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
  }

  return 0;
}
