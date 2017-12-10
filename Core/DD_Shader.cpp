#include "DD_Shader.h"
#include <fstream>
#include <sstream>

namespace {
// Return type of glsl attribute
const char* getTypeString(GLenum type) {
  switch (type) {
    case GL_FLOAT:
      return "float";
    case GL_FLOAT_VEC2:
      return "vec2";
    case GL_FLOAT_VEC3:
      return "vec3";
    case GL_FLOAT_VEC4:
      return "vec4";
    case GL_DOUBLE:
      return "double";
    case GL_INT:
      return "int";
    case GL_UNSIGNED_INT:
      return "unsigned int";
    case GL_BOOL:
      return "bool";
    case GL_FLOAT_MAT2:
      return "mat2";
    case GL_FLOAT_MAT3:
      return "mat3";
    case GL_FLOAT_MAT4:
      return "mat4";
    case GL_SAMPLER_2D:
      return "sampler2D";
    default:
      return "?";
  }
}
}

DD_Shader::DD_Shader() : m_Program(0) {}

DD_Shader::~DD_Shader() {
  if (m_Program == 0) return;

  // Query the number of attached shaders
  GLint numShaders = 0;
  glGetProgramiv(m_Program, GL_ATTACHED_SHADERS, &numShaders);

  // Get the shader names
  GLuint* shaderNames = new GLuint[numShaders];
  glGetAttachedShaders(m_Program, numShaders, NULL, shaderNames);

  // Delete the shaders
  for (int i = 0; i < numShaders; i++) {
    glDeleteShader(shaderNames[i]);
  }

  // Delete the program
  glDeleteProgram(m_Program);

  delete[] shaderNames;
}

void DD_Shader::CreateVertexShader(const char* filePath) {
  std::string shader_code = (ReadShaderFile(filePath)).c_str();
  GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
  POW2_VERIFY_MSG(vertShader > 0, "Error creating vertex shader.", 0);

  // compile and link shader
  if (CompileShader(&vertShader, shader_code.c_str(), "VERTEX")) {
    LinkShaderToProgram(&vertShader, "VERTEX");
  }
}

void DD_Shader::CreateFragShader(const char* filePath) {
  std::string shader_code = (ReadShaderFile(filePath)).c_str();
  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  POW2_VERIFY_MSG(fragShader > 0, "Error creating fragement shader.", 0);

  // compile and link shader
  if (CompileShader(&fragShader, shader_code.c_str(), "FRAGMENT")) {
    LinkShaderToProgram(&fragShader, "FRAGMENT");
  }
}

void DD_Shader::CreateComputeShader(const char* filePath) {
  std::string shader_code = (ReadShaderFile(filePath)).c_str();
  GLuint compShader = glCreateShader(GL_COMPUTE_SHADER);
  POW2_VERIFY_MSG(compShader > 0, "Error creating compute shader.", 0);

  // compile and link shader
  if (CompileShader(&compShader, shader_code.c_str(), "COMPUTE")) {
    LinkShaderToProgram(&compShader, "COMPUTE");
  }
}

void DD_Shader::CreateGeomShader(const char* filePath) {
  std::string shader_code = (ReadShaderFile(filePath)).c_str();
  GLuint geomShader = glCreateShader(GL_GEOMETRY_SHADER);
  POW2_VERIFY_MSG(geomShader > 0, "Error creating geometry shader.", 0);

  // compile and link shader
  if (CompileShader(&geomShader, shader_code.c_str(), "GEOMETRY")) {
    LinkShaderToProgram(&geomShader, "GEOMETRY");
  }
}

// Retrieve the attributes from the shader program
void DD_Shader::QueryShaderAttributes() {
  POW2_VERIFY_MSG(m_Program > 0, "Program has not been compiled", 0);
  GLint numAttribs;
  glGetProgramInterfaceiv(m_Program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES,
                          &numAttribs);

  // loop through attributes
  GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};
  printf("Active attributes:\n");
  for (int i = 0; i < numAttribs; ++i) {
    GLint results[3];
    glGetProgramResourceiv(m_Program, GL_PROGRAM_INPUT, i, 3, properties, 3,
                           NULL, results);

    GLint nameBufSize = results[0] + 1;
    char* name = new char[nameBufSize];
    glGetProgramResourceName(m_Program, GL_PROGRAM_INPUT, i, nameBufSize, NULL,
                             name);
    printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
    delete[] name;
  }
}

// Retrieve the uniforms from the shader program
void DD_Shader::QueryUniforms() {
  POW2_VERIFY_MSG(m_Program > 0, "Program has not been compiled", 0);
  GLint numUniforms;
  glGetProgramInterfaceiv(m_Program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &numUniforms);

  // loop thru uniforms
  GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};
  printf("Active uniforms:\n");
  for (int i = 0; i < numUniforms; ++i) {
    GLint results[4];
    glGetProgramResourceiv(m_Program, GL_UNIFORM, i, 4, properties, 4, NULL,
                           results);

    GLint nameBufSize = results[0] + 1;
    char* name = new char[nameBufSize];
    glGetProgramResourceName(m_Program, GL_UNIFORM, i, nameBufSize, NULL, name);
    printf("%-5d %s (%s)\n", results[2], name, getTypeString(results[1]));
    delete[] name;
  }
}

// Create all-purpose/specific shader with pre-processor defines
void DD_Shader::CompileUberShader(GLenum Type, const char* version,
                                  const char* defines, const char* file) {
  std::string shader_code = (ReadShaderFile(file)).c_str();
  std::string versionDirective = version;
  shader_code = versionDirective + defines + shader_code;

  std::string type;
  if (Type == GL_FRAGMENT_SHADER) {
    type = "FRAGMENT";
  } else if (Type == GL_VERTEX_SHADER) {
    type = "VERTEX";
  } else if (Type == GL_COMPUTE_SHADER) {
    type = "COMPUTE";
  }

  GLuint shader = glCreateShader(Type);
  POW2_VERIFY_MSG(shader > 0, "Error creating uber shader.", 0);

  // compile and link shader
  if (CompileShader(&shader, shader_code.c_str(), type.c_str())) {
    LinkShaderToProgram(&shader, type.c_str());
  }
}

// Uses the current shader program
void DD_Shader::Use() {
  POW2_VERIFY_MSG(m_Program > 0, "Program has not been compiled", 0);
  glUseProgram(m_Program);
}

GLuint DD_Shader::GetHandle() {
  POW2_VERIFY_MSG(m_Program > 0, "Program has not been compiled", 0);
  return m_Program;
}

void DD_Shader::setUniform(const char* name, const GLint data) {
  GLint loc = getUniformLocation(name);
  glUniform1i(loc, data);
}

void DD_Shader::setUniform(const char* name, const GLfloat data) {
  GLint loc = getUniformLocation(name);
  glUniform1f(loc, data);
}

void DD_Shader::setUniform(const char* name, const GLboolean flag) {
  GLint loc = getUniformLocation(name);
  glUniform1i(loc, flag);
}

void DD_Shader::setUniform(const char* name, const glm::vec2& data) {
  GLint loc = getUniformLocation(name);
  glUniform2f(loc, data.x, data.y);
}

void DD_Shader::setUniform(const char* name, const glm::vec3& data) {
  GLint loc = getUniformLocation(name);
  glUniform3f(loc, data.x, data.y, data.z);
}

void DD_Shader::setUniform(const char* name, const glm::vec4& data) {
  GLint loc = getUniformLocation(name);
  glUniform4f(loc, data.x, data.y, data.z, data.w);
}

void DD_Shader::setUniform(const char* name, const glm::mat3& data) {
  GLint loc = getUniformLocation(name);
  glUniformMatrix4fv(loc, 1, GL_FALSE, &data[0][0]);
}

void DD_Shader::setUniform(const char* name, const glm::mat4& data) {
  GLint loc = getUniformLocation(name);
  glUniformMatrix4fv(loc, 1, GL_FALSE, &data[0][0]);
}

int DD_Shader::getUniformLocation(const char* name) {
  std::map<std::string, int>::iterator pos;
  pos = m_uniformLocations.find(name);

  // if its not in the list, add the new uniform
  if (pos == m_uniformLocations.end()) {
    m_uniformLocations[name] = glGetUniformLocation(m_Program, name);
  }

  return m_uniformLocations[name];
}

// Read in a file and return the contents in a char array
std::string DD_Shader::ReadShaderFile(const char* filePath) {
  std::ifstream file(filePath);
  if (file.is_open()) {
    std::stringstream sstr;
    sstr << file.rdbuf();
    file.close();
    return sstr.str();
  }
  fprintf(stderr, "Could not open \"%s\"\n", filePath);
  std::string msg = "Could not open " + std::string(filePath);
  POW2_VERIFY_MSG(false, msg.c_str(), 0);
  return "";
}

bool DD_Shader::CompileShader(GLuint* shader, const GLchar* shaderCode,
                              const char* shaderType) {
  // load shader with one or more sources
  const GLchar* codeArray[] = {shaderCode};
  glShaderSource(*shader, 1, codeArray, NULL);

  glCompileShader(*shader);
  GLint result;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &result);
  if (GL_FALSE == result) {
    std::string logStr;
    GLint logLen;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 0) {
      char* log = new char[logLen];
      GLsizei written;
      glGetShaderInfoLog(*shader, logLen, &written, log);
      logStr = log;
      delete[] log;
    }
    std::string msg = std::string(shaderType) +
                      " shader compilation failed!\n" + "Shader log:\n" +
                      logStr;
    POW2_VERIFY_MSG(false, msg.c_str(), 0);
  }
  return true;
}

void DD_Shader::LinkShaderToProgram(GLuint* shader, const char* shaderType) {
  glAttachShader(m_Program, *shader);
  glLinkProgram(m_Program);

  GLint status;
  glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
  if (GL_FALSE == status) {
    std::string logStr;
    GLint logLen;
    glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 0) {
      char* log = new char[logLen];
      GLsizei written;
      glGetProgramInfoLog(m_Program, logLen, &written, log);
      logStr = log;
      delete[] log;
    }
    std::string msg = "Failed to link " + std::string(shaderType) +
                      " to program!\nProgram log:\n" + logStr;
    POW2_VERIFY_MSG(false, msg.c_str(), 0);
  }
}
