#include <cstdio>
#include "../source/gl_core_4_3.h"
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

bool load_buffer_data(ddMeshBufferData *&buff_ptr, DDM_Data *ddm_ptr) {
  // Create ddMeshBufferData and load gpu w/ buffer data
  if (buff_ptr) destroy_buffer_data(buff_ptr);

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

  buff_ptr = new ddMeshBufferData();
  if (!buff_ptr) {
    fprintf(stderr, "Failed to create ddMeshBufferData object in RAM\n");
    return false;
  }
  buff_ptr->vertex_buffer = vbo;
  buff_ptr->index_buffer = ebo;

  return true;
}

void destroy_buffer_data(ddMeshBufferData *&buff_ptr) {
  // Delete initialized data and destroy buffer object
  if (!buff_ptr) return;

  glDeleteBuffers(1, &buff_ptr->vertex_buffer);
  glDeleteBuffers(1, &buff_ptr->index_buffer);

  check_gl_errors("destroy_buffer_data");

  delete buff_ptr;
  buff_ptr = nullptr;
}

}  // namespace ddGPUFrontEnd
