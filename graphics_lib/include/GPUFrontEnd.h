#pragma once
#include <inttypes.h>
#ifdef WIN32
#define GLM_FORCE_CXX98
#define GLM_FORCE_CXX11
#define GLM_FORCE_CXX14
#define GLM_FORCE_PURE
#pragma warning(disable : 4201)  // removes non-standard extensions warnings
#endif
#include <glm/glm.hpp>
#include "Container.h"
#include "StringLib.h"

// Buffer declarations (API defined)
struct ddVAOData;
struct ddInstBufferData;
struct ddMeshBufferData;
struct ddTextureData;
struct ddStorageBufferData;
struct ddIndexBufferData;

/** \brief Frame buffer declarations (API defined) */
enum class ddBufferType : unsigned {
  GEOM,
  LIGHT,
  PARTICLE,
  SHADOW,
  CUBE,
  FILTER,
  DEFAULT,
  NUM_TYPES
};
struct ddGBuffer;
struct ddShadowBuffer;
struct ddLightBuffer;
struct ddParticleBuffer;
struct ddCubeMapBuffer;
struct ddFilterBuffer;

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

enum class CubeMapFaces : unsigned {
  RIGHT = 0,
  LEFT,
  TOP,
  BOTTOM,
  BACK,
  FRONT,
  NUM_FACES
};

struct ImageInfo {
  /// \brief Handle to GPU buffer
  ddTextureData *tex_buff = nullptr;
  /// \brief width and height
  int width = 0, height = 0;
  /// \brief channels in image (R, G, B, and/or A)
  int channels = 0;
  /// \brief Texture object formet
  unsigned internal_format;
  /// \brief Image formet
  unsigned image_format;
  /// \brief Wrapping mode along axis
  unsigned wrap_s, wrap_t, wrap_r;
  /// \brief filter when pixels < screen pixels
  unsigned min_filter;
  /// \brief filter when pixels > screen pixels
  unsigned mag_filter;
  /// \brief pointer to image data in RAM
  dd_array<unsigned char> image_data[(unsigned)CubeMapFaces::NUM_FACES];
  /// \brief path to image file
  cbuff<256> path[(unsigned)CubeMapFaces::NUM_FACES];
};

/** \brief Primitive buffer attribute types */
enum class ddAttribPrimitive : unsigned { BOOL = 0, INT, UINT, FLOAT, DOUBLE };

/** \brief Front end interace for using low-level graphics API */
namespace ddGPUFrontEnd {
// Wipe back-buffer screen
void clear_screen(const float r = 0.f, const float g = 0.f, const float b = 0.f,
                  const float a = 1.f);
// Initialize Graphics API library
bool load_api_library(const bool display_info = true);
// Spot check for API errors (returns true if errors found)
bool spot_check_errors(const char *sig);

//*****************************************************************************

// Delete texture data
void destroy_texture(ddTextureData *&tex_ptr);
// Create gpu texture data (RBGA8, mip-map linear, Repeats in U & V axis)
// witdh and height must be set in ImageInfo prior to call
bool generate_texture2D_RGBA8_LR(ImageInfo &img);
// Create gpu texture data (RBGA16f, mip-map linear, Repeats in U & V axis)
// witdh and height must be set in ImageInfo prior to call
bool generate_texture2D_RGBA16F_LR(ImageInfo &img);
// Create cubemap texture data (empty -> false means create w/out data)
// witdh and height must be set in ImageInfo prior to call
bool generate_textureCube_RGBA8_LR(ImageInfo &img, const bool empty = false);
// Save image (ignores ImGUI UI if present) of provided buffer and format
void grab_image_from_buffer(const unsigned frame_buff, const unsigned format,
                            dd_array<unsigned char> pixel, const unsigned width,
                            const unsigned height);

//*****************************************************************************

// Create gpu buffer data
bool load_buffer_data(ddMeshBufferData *&mbuff_ptr, DDM_Data *ddm_ptr);
// Delete ddGBuffer data
void destroy_buffer_data(ddMeshBufferData *&mbuff_ptr);
// Create ddInstBufferData from instance size (must set width & height fields)
bool load_instance_data(ddInstBufferData *&ibuff_ptr, const int inst_size);
// Delete ddInstBufferData
void destroy_instance_data(ddInstBufferData *&ibuff_ptr);
// Create empty ddVAOData object
bool create_vao(ddVAOData *&vbuff_ptr);
// Destroy ddVAOData object
void destroy_vao(ddVAOData *&vbuff_ptr);
// Bind ddMeshBufferData and/or ddInstBufferData buffer to ddVAOData object
bool bind_object(ddVAOData *vbuff_ptr, ddInstBufferData *ibuff_ptr,
                 ddMeshBufferData *mbuff_ptr);
// Destroy ddStorageBufferData object
void destroy_storage_buffer(ddStorageBufferData *&sbuff_ptr);
// Create ddStorageBufferData buffer
bool create_storage_buffer(ddStorageBufferData *&sbuff_ptr,
                           const unsigned byte_size);
// Destroy ddIndexBufferData object
void destroy_index_buffer(ddIndexBufferData *&ebuff_ptr);
// Create and load ddIndexBufferData
bool create_index_buffer(ddIndexBufferData *&ebuff_ptr,
                         const unsigned byte_size, const void *data);
// Extract ddStorageBufferData data to provided buffer
bool extract_storage_buffer_data(ddStorageBufferData *sbuff_ptr,
                                 const unsigned buff_size_bytes,
                                 void *data_storage);
// Bind storage buffer object
void bind_storage_buffer(const unsigned location,
                         const ddStorageBufferData *sbuff_ptr);
// Fill in dynamically allocated (on gpu) instance buffer
// true = instance m4x4, false = color vec3
void set_instance_buffer_contents(const ddInstBufferData *ibuff_ptr,
                                  bool inst_col, const unsigned byte_size,
                                  const unsigned offset, void *data);
// Fill in dynamically allocated (on gpu) shader storage buffer
void set_storage_buffer_contents(const ddStorageBufferData *sbuff_ptr,
                                 const unsigned byte_size,
                                 const unsigned offset, void *data);
// Bind parts of ssbo to VAO object
void bind_storage_buffer_atrribute(const ddVAOData *vao,
                                   const ddStorageBufferData *sbuff_ptr,
                                   ddAttribPrimitive type,
                                   const unsigned attrib_loc,
                                   const unsigned num_attribs_of_type,
                                   const unsigned stride_in_bytes,
                                   const unsigned offset_in_stride);
// Bind index buffer to VAO
void bind_index_buffer(const ddVAOData *vao,
                       const ddIndexBufferData *ebuff_ptr);

//*****************************************************************************

// Render full screen quad
void render_quad();
// Render split screen quad (true = render left quad, false = render right quad)
void render_split_quad(bool leftside);
// Render cube map
void render_cube();
// Render line segment
// TODO: change shader to use SSBO for opengl
// void render_line_segment(dd_array<glm::vec4>& points3d

//*****************************************************************************

// Create geometry frame buffer
void create_gbuffer(const int width, const int height);
// Create light frame buffer
void create_lbuffer(const int width, const int height);
// Create shadow frame buffer
void create_sbuffer(const int width, const int height);
// Create particle frame buffer
void create_pbuffer(const int width, const int height);
// Create cube map frame buffer
void create_cbuffer(const int width, const int height);
// Create filter image frame buffer
void create_fbuffer(const int c_width, const int c_height, const int s_width,
                    const int s_height);
// Bind framebuffer
// boolean flag is for choosing to bind color or shadow filter buffer
// (True = color buffer, false = shadow buffer)
void bind_framebuffer(const ddBufferType type, const bool color_shadow = true);
// Bind textures associated w/ framebuffer
// loc = texture location bound to shader
// xtra_param -> cube map = CubeMapFaces enum, filter = color(0) or shadow(1),
// shadow = lighting pass(0) or other(1)
void bind_pass_texture(const ddBufferType type, const unsigned loc = 0,
                       const unsigned xtra_param = 0);
// Bind framebuffers and copy depth from in -> out
void blit_depth_buffer(const ddBufferType in_type, const ddBufferType out_type,
                       const unsigned width, const unsigned height);

//*****************************************************************************

// Set framebuffer viewport dimensions for render
void set_viewport(const unsigned width, const unsigned height);
// Clear color buffer for framebuffer
void clear_color_buffer();
// Clear depth buffer for framebuffer
void clear_depth_buffer();
// Clear stencil buffer for framebuffer
void clear_stencil_buffer();
// Enable or disable framebuffer depth mask
void toggle_depth_mask(bool flag = false);
// Enable or disable framebuffer depth test
void toggle_depth_test(bool flag = false);
// Enable or disable framebuffer stencil test
void toggle_stencil_test(bool flag = false);
// Enable or disable framebuffer additive blending
void toggle_additive_blend(bool flag = false);
// Enable or disable framebuffer alpha blending
void toggle_alpha_blend(bool flag = false);
// Enable or disable wireframe draw
void toggle_wireframe(bool flag = false);

enum DepthMode { LESS_OR_EQUAL, GREATER, LESS };
// Set how depth testing function behaves
void set_depth_mode(const DepthMode mode);
// Enable or disable triangle face cull
void toggle_face_cull(bool flag = false);
// Set backface or frontface culling
void set_face_cull(const bool backface = true);
// Enable or disable shader clip plane
void toggle_clip_plane(bool flag = false);

// Bind arbitrary texture to shader location
void bind_texture(const unsigned location, ddTextureData *tex_data);

// Draw ddVAOObject instanced
void draw_instanced_vao(const ddVAOData *vao, const unsigned num_indices,
                        const unsigned instance_size);

// Draw ddVAOObject indexed
void draw_indexed_vao(const ddVAOData *vao, const unsigned num_indices,
                      const unsigned offset_in_index_buffer);

// Draw ddVAOObject indexed
void draw_indexed_lines_vao(const ddVAOData *vao, const unsigned num_indices,
                      const unsigned offset_in_index_buffer);

// Draw ddStorageBufferData as points
void draw_points(const ddVAOData *vao, const ddStorageBufferData *sbuff_ptr,
                 ddAttribPrimitive type, const unsigned attrib_loc,
                 const unsigned num_attribs, const unsigned stride,
                 const unsigned offset_in_stride,
                 const unsigned offset_in_buffer, const unsigned num_points);

// Deploy compute shader
void deploy_compute_task(const unsigned x_work_groups,
                         const unsigned y_work_groups,
                         const unsigned z_work_groups);
}  // namespace ddGPUFrontEnd
