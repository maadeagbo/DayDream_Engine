#include "ddShader.h"
#include <Pow2Assert.h>
#include <string>
#include "gl_core_4_3.h"
#include "ddFileIO.h"

struct ddShaderHandle {
  GLuint program;
};

// Error processing function for OpenGL calls
bool check_gl_errors(const char* signature) {
  GLenum err;
  bool flag = false;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%s::OpenGL error: %d", signature, err);
    flag = true;
  }
  return flag;
}

#define UNIFORM_TO_STRING(x) #x

const char* get_uniform_type(unsigned type) {
  switch (type) {
    case GL_FLOAT:
      return UNIFORM_TO_STRING(GL_FLOAT);
    case GL_FLOAT_VEC2:
      return UNIFORM_TO_STRING(GL_FLOAT_VEC2);
    case GL_FLOAT_VEC3:
      return UNIFORM_TO_STRING(GL_FLOAT_VEC3);
    case GL_FLOAT_VEC4:
      return UNIFORM_TO_STRING(GL_FLOAT_VEC4);
    case GL_DOUBLE:
      return UNIFORM_TO_STRING(GL_DOUBLE);
    case GL_INT:
      return UNIFORM_TO_STRING(GL_INT);
    case GL_UNSIGNED_INT:
      return UNIFORM_TO_STRING(GL_UNSIGNED_INT);
    case GL_BOOL:
      return UNIFORM_TO_STRING(GL_BOOL);
    case GL_FLOAT_MAT2:
      return UNIFORM_TO_STRING(GL_FLOAT_MAT2);
    case GL_FLOAT_MAT3:
      return UNIFORM_TO_STRING(GL_FLOAT_MAT3);
    case GL_FLOAT_MAT4:
      return UNIFORM_TO_STRING(GL_FLOAT_MAT4);
    case GL_SAMPLER_2D:
      return UNIFORM_TO_STRING(GL_SAMPLER_2D);
    default:
      return UNIFORM_TO_STRING(GL_FLOAT);
  }
}

ddShader::~ddShader() {
  // delete shader and atached programs
  if (!handle) return;

  // Query the number of attached shaders
  GLint num_shaders = 0;
  glGetProgramiv(handle->program, GL_ATTACHED_SHADERS, &num_shaders);

  // Get the shader names
  GLuint* shader_handles = new GLuint[num_shaders];
  glGetAttachedShaders(handle->program, num_shaders, NULL, shader_handles);

  // Delete the shaders
  for (int i = 0; i < num_shaders; i++) {
    glDeleteShader(shader_handles[i]);
  }

  // Delete the program
  glDeleteProgram(handle->program);

  delete[] shader_handles;
}