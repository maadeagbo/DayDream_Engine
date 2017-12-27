#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Shader:
*		- creates shader program from vertex and fragment shader
*			-
*		- Isolates OpenGL calls
*
*	TODO:
*
*	Uses POW_ASSERT
-----------------------------------------------------------------------------*/

#include <Pow2Assert.h>
#include "DD_Types.h"

class DD_Shader {
 public:
  // Generates the shader program
  DD_Shader();
  ~DD_Shader();

  inline void init() {
    m_Program = glCreateProgram();
    POW2_VERIFY_MSG(m_Program > 0, "Error creating program object.", 0);
  }
  void CreateVertexShader(const char* filePath);
  void CreateFragShader(const char* filePath);
  void CreateComputeShader(const char* filePath);
  void CreateGeomShader(const char* filePath);
  void QueryShaderAttributes();
  void QueryUniforms();
  void CompileUberShader(GLenum Type, const char* version, const char* defines,
                         const char* file);
  void Use();
  GLuint GetHandle();
  void setUniform(const char* name, const GLint data);
  void setUniform(const char* name, const GLfloat data);
  void setUniform(const char* name, const GLboolean flag);
  void setUniform(const char* name, const glm::vec2& data);
  void setUniform(const char* name, const glm::vec3& data);
  void setUniform(const char* name, const glm::vec4& data);
  void setUniform(const char* name, const glm::mat3& data);
  void setUniform(const char* name, const glm::mat4& data);

  std::string m_ID = "", m_vs = "", m_fs = "", m_gs = "", m_cs = "";

 private:
  GLuint m_Program;
  std::map<std::string, int> m_uniformLocations;
  int getUniformLocation(const char* name);
  std::string ReadShaderFile(const char* filePath);
  bool CompileShader(GLuint* shader, const GLchar* shaderCode,
                     const char* shaderType);
  void LinkShaderToProgram(GLuint* shader, const char* shaderType);
};
