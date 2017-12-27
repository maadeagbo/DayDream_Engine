#include "GPUFrontEnd.h"
#include <cstdio>
#include "../source/gl_core_4_3.h"

namespace DD_GPUFrontEnd {
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

}
