#include <SOIL.h>
#include <cstdio>
#include "gl_core_4_3.h"
#include "GPUFrontEnd.h"

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

// Error processing function for OpenGL calls
bool check_gl_errors(const char *signature) {
  GLenum err;
  bool flag = false;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%s::OpenGL error: %d", signature, err);
    flag = true;
  }
  return flag;
}

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

void destroy_texture(ddTextureData *&tex_ptr) {
  if (!tex_ptr) return;

  glDeleteTextures(1, &tex_ptr->texture_handle);
  check_gl_errors("destroy_texture");

  delete tex_ptr;
  tex_ptr = nullptr;
}

bool generate_texture2D_RGBA8_LR(ImageInfo &img) {
  if (img.tex_buff) destroy_texture(img.tex_buff);

  // set up image parameters
  img.internal_format = GL_RGBA8;  // TODO: should set based on channels ?
  img.image_format = GL_RGBA8;     // TODO: should set based on channels ?
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
  glGenTextures(1, &img.tex_buff->texture_handle);
  if (check_gl_errors("generate_texture2D_RGBA8_LR::Creating texture")) {
    return false;
  }

  // transfer image to GPU then delete from RAM
  glBindTexture(GL_TEXTURE_2D, img.tex_buff->texture_handle);
  const int num_mipmaps = (int)floor(log2(std::max(img.width, img.height))) + 1;
  glTexStorage2D(GL_TEXTURE_2D, num_mipmaps, img.internal_format, img.width,
                 img.height);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.width, img.height,
                  img.image_format, GL_UNSIGNED_BYTE, img.image_data[0]);
  if (check_gl_errors("generate_texture2D_RGBA8_LR::Loading data")) {
    return false;
  }
  SOIL_free_image_data(img.image_data[0]);
  img.image_data[0] = nullptr;

  // texture settings
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, img.wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, img.wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, img.min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, img.mag_filter);
  glGenerateMipmap(GL_TEXTURE_2D);
  if (check_gl_errors("generate_texture2D_RGBA8_LR::Configuring settings")) {
    return false;
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

bool generate_textureCube_RGBA8_LR(ImageInfo &img, const bool empty) {
  if (img.tex_buff) destroy_texture(img.tex_buff);

  // set up image parameters
  img.internal_format = GL_RGBA8;  // TODO: should set based on channels ?
  img.image_format = GL_RGBA8;     // TODO: should set based on channels ?
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
  if (check_gl_errors("generate_textureCube_RGBA8_LR::Creating texture")) {
    return false;
  }
  glBindTexture(GL_TEXTURE_CUBE_MAP, img.tex_buff->texture_handle);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  // empty flag means generate dummy cubemap texture that will be rendered to
  if (empty) {
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, img.internal_format, img.width,
                   img.height);
    if (check_gl_errors("generate_textureCube_RGBA8_LR::Creating empty")) {
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
                      img.image_format, GL_UNSIGNED_BYTE, img.image_data[i]);
      SOIL_free_image_data(img.image_data[i]);
      img.image_data[i] = nullptr;

      err_msg.format("generate_textureCube_RGBA8_LR::Loading image <%d>", i);
      if (check_gl_errors(err_msg.str())) return false;
    }
  }

  // texture settings
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, img.wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, img.wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, img.wrap_r);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, img.min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, img.mag_filter);

  glBindTexture(GL_TEXTURE_2D, 0);
  return true;
}

bool load_buffer_data(ddMeshBufferData *&mbuff_ptr, DDM_Data *ddm_ptr) {
  // Create ddMeshBufferData and load gpu w/ buffer data
  if (mbuff_ptr) destroy_buffer_data(mbuff_ptr);

  // create and load: vertex and index buffer objects
  GLuint vbo = 0, ebo = 0;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  if (check_gl_errors("load_buffer_data::Creating buffers")) return false;

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, ddm_ptr->data.sizeInBytes(), &ddm_ptr->data[0],
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ddm_ptr->indices.sizeInBytes(),
               &ddm_ptr->indices[0], GL_STATIC_DRAW);

  if (check_gl_errors("load_buffer_data::Loading buffers")) return false;
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

  check_gl_errors("destroy_buffer_data");

  delete mbuff_ptr;
  mbuff_ptr = nullptr;
}

bool load_instance_data(ddInstBufferData *&ibuff_ptr, const int inst_size) {
  if (ibuff_ptr) destroy_instance_data(ibuff_ptr);

  // create dynamic buffer and set max data size
  GLuint inst_m4x4 = 0, inst_v3 = 0;
  glGenBuffers(1, &inst_m4x4);
  glGenBuffers(1, &inst_v3);
  if (check_gl_errors("load_instance_data::Creating buffers")) return false;

  glBindBuffer(GL_ARRAY_BUFFER, inst_m4x4);
  glBufferData(GL_ARRAY_BUFFER, inst_size * sizeof(glm::mat4), NULL,
               GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, inst_v3);
  glBufferData(GL_ARRAY_BUFFER, inst_size * sizeof(glm::vec3), NULL,
               GL_DYNAMIC_DRAW);

  if (check_gl_errors("load_buffer_data::Loading buffers")) return false;
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
  check_gl_errors("destroy_instance_data");

  delete ibuff_ptr;
  ibuff_ptr = nullptr;
}

bool create_vao(ddVAOData *&vbuff_ptr) {
  if (vbuff_ptr) destroy_vao(vbuff_ptr);

  // create vertex array object
  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  if (check_gl_errors("create_vao")) return false;

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
  check_gl_errors("destroy_vao");

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
    // indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mbuff_ptr->index_buffer);

    if (check_gl_errors("bind_object::mesh buffers")) return false;
  }

  // instance buffer
  if (ibuff_ptr) {
    // Set attribute pointers for matrix (4 times vec4)
    glBindBuffer(GL_ARRAY_BUFFER, ibuff_ptr->instance_buffer);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)0);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)(sizeof(glm::vec4)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (GLvoid *)(3 * sizeof(glm::vec4)));
    // colors
    glBindBuffer(GL_ARRAY_BUFFER, ibuff_ptr->color_instance_buffer);
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          (GLvoid *)0);
    // set divisors for instanced render (1 means process every index per inst)
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);

    if (check_gl_errors("bind_object::instance buffers")) return false;
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  return true;
}

}  // namespace ddGPUFrontEnd
