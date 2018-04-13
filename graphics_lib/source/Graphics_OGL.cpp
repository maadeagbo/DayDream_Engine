#include <cstdio>
#include "GPUFrontEnd.h"
#include "gl_core_4_3.h"

struct ddVAOData {
  GLuint vao_handle;
};

struct ddInstBufferData {
  GLuint instance_buffer;
  GLuint color_instance_buffer;
};

struct ddMeshBufferData {
  GLuint vertex_buffer;
  GLuint index_buffer;
};

struct ddTextureData {
  GLuint texture_handle;
};

struct ddStorageBufferData {
  GLuint storage_handle;
};

struct ddIndexBufferData {
  GLuint index_handle;
};

//*****************************************************************************

struct ddGBuffer {
  GLuint deffered_fbo, depth_buf, pos_tex, norm_tex, color_tex, xtra_tex;
};

struct ddShadowBuffer {
  GLuint shadow_fbo, depth_buf, shadow_tex, width, height;
};

struct ddLightBuffer {
  GLuint light_fbo, depth_buf, color_tex;
};

struct ddParticleBuffer {
  GLuint particle_fbo, depth_buf, color_tex, xtra_tex;
};

struct ddCubeMapBuffer {
  GLuint cube_fbo, depth_buf;
  ImageInfo cube_texture;
};

struct ddFilterBuffer {
  GLuint color_fbo, shadow_fbo, color_tex, shadow_tex;
};

//*****************************************************************************

const GLenum attrib_type_ogl[] = {GL_BOOL, GL_INT, GL_UNSIGNED_INT, GL_FLOAT,
                                  GL_DOUBLE};
const unsigned attrib_size_ogl[] = {sizeof(GLboolean), sizeof(GLint),
                                    sizeof(GLuint), sizeof(GLfloat),
                                    sizeof(GLdouble)};

//*****************************************************************************

namespace {
// Error processing function for OpenGL calls
bool gl_error(const char *signature) {
  GLenum err;
  bool flag = false;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%s::OpenGL error::%d\n", signature, err);
    flag = true;
  }
  return flag;
}

void create_texture2D(const GLenum format, GLuint &tex_handle, const int width,
                      const int height, GLenum min = GL_LINEAR,
                      GLenum mag = GL_LINEAR, GLenum wrap_s = GL_REPEAT,
                      GLenum wrap_t = GL_REPEAT, GLenum wrap_r = GL_REPEAT,
                      glm::vec4 bcol = glm::vec4(-1), const bool mips = false,
                      unsigned char *data = nullptr,
                      GLenum data_formet = GL_RGBA8) {
  // create texture
  glGenTextures(1, &tex_handle);
  glBindTexture(GL_TEXTURE_2D, tex_handle);
  POW2_VERIFY_MSG(!gl_error("create_texture2D"), "Error generating buffer", 0);

  // texture settings
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrap_r);
  if (bcol.x >= 0.f) {
    GLfloat border[] = {bcol.x, bcol.y, bcol.z, bcol.w};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
  }

  // generate
  const int num_mipmaps =
      mips ? (int)floor(log2(std::max(width, height))) + 1 : 1;
  glTexStorage2D(GL_TEXTURE_2D, num_mipmaps, format, width, height);
  // fill w/ data
  if (data) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, data_formet,
                    GL_UNSIGNED_BYTE, data);
    POW2_VERIFY_MSG(!gl_error("create_texture2D"), "Error generating storage",
                    0);
  }
  // mip maps
  if (mips) glGenerateMipmap(GL_TEXTURE_2D);

  POW2_VERIFY_MSG(!gl_error("create_texture2D"), "Error generating image2D", 0);
}

// screen space quad
GLuint quad_vao = 0, quad_vbo = 0;

// vr split-screen quad
GLuint vr_vao[2] = {0, 0}, vr_vbo[2] = {0, 0};

// cube map
GLuint cube_vao = 0, cube_vbo = 0;

// line render
// GLuint line_vao = 0, line_vbo = 0;

// buffer for writing arbitrary pixels
dd_array<unsigned char> pixel_write_buffer;

// Buffers
ddGBuffer g_buff;
ddLightBuffer l_buff;
ddShadowBuffer s_buff;
ddParticleBuffer p_buff;
ddCubeMapBuffer c_buff;
ddFilterBuffer f_buff;

}  // namespace

//*****************************************************************************

namespace ddGPUFrontEnd {

void clear_screen(const float r, const float g, const float b, const float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

bool load_api_library(const bool display_info) {
  // Initialize GLLoadGen to setup the OpenGL Function pointers
  int loaded = ogl_LoadFunctions();
  if (loaded == ogl_LOAD_FAILED) {
    int num_failed = loaded - ogl_LOAD_SUCCEEDED;
    printf("Number of functions that failed to load: %d\n", num_failed);
    return false;
  }

  if (display_info) {
    // check opengl version supported
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    // check number of color attachments and drawbuffer attachments
    GLint maxAttach = 0, maxDrawBuf = 0;
    ;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuf);

    // check compute shader limitations
    int maxComputeWG_COUNT_0 = 0, maxComputeWG_COUNT_1 = 0,
        maxComputeWG_COUNT_2 = 0, maxComputeWG_SIZE_0 = 0,
        maxComputeWG_SIZE_1 = 0, maxComputeWG_SIZE_2 = 0,
        maxComputeWG_INVOCATIONS = 0;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxComputeWG_COUNT_0);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxComputeWG_COUNT_1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxComputeWG_COUNT_2);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxComputeWG_SIZE_0);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxComputeWG_SIZE_1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxComputeWG_SIZE_2);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                  &maxComputeWG_INVOCATIONS);

    printf("GL Vendor             :%s\n", vendor);
    printf("GL Renderer           :%s\n", renderer);
    printf("GL Version (string)   :%s\n", version);
    printf("GL Version (integer)  :%d.%d\n", major, minor);
    printf("GLSL Version          :%s\n\n", glslVersion);
    printf("GPU Max Color Attachments    :%d\n", maxAttach);
    printf("GPU Max Draw Buffers         :%d\n\n", maxDrawBuf);
    printf("GPU Max Compute Work Group Count (0)    :%d\n",
           maxComputeWG_COUNT_0);
    printf("GPU Max Compute Work Group Count (1)    :%d\n",
           maxComputeWG_COUNT_1);
    printf("GPU Max Compute Work Group Count (2)    :%d\n",
           maxComputeWG_COUNT_2);
    printf("GPU Max Compute Work Group Size  (0)    :%d\n",
           maxComputeWG_SIZE_0);
    printf("GPU Max Compute Work Group Size  (1)    :%d\n",
           maxComputeWG_SIZE_1);
    printf("GPU Max Compute Work Group Size  (2)    :%d\n",
           maxComputeWG_SIZE_2);
    printf("GPU Max Compute Work Group Invocations  :%d\n\n",
           maxComputeWG_INVOCATIONS);
  }
  return true;
}

bool spot_check_errors(const char *sig) { return gl_error(sig); }

//*****************************************************************************

void destroy_texture(ddTextureData *&tex_ptr) {
  if (!tex_ptr) return;

  glDeleteTextures(1, &tex_ptr->texture_handle);
  gl_error("destroy_texture");

  delete tex_ptr;
  tex_ptr = nullptr;
}

bool generate_texture2D_RGBA8_LR(ImageInfo &img) {
  if (img.tex_buff) destroy_texture(img.tex_buff);

  // set up image parameters
  img.internal_format = GL_RGBA8;
  img.image_format = GL_RGBA;  // TODO: should set based on channels ?
  img.wrap_s = GL_REPEAT;
  img.wrap_t = GL_REPEAT;
  img.min_filter = GL_LINEAR_MIPMAP_LINEAR;
  img.mag_filter = GL_LINEAR;

  // create handle and texture
  img.tex_buff = new ddTextureData();
  if (!img.tex_buff) {
    fprintf(stderr, "generate_texture2D_RGBA8_LR::RAM load failure\n");
    return false;
  }

  // create image
  unsigned char *img_ptr =
      (img.image_data[0].size() > 0) ? &img.image_data[0][0] : nullptr;
  create_texture2D(img.internal_format, img.tex_buff->texture_handle, img.width,
                   img.height, img.min_filter, img.mag_filter, img.wrap_s,
                   img.wrap_t, GL_REPEAT, glm::vec4(-1.f), true, img_ptr,
                   img.image_format);
  // SOIL_free_image_data(img.image_data[0]);
  // img.image_data[0] = nullptr;

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool generate_texture2D_RGBA16F_LR(ImageInfo &img) {
  if (img.tex_buff) destroy_texture(img.tex_buff);

  // set up image parameters
  img.internal_format = GL_RGBA16F;
  img.image_format = GL_RGBA;  // TODO: should set based on channels ?
  img.wrap_s = GL_REPEAT;
  img.wrap_t = GL_REPEAT;
  img.min_filter = GL_LINEAR_MIPMAP_LINEAR;
  img.mag_filter = GL_LINEAR;

  // create handle and texture
  img.tex_buff = new ddTextureData();
  if (!img.tex_buff) {
    fprintf(stderr, "generate_texture2D_RGBA8_LR::RAM load failure\n");
    return false;
  }

  // create image
  unsigned char *img_ptr =
      (img.image_data[0].size() > 0) ? &img.image_data[0][0] : nullptr;
  create_texture2D(img.internal_format, img.tex_buff->texture_handle, img.width,
                   img.height, img.min_filter, img.mag_filter, img.wrap_s,
                   img.wrap_t, GL_REPEAT, glm::vec4(-1.f), true, img_ptr,
                   img.image_format);
  // SOIL_free_image_data(img.image_data[0]);
  // img.image_data[0] = nullptr;

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool generate_textureCube_RGBA8_LR(ImageInfo &img, const bool empty) {
  if (img.tex_buff) destroy_texture(img.tex_buff);

  // set up image parameters
  img.internal_format = GL_RGBA8;
  img.image_format = GL_RGBA;  // TODO: should set based on channels ?
  img.wrap_s = GL_REPEAT;
  img.wrap_t = GL_REPEAT;
  img.wrap_r = GL_REPEAT;
  img.min_filter = GL_LINEAR;
  img.mag_filter = GL_LINEAR;

  // create handle and texture
  img.tex_buff = new ddTextureData();
  if (!img.tex_buff) {
    fprintf(stderr, "generate_textureCube_RGBA8_LR::RAM load failure\n");
    return false;
  }
  glGenTextures(1, &img.tex_buff->texture_handle);
  if (gl_error("generate_textureCube_RGBA8_LR::Creating texture")) {
    return false;
  }
  glBindTexture(GL_TEXTURE_CUBE_MAP, img.tex_buff->texture_handle);
  // texture settings
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, img.wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, img.wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, img.wrap_r);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, img.min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, img.mag_filter);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  // empty flag means generate dummy cubemap texture that will be rendered to
  if (empty) {
    // generate
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, img.internal_format, img.width,
                   img.height);
    if (gl_error("generate_textureCube_RGBA8_LR::Creating empty")) {
      return false;
    }
  } else {
    GLuint targets[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
    // generate cubemap texture, then apply each face based on targets array
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, img.internal_format, img.width,
                   img.height);

    cbuff<100> err_msg;
    // candidate for omp multi-threading
    for (int i = 0; i < 6; i++) {
      // transfer image to GPU then delete from RAM
      glTexSubImage2D(targets[i], 0, 0, 0, img.width, img.height,
                      img.image_format, GL_UNSIGNED_BYTE,
                      &img.image_data[i][0]);
      // SOIL_free_image_data(img.image_data[i]);
      // img.image_data[i] = nullptr;

      err_msg.format("generate_textureCube_RGBA8_LR::Loading image <%d>", i);
      if (gl_error(err_msg.str())) return false;
    }
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  return true;
}

void grab_image_from_buffer(const unsigned frame_buff, const unsigned format,
                            dd_array<unsigned char> pixels,
                            const unsigned width, const unsigned height) {
  // bind buffer for read
  glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)frame_buff);
  glReadPixels(0, 0, width, height, (GLenum)format, GL_UNSIGNED_BYTE,
               &pixels[0]);
  POW2_VERIFY_MSG(!gl_error("grab_image_from_buffer"), "Failed to screen grab",
                  0);
}

//*****************************************************************************

bool load_buffer_data(ddMeshBufferData *&mbuff_ptr, DDM_Data *ddm_ptr) {
  // Create ddMeshBufferData and load gpu w/ buffer data
  if (mbuff_ptr) destroy_buffer_data(mbuff_ptr);

  // create and load: vertex and index buffer objects
  GLuint vbo = 0, ebo = 0;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  if (gl_error("load_buffer_data::Creating buffers")) return false;

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, ddm_ptr->data.sizeInBytes(), &ddm_ptr->data[0],
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ddm_ptr->indices.sizeInBytes(),
               &ddm_ptr->indices[0], GL_STATIC_DRAW);

  if (gl_error("load_buffer_data::Loading buffers")) return false;
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  mbuff_ptr = new ddMeshBufferData();
  if (!mbuff_ptr) {
    fprintf(stderr, "Failed to create ddMeshBufferData object in RAM\n");
    return false;
  }
  mbuff_ptr->vertex_buffer = vbo;
  mbuff_ptr->index_buffer = ebo;

  return true;
}

void destroy_buffer_data(ddMeshBufferData *&mbuff_ptr) {
  // free initialized data and destroy buffer object
  if (!mbuff_ptr) return;

  glDeleteBuffers(1, &mbuff_ptr->vertex_buffer);
  glDeleteBuffers(1, &mbuff_ptr->index_buffer);

  gl_error("destroy_buffer_data");

  delete mbuff_ptr;
  mbuff_ptr = nullptr;
}

bool load_instance_data(ddInstBufferData *&ibuff_ptr, const int inst_size) {
  if (ibuff_ptr) destroy_instance_data(ibuff_ptr);

  // create dynamic buffer and set max data size
  GLuint inst_m4x4 = 0, inst_v3 = 0;
  glGenBuffers(1, &inst_m4x4);
  glGenBuffers(1, &inst_v3);
  if (gl_error("load_instance_data::Creating buffers")) return false;

  glBindBuffer(GL_ARRAY_BUFFER, inst_m4x4);
  glBufferData(GL_ARRAY_BUFFER, inst_size * sizeof(glm::mat4), NULL,
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, inst_v3);
  glBufferData(GL_ARRAY_BUFFER, inst_size * sizeof(glm::vec3), NULL,
               GL_DYNAMIC_DRAW);

  if (gl_error("load_buffer_data::Loading buffers")) return false;
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  ibuff_ptr = new ddInstBufferData();
  if (!ibuff_ptr) {
    fprintf(stderr, "Failed to create ddInstBufferData object in RAM\n");
    return false;
  }
  ibuff_ptr->instance_buffer = inst_m4x4;
  ibuff_ptr->color_instance_buffer = inst_v3;

  return true;
}

void destroy_instance_data(ddInstBufferData *&ibuff_ptr) {
  // free initialized data and destroy buffer object
  if (!ibuff_ptr) return;

  glDeleteBuffers(1, &ibuff_ptr->instance_buffer);
  glDeleteBuffers(1, &ibuff_ptr->color_instance_buffer);
  gl_error("destroy_instance_data");

  delete ibuff_ptr;
  ibuff_ptr = nullptr;
}

bool create_vao(ddVAOData *&vbuff_ptr) {
  if (vbuff_ptr) destroy_vao(vbuff_ptr);

  // create vertex array object
  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  if (gl_error("create_vao")) return false;

  vbuff_ptr = new ddVAOData();
  if (!vbuff_ptr) {
    fprintf(stderr, "Failed to create ddVAOData object in RAM\n");
    return false;
  }
  vbuff_ptr->vao_handle = vao;

  return true;
}

void destroy_vao(ddVAOData *&vbuff_ptr) {
  if (!vbuff_ptr) return;

  // destroy vertex array object
  glDeleteVertexArrays(1, &vbuff_ptr->vao_handle);
  gl_error("destroy_vao");

  delete vbuff_ptr;
  vbuff_ptr = nullptr;
}

bool bind_object(ddVAOData *vbuff_ptr, ddInstBufferData *ibuff_ptr,
                 ddMeshBufferData *mbuff_ptr) {
  if (!vbuff_ptr) {
    fprintf(stderr, "bind_object::Non-valid ddVAOData pointer.\n");
    return false;
  }
  // Bind valid mesh and instance buffer pointer objects to vertex array
  glBindVertexArray(vbuff_ptr->vao_handle);

  // mesh buffer
  if (mbuff_ptr) {
    glBindBuffer(GL_ARRAY_BUFFER, mbuff_ptr->vertex_buffer);
    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)0);
    // normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, normal));
    // texture coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, texCoords));
    // tangent normals
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, tangent));
    // blend weights
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, blendweight));
    // joint indices
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLvoid *)offsetof(Vertex, joints));
    // indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mbuff_ptr->index_buffer);

    if (gl_error("bind_object::mesh buffers")) return false;
  }

  // instance buffer
  if (ibuff_ptr) {
    // Set attribute pointers for matrix (4 times vec4)
    glBindBuffer(GL_ARRAY_BUFFER, ibuff_ptr->instance_buffer);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)0);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)(sizeof(glm::vec4)));
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)(3 * sizeof(glm::vec4)));
    // colors
    glBindBuffer(GL_ARRAY_BUFFER, ibuff_ptr->color_instance_buffer);
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          (GLvoid *)0);
    // set divisors for instanced render (1 means process every index per inst)
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glVertexAttribDivisor(9, 1);
    glVertexAttribDivisor(10, 1);

    if (gl_error("bind_object::instance buffers")) return false;
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  return true;
}

void destroy_storage_buffer(ddStorageBufferData *&sbuff_ptr) {
  // destroy buffer and destroy buffer object
  if (!sbuff_ptr) return;

  glDeleteBuffers(1, &sbuff_ptr->storage_handle);

  gl_error("destroy_storage_buffer");

  delete sbuff_ptr;
  sbuff_ptr = nullptr;
}

bool create_storage_buffer(ddStorageBufferData *&sbuff_ptr,
                           const unsigned byte_size) {
  if (sbuff_ptr) destroy_storage_buffer(sbuff_ptr);

  // create shader storage buffer object
  GLuint ssbo = 0;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, byte_size, NULL, GL_DYNAMIC_DRAW);
  if (gl_error("create_storage_buffer")) return false;

  sbuff_ptr = new ddStorageBufferData();
  if (!sbuff_ptr) {
    fprintf(stderr, "Failed to create ddStorageBufferData object in RAM\n");
    return false;
  }
  sbuff_ptr->storage_handle = ssbo;

  return true;
}

void destroy_index_buffer(ddIndexBufferData *&ebuff_ptr) {
  // destroy buffer and destroy buffer object
  if (!ebuff_ptr) return;

  glDeleteBuffers(1, &ebuff_ptr->index_handle);

  gl_error("destroy_index_buffer");

  delete ebuff_ptr;
  ebuff_ptr = nullptr;
}

bool create_index_buffer(ddIndexBufferData *&ebuff_ptr,
                         const unsigned byte_size, const void *data) {
  if (ebuff_ptr) destroy_index_buffer(ebuff_ptr);

  // create index buffer object
  GLuint ebo = 0;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_size, data, GL_STATIC_DRAW);
  if (gl_error("create_index_buffer")) return false;

  ebuff_ptr = new ddIndexBufferData();
  if (!ebuff_ptr) {
    fprintf(stderr, "Failed to create ddIndexBufferData object in RAM\n");
    return false;
  }
  ebuff_ptr->index_handle = ebo;

  return true;
}

bool extract_storage_buffer_data(ddStorageBufferData *sbuff_ptr,
                                 const unsigned buff_size_bytes,
                                 void *data_storage) {
  // check that ssbo is valid
  if (!sbuff_ptr) return false;

  // extract data
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbuff_ptr->storage_handle);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buff_size_bytes,
                     data_storage);
  POW2_VERIFY(!gl_error("extract_storage_buffer_data"));
  return true;
}

void bind_storage_buffer(const unsigned location,
                         const ddStorageBufferData *sbuff_ptr) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location,
                   sbuff_ptr->storage_handle);
}

void set_instance_buffer_contents(const ddInstBufferData *ibuff_ptr,
                                  bool inst_col, const unsigned byte_size,
                                  const unsigned offset, void *data) {
  POW2_VERIFY_MSG(ibuff_ptr != nullptr, "Instance buffer is null", 0);

  if (!inst_col) {
    // fill in instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, ibuff_ptr->instance_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, byte_size, data);
    POW2_VERIFY_MSG(!gl_error("set_instance_buffer_contents"),
                    "Instance buffer not set", 0);
  } else {
    // fill in color buffer
    glBindBuffer(GL_ARRAY_BUFFER, ibuff_ptr->color_instance_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, byte_size, data);
    POW2_VERIFY_MSG(!gl_error("set_instance_buffer_contents"),
                    "Color buffer not set", 0);
  }
}

void set_storage_buffer_contents(const ddStorageBufferData *sbuff_ptr,
                                 const unsigned byte_size,
                                 const unsigned offset, void *data) {
  POW2_VERIFY_MSG(sbuff_ptr != nullptr, "Storage buffer is null", 0);

  // Fill in shader storage buffer object
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbuff_ptr->storage_handle);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, byte_size, data);
  POW2_VERIFY_MSG(!gl_error("set_storage_buffer_contents"), "SSBO not set", 0);
}

void bind_storage_buffer_atrribute(const ddVAOData *vao,
                                   const ddStorageBufferData *sbuff_ptr,
                                   ddAttribPrimitive type,
                                   const unsigned attrib_loc,
                                   const unsigned num_attribs_of_type,
                                   const unsigned stride_in_bytes,
                                   const unsigned offset_in_stride) {
  POW2_VERIFY_MSG(vao != nullptr, "bind_attribute::Null VAO object", 0);
  POW2_VERIFY_MSG(sbuff_ptr != nullptr, "bind_attribute::Null storage buffer",
                  0);

  // bind buffer, set attributes, then draw points
  glBindVertexArray(vao->vao_handle);
  glBindBuffer(GL_ARRAY_BUFFER, sbuff_ptr->storage_handle);

  glEnableVertexAttribArray(attrib_loc);
  glVertexAttribPointer(attrib_loc, num_attribs_of_type,
                        attrib_type_ogl[(int)type], GL_FALSE, stride_in_bytes,
                        (GLvoid *)offset_in_stride);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void bind_index_buffer(const ddVAOData *vao,
                       const ddIndexBufferData *ebuff_ptr) {
  POW2_VERIFY_MSG(vao != nullptr, "bind_index_buffer::Null VAO object", 0);
  POW2_VERIFY_MSG(ebuff_ptr != nullptr, "bind_index_buffer::Null index buffer",
                  0);

  glBindVertexArray(vao->vao_handle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebuff_ptr->index_handle);

  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//*****************************************************************************

void render_quad() {
  if (quad_vao == 0) {
    // create quad VAO for rendering
    GLfloat quad_verts[] = {
        // Positions (3) & Texture Coords (2)
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // set up Plane VAO for post processing shaderUntitled-1
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    POW2_VERIFY_MSG(!gl_error("render_quad"), "Error generating quad buffers",
                    0);

    // fill vbo
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts,
                 GL_STATIC_DRAW);
    POW2_VERIFY_MSG(!gl_error("render_quad"), "Error sending data to buffers",
                    0);

    // bind attributes to VAO
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid *)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid *)(3 * sizeof(GLfloat)));
    POW2_VERIFY_MSG(!gl_error("render_quad"), "Error binding attributes", 0);
  }
  // draw
  glBindVertexArray(quad_vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void render_split_quad(bool leftside) {
  if (vr_vao[0] == 0) {
    // create split quad vao for rendering
    GLfloat vr_left[] = {
        // Positions (3) & Texture Coords (2)
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    GLfloat vr_right[] = {
        // Positions (3) & Texture Coords (2)
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup 2 plane vao's
    glGenVertexArrays(2, vr_vao);
    glGenBuffers(2, vr_vbo);
    POW2_VERIFY_MSG(!gl_error("render_split_quad"),
                    "Error generating quad buffers", 0);

    for (unsigned i = 0; i < 2; i++) {
      // fill i-th vbo & set attributes in vao
      glBindVertexArray(vr_vao[i]);
      glBindBuffer(GL_ARRAY_BUFFER, vr_vbo[i]);

      GLfloat *data = (i == 0) ? vr_left : vr_right;

      glBufferData(GL_ARRAY_BUFFER, sizeof(vr_left), data, GL_STATIC_DRAW);
      POW2_VERIFY_MSG(!gl_error("render_split_quad"),
                      "Error sending data to buffers <%u>", i);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                            (GLvoid *)0);
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                            (GLvoid *)(3 * sizeof(GLfloat)));
      POW2_VERIFY_MSG(!gl_error("render_split_quad"),
                      "Error binding attributes <%u>", i);
    }
    // draw
    if (leftside) {
      glBindVertexArray(vr_vao[0]);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
      glBindVertexArray(vr_vao[1]);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glBindVertexArray(0);
  }
}

void render_cube() {
  if (cube_vao == 0) {
    GLfloat box_verts[] = {
        // Positions (3)
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
    // setup cube vao for rendering
    glGenVertexArrays(1, &cube_vao);
    glGenBuffers(1, &cube_vbo);
    POW2_VERIFY_MSG(!gl_error("render_cube"), "Error generating cube buffers",
                    0);

    // fill vbo
    glBindVertexArray(cube_vao);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box_verts), box_verts, GL_STATIC_DRAW);
    POW2_VERIFY_MSG(!gl_error("render_cube"), "Error sending data to buffer",
                    0);

    // set position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                          (GLvoid *)0);
  }
  // draw
  glBindVertexArray(cube_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

void create_gbuffer(const int width, const int height) {
  // create and bind fbo
  glGenFramebuffers(1, &g_buff.deffered_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, g_buff.deffered_fbo);

  // create depth/stencil buffer
  glGenRenderbuffers(1, &g_buff.depth_buf);
  glBindRenderbuffer(GL_RENDERBUFFER, g_buff.depth_buf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

  // position, normal, color textures
  create_texture2D(GL_RGBA16F, g_buff.pos_tex, width, height);
  create_texture2D(GL_RGBA16F, g_buff.norm_tex, width, height);
  create_texture2D(GL_RGBA16F, g_buff.color_tex, width, height);

  // attach images and depth to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, g_buff.depth_buf);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         g_buff.pos_tex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         g_buff.norm_tex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                         g_buff.color_tex, 0);
  GLenum drawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                          GL_COLOR_ATTACHMENT2};
  glDrawBuffers(4, drawBuffers);

  // check for errors
  GLenum success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE,
                  "GBuffer creation failure", 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_lbuffer(const int width, const int height) {
  // create and bind fbo
  glGenFramebuffers(1, &l_buff.light_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, l_buff.light_fbo);

  // create depth/stencil buffer
  glGenRenderbuffers(1, &l_buff.depth_buf);
  glBindRenderbuffer(GL_RENDERBUFFER, l_buff.depth_buf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

  // color texture
  create_texture2D(GL_RGBA16F, l_buff.color_tex, width, height);

  // attach image and depth to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, l_buff.depth_buf);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         l_buff.color_tex, 0);
  GLenum drawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT0};
  glDrawBuffers(2, drawBuffers);

  // check for errors
  GLenum success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE,
                  "LBuffer creation failure", 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_sbuffer(const int width, const int height) {
  // create and bind fbo
  s_buff.width = width;
  s_buff.height = height;
  glm::vec4 border = glm::vec4(1.f);
  glGenFramebuffers(1, &s_buff.shadow_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, s_buff.shadow_fbo);

  // create depth32 buffer
  glGenRenderbuffers(1, &s_buff.depth_buf);
  glBindRenderbuffer(GL_RENDERBUFFER, s_buff.depth_buf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

  // color texture
  create_texture2D(GL_RG32F, s_buff.shadow_tex, width, height,
                   GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
                   GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, border, true);

  // attach image and depth to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, s_buff.depth_buf);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         s_buff.shadow_tex, 0);
  GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBuffers);

  // check for errors
  GLenum success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE,
                  "SBuffer creation failure", 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_pbuffer(const int width, const int height) {
  // create and bind fbo
  glGenFramebuffers(1, &p_buff.particle_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, p_buff.particle_fbo);

  // create depth/stencil buffer
  glGenRenderbuffers(1, &p_buff.depth_buf);
  glBindRenderbuffer(GL_RENDERBUFFER, p_buff.depth_buf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

  // color texture2
  create_texture2D(GL_RGBA16F, p_buff.color_tex, width, height);
  create_texture2D(GL_RGBA16F, p_buff.xtra_tex, width, height);

  // attach image and depth to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, p_buff.depth_buf);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         p_buff.color_tex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         p_buff.xtra_tex, 0);

  GLenum drawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(3, drawBuffers);

  // check for errors
  GLenum success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE,
                  "PBuffer creation failure", 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_cbuffer(const int width, const int height) {
  // create and bind fbo
  glGenFramebuffers(1, &c_buff.cube_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, c_buff.cube_fbo);

  // create empty cube map texture for rendering to
  c_buff.cube_texture.width = width;
  c_buff.cube_texture.height = height;
  generate_textureCube_RGBA8_LR(c_buff.cube_texture, true);

  // create depth/stencil buffer
  glGenRenderbuffers(1, &c_buff.depth_buf);
  glBindRenderbuffer(GL_RENDERBUFFER, c_buff.depth_buf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

  // attach depth to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, c_buff.depth_buf);
  GLenum drawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT0};
  glDrawBuffers(2, drawBuffers);

  // check for errors
  GLenum success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE,
                  "CBuffer creation failure", 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_fbuffer(const int c_width, const int c_height, const int s_width,
                    const int s_height) {
  glm::vec4 border = glm::vec4(1.f);
  // create and bind color fbo
  glGenFramebuffers(1, &f_buff.color_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, f_buff.color_fbo);

  // color texture
  create_texture2D(GL_RGBA16F, f_buff.color_tex, c_width, c_height);

  // attach image to color framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         f_buff.color_tex, 0);
  GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBuffers);

  // check for errors
  GLenum success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE, "FBuffer::color failure",
                  0);

  // create and bind shadow fbo
  glGenFramebuffers(1, &f_buff.shadow_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, f_buff.shadow_fbo);

  // depth texture
  create_texture2D(GL_RG32F, f_buff.shadow_tex, s_width, s_height,
                   GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER,
                   GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, border, true);

  // attach image to shadow framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         f_buff.shadow_tex, 0);
  GLenum drawBuffers2[] = {GL_NONE, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, drawBuffers2);

  // check for errors
  success = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  POW2_VERIFY_MSG(success == GL_FRAMEBUFFER_COMPLETE, "FBuffer::shadow failure",
                  0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void bind_framebuffer(const ddBufferType type, const bool color_shadow) {
  switch (type) {
    case ddBufferType::GEOM:
      glBindFramebuffer(GL_FRAMEBUFFER, g_buff.deffered_fbo);
      break;
    case ddBufferType::LIGHT:
      glBindFramebuffer(GL_FRAMEBUFFER, l_buff.light_fbo);
      break;
    case ddBufferType::SHADOW:
      glBindFramebuffer(GL_FRAMEBUFFER, s_buff.shadow_fbo);
      break;
    case ddBufferType::PARTICLE:
      glBindFramebuffer(GL_FRAMEBUFFER, p_buff.particle_fbo);
      break;
    case ddBufferType::CUBE:
      glBindFramebuffer(GL_FRAMEBUFFER, c_buff.cube_fbo);
      break;
    case ddBufferType::FILTER:
      if (color_shadow) {
        glBindFramebuffer(GL_FRAMEBUFFER, f_buff.color_fbo);
      } else {
        glBindFramebuffer(GL_FRAMEBUFFER, f_buff.shadow_fbo);
      }
      break;
    case ddBufferType::DEFAULT:
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      break;
    default:
      POW2_VERIFY_MSG(false, "Error binding frame buffer <%u>", (unsigned)type);
      break;
  }
}

void bind_cube_target_for_render(GLenum target) {
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                         c_buff.cube_texture.tex_buff->texture_handle, 0);
  POW2_VERIFY_MSG(!gl_error("render_to_cube_map"), "Error writing cube map %u",
                  (unsigned)target);
}

void bind_pass_texture(const ddBufferType type, const unsigned loc,
                       const unsigned xtra_param) {
  switch (type) {
    case ddBufferType::GEOM:
      // position (shader location 0)
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, g_buff.pos_tex);
      // color (shader location 1)
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, g_buff.color_tex);
      // normal (shader location 2)
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, g_buff.norm_tex);
      break;
    case ddBufferType::LIGHT:
      // color (shader location 0)
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, l_buff.color_tex);
      break;
    case ddBufferType::SHADOW:
      if (xtra_param == 0) {
        // light pass (probably shader location 3)
        glActiveTexture(GL_TEXTURE0 + loc);
        glBindTexture(GL_TEXTURE_2D, s_buff.shadow_tex);
      } else {
        // generic pass (shader location 0)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_buff.shadow_tex);
      }
      break;
    case ddBufferType::PARTICLE:
      if (xtra_param == 0) {
        // color (shader location 1)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, p_buff.color_tex);
      } else {
        // color (shader location 0)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, p_buff.xtra_tex);
      }
      break;
    case ddBufferType::CUBE:
      // bind framebuffer texture before draw
      switch ((CubeMapFaces)xtra_param) {
        case CubeMapFaces::RIGHT:
          bind_cube_target_for_render(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
          break;
        case CubeMapFaces::LEFT:
          bind_cube_target_for_render(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
          break;
        case CubeMapFaces::TOP:
          bind_cube_target_for_render(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
          break;
        case CubeMapFaces::BOTTOM:
          bind_cube_target_for_render(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
          break;
        case CubeMapFaces::BACK:
          bind_cube_target_for_render(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
          break;
        case CubeMapFaces::FRONT:
          bind_cube_target_for_render(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
          break;
        default:
          POW2_VERIFY_MSG(false, "Invalid cube map face to render to <%u>",
                          xtra_param);
          break;
      }
      break;
    case ddBufferType::FILTER:
      // make texture 0 active (texture to run algorithm on)
      glActiveTexture(GL_TEXTURE0);
      break;
    case ddBufferType::DEFAULT:
      glBindTexture(GL_TEXTURE_2D, 0);
      break;
    default:
      POW2_VERIFY_MSG(false, "Error binding buffer textures <%u>",
                      (unsigned)type);
      break;
  }
}

void blit_depth_buffer(const ddBufferType in_type, const ddBufferType out_type,
                       const unsigned width, const unsigned height) {
  // set buffer in & out
  switch (in_type) {
    case ddBufferType::GEOM:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buff.deffered_fbo);
      break;
    case ddBufferType::LIGHT:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, l_buff.light_fbo);
      break;
    case ddBufferType::SHADOW:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, s_buff.shadow_fbo);
      break;
    case ddBufferType::PARTICLE:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, p_buff.particle_fbo);
      break;
    case ddBufferType::CUBE:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, c_buff.cube_fbo);
      break;
    case ddBufferType::DEFAULT:
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      break;
    default:
      POW2_VERIFY_MSG(false, "Invalid in_buffer <%u>", (unsigned)in_type);
      break;
  }
  switch (out_type) {
    case ddBufferType::GEOM:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_buff.deffered_fbo);
      break;
    case ddBufferType::LIGHT:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_buff.light_fbo);
      break;
    case ddBufferType::SHADOW:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_buff.shadow_fbo);
      break;
    case ddBufferType::PARTICLE:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p_buff.particle_fbo);
      break;
    case ddBufferType::CUBE:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, c_buff.cube_fbo);
      break;
    case ddBufferType::DEFAULT:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      break;
    default:
      POW2_VERIFY_MSG(false, "Invalid out_buffer <%u>", (unsigned)out_type);
      break;
  }
  glBlitFramebuffer(0, 0, width, height, 0, 0, (GLint)width, (GLint)height,
                    GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  POW2_VERIFY_MSG(!gl_error("blit_depth"), "Error: blit", (unsigned)out_type);
}

float sample_depth_buffer(const ddBufferType type, const int pos_x,
                          const int pos_y) {
  switch (type) {
    case ddBufferType::GEOM:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_buff.deffered_fbo);
      break;
    case ddBufferType::LIGHT:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l_buff.light_fbo);
      break;
    case ddBufferType::SHADOW:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_buff.shadow_fbo);
      break;
    case ddBufferType::PARTICLE:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, p_buff.particle_fbo);
      break;
    case ddBufferType::CUBE:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, c_buff.cube_fbo);
      break;
    case ddBufferType::DEFAULT:
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      break;
    default:
      POW2_VERIFY_MSG(false, "Invalid out_buffer <%u>", (unsigned)type);
      break;
  }
  // get depth
  GLfloat _z = 0.f;
  glReadPixels(pos_x, pos_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &_z);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return _z;
}

void set_viewport(const unsigned width, const unsigned height) {
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void clear_color_buffer() { glClear(GL_COLOR_BUFFER_BIT); }

void clear_depth_buffer() { glClear(GL_DEPTH_BUFFER_BIT); }

void clear_stencil_buffer() { glClear(GL_STENCIL_BUFFER_BIT); }

void toggle_depth_mask(bool flag) {
  if (flag) {
    // mask on
    glDepthMask(GL_TRUE);
  } else {
    // mask off
    glDepthMask(GL_FALSE);
  }
}

void toggle_depth_test(bool flag) {
  if (flag) {
    // test on
    glEnable(GL_DEPTH_TEST);
  } else {
    // test off
    glDisable(GL_DEPTH_TEST);
  }
}

void toggle_stencil_test(bool flag) {
  if (flag) {
    // test on
    glEnable(GL_STENCIL_TEST);
  } else {
    // test off
    glDisable(GL_STENCIL_TEST);
  }
}

void toggle_additive_blend(bool flag) {
  if (flag) {
    // additive blend on
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
  } else {
    // disble blending
    glDisable(GL_BLEND);
  }
}

void toggle_alpha_blend(bool flag) {
  if (flag) {
    // additive blend on
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    // disble blending
    glDisable(GL_BLEND);
  }
}

void toggle_wireframe(bool flag) {
  if (flag) {
    // wireframe on
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    // disble wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

void set_depth_mode(const DepthMode mode) {
  switch (mode) {
    case DepthMode::LESS:
      glDepthFunc(GL_LESS);
      break;
    case DepthMode::LESS_OR_EQUAL:
      glDepthFunc(GL_LEQUAL);
      break;
    case DepthMode::GREATER:
      glDepthFunc(GL_GREATER);
      break;
    default:
      POW2_VERIFY_MSG(false, "Invalid DepthMode provided <%u>", (unsigned)mode);
      break;
  }
}

void toggle_face_cull(bool flag) {
  if (flag) {
    // cull on
    glEnable(GL_CULL_FACE);
  } else {
    // cull off
    glDisable(GL_CULL_FACE);
  }
}

void set_face_cull(const bool backface) {
  if (backface) {
    glCullFace(GL_BACK);
  } else {
    glCullFace(GL_FRONT);
  }
}

void toggle_clip_plane(bool flag) {
  if (flag) {
    // clip on
    glEnable(GL_CLIP_DISTANCE0);
  } else {
    // clip off
    glDisable(GL_CLIP_DISTANCE0);
  }
}

void bind_texture(const unsigned location, ddTextureData *tex_data) {
  glActiveTexture(GL_TEXTURE0 + location);
  glBindTexture(GL_TEXTURE_2D, tex_data->texture_handle);
}

void draw_instanced_vao(const ddVAOData *vao, const unsigned num_indices,
                        const unsigned instance_size) {
  POW2_VERIFY_MSG(vao != nullptr, "draw_instanced_vao::Null vao provided", 0);

  // bind vao and draw
  glBindVertexArray(vao->vao_handle);
  glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)num_indices, GL_UNSIGNED_INT,
                          0, instance_size);
  glBindVertexArray(0);
}

void draw_indexed_vao(const ddVAOData *vao, const unsigned num_indices,
                      const unsigned offset_in_index_buffer) {
  POW2_VERIFY_MSG(vao != nullptr, "draw_indexed_vao::Null vao provided", 0);

  // bind vao and draw
  glBindVertexArray(vao->vao_handle);
  glDrawElements(GL_TRIANGLES, (GLsizei)num_indices, GL_UNSIGNED_INT,
                 (GLvoid *)offset_in_index_buffer);
  glBindVertexArray(0);
}

void draw_indexed_lines_vao(const ddVAOData *vao, const unsigned num_indices,
                            const unsigned offset_in_index_buffer) {
  POW2_VERIFY_MSG(vao != nullptr, "draw_indexed_vao::Null vao provided", 0);

  // bind vao and draw
  glBindVertexArray(vao->vao_handle);
  glDrawElements(GL_LINES, (GLsizei)num_indices, GL_UNSIGNED_INT,
                 (GLvoid *)offset_in_index_buffer);
  glBindVertexArray(0);
}

void draw_points(const ddVAOData *vao, const ddStorageBufferData *sbuff_ptr,
                 ddAttribPrimitive type, const unsigned attrib_loc,
                 const unsigned num_attribs, const unsigned stride,
                 const unsigned offset_in_stride,
                 const unsigned offset_in_buffer, const unsigned num_points) {
  POW2_VERIFY_MSG(vao != nullptr, "draw_points::Null VAO object", 0);
  POW2_VERIFY_MSG(sbuff_ptr != nullptr, "draw_points::Null storage buffer", 0);

  const int stride_offset = (offset_in_stride * attrib_size_ogl[(int)type]);

  // bind buffer, set attributes, then draw points
  glBindVertexArray(vao->vao_handle);
  glBindBuffer(GL_ARRAY_BUFFER, sbuff_ptr->storage_handle);

  glEnableVertexAttribArray(attrib_loc);
  glVertexAttribPointer(attrib_loc, num_attribs, attrib_type_ogl[(int)type],
                        GL_FALSE, stride * attrib_size_ogl[(int)type],
                        (GLvoid *)stride_offset);

  glDrawArrays(GL_POINTS, offset_in_buffer, num_points);

  glBindVertexArray(0);
  POW2_VERIFY_MSG(!gl_error("draw_points"), "draw_points::error drawing points",
                  0);
}

void deploy_compute_task(const unsigned x_work_groups,
                         const unsigned y_work_groups,
                         const unsigned z_work_groups) {
  glDispatchCompute((GLuint)x_work_groups, (GLuint)y_work_groups,
                    (GLuint)z_work_groups);
  glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

}  // namespace ddGPUFrontEnd
