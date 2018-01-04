#include "ddShader.h"
#include <Pow2Assert.h>
#include <fstream>
#include <sstream>
#include <string>
#include "gl_core_4_3.h"

struct ddShaderHandle {
  GLuint program;
};

/// \brief Read file and return string holding its entire contents
/// \param filename
/// \return String of file contents
std::string read_shader_file(const char *filename);
/// \brief Compile shader and check for errors
/// \param s_handle Shader unsigned handle
/// \param s_code Shader code
/// \param s_type Shader type
/// \return True-> shader sucessfully compiled
bool compile_shader(GLuint *s_handle, const GLchar *s_code, const char *s_type);
/// \brief Link provided shader to program
/// \param s_handle Shader handle
/// \param s_type Shader type
/// \param program Shader program handle
void link_shader(GLuint *s_handle, const char *s_type, GLuint *program);

// Error processing function for OpenGL calls
namespace {
bool check_gl_errors(const char *signature) {
  GLenum err;
  bool flag = false;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%s::OpenGL error: %d\n", signature, err);
    flag = true;
  }
  return flag;
}

#define UNIFORM_TO_STRING(x) #x
}  // namespace

const char *get_uniform_type(unsigned type) {
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
      return UNIFORM_TO_STRING("NOT LISTED");
  }
}

ddShader::~ddShader() {
  // delete shader and atached programs
  if (!handle) return;

  // Query the number of attached shaders
  GLint num_shaders = 0;
  glGetProgramiv(handle->program, GL_ATTACHED_SHADERS, &num_shaders);

  // Get the shader names
  GLuint *shader_handles = new GLuint[num_shaders];
  glGetAttachedShaders(handle->program, num_shaders, NULL, shader_handles);

  // Delete the shaders
  for (int i = 0; i < num_shaders; i++) {
    glDeleteShader(shader_handles[i]);
  }

  // Delete the program
  glDeleteProgram(handle->program);
  check_gl_errors("shader_deconstructor");

  delete[] shader_handles;
}

void ddShader::init() {
  handle = new ddShaderHandle();
  if (!handle) {
    POW2_VERIFY_MSG(false, "init::Error creating shader handle", 0);
  }
  handle->program = 0;
  handle->program = glCreateProgram();
  POW2_VERIFY_MSG(!check_gl_errors("init"), "Failed to create shader program",
                  0);
}

void ddShader::create_vert_shader(const char *filePath) {
  const char *s_type = "VERTEX";
  std::string s_code = read_shader_file(filePath).c_str();
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  POW2_VERIFY_MSG(!check_gl_errors("create_vert_shader"),
                  "Failed %s shader init", s_type);

  // compile and link shader
  if (handle && compile_shader(&vertex_shader, s_code.c_str(), s_type)) {
    link_shader(&vertex_shader, s_type, &handle->program);
  }
	POW2_VERIFY_MSG(!check_gl_errors("create_vert_shader"),
									"Failed %s shader link", s_type);
}

void ddShader::create_frag_shader(const char *filePath) {
  const char *s_type = "FRAGMENT";
  std::string s_code = read_shader_file(filePath).c_str();
  GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  POW2_VERIFY_MSG(!check_gl_errors("create_frag_shader"),
                  "Failed %s shader init", s_type);

  // compile and link shader
  if (handle && compile_shader(&frag_shader, s_code.c_str(), s_type)) {
    link_shader(&frag_shader, s_type, &handle->program);
  }
	POW2_VERIFY_MSG(!check_gl_errors("create_frag_shader"),
									"Failed %s shader link", s_type);
}

void ddShader::create_comp_shader(const char *filePath) {
  const char *s_type = "COMPUTE";
  std::string s_code = read_shader_file(filePath).c_str();
  GLuint comp_shader = glCreateShader(GL_COMPUTE_SHADER);
  POW2_VERIFY_MSG(!check_gl_errors("create_comp_shader"),
                  "Failed %s shader init", s_type);

  // compile and link shader
  if (handle && compile_shader(&comp_shader, s_code.c_str(), s_type)) {
    link_shader(&comp_shader, s_type, &handle->program);
  }
	POW2_VERIFY_MSG(!check_gl_errors("create_comp_shader"),
									"Failed %s shader link", s_type);
}

void ddShader::create_geom_shader(const char *filePath) {
  const char *s_type = "GEOMETRY";
  std::string s_code = read_shader_file(filePath).c_str();
  GLuint geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
  POW2_VERIFY_MSG(!check_gl_errors("create_geom_shader"),
                  "Failed %s shader init", s_type);

  // compile and link shader
  if (handle && compile_shader(&geom_shader, s_code.c_str(), s_type)) {
    link_shader(&geom_shader, s_type, &handle->program);
  }
	POW2_VERIFY_MSG(!check_gl_errors("create_geom_shader"),
									"Failed %s shader link", s_type);
}

dd_array<ddQueryInfo> ddShader::query_shader_attributes() {
  dd_array<ddQueryInfo> info;
  if (!handle) return info;

  GLint num_attribs = 0;
  glGetProgramInterfaceiv(handle->program, GL_PROGRAM_INPUT,
                          GL_ACTIVE_RESOURCES, &num_attribs);

  // loop thru attributes
  info.resize(num_attribs);
  GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};
  for (int i = 0; i < (int)info.size(); i++) {
    GLint results[3];
    glGetProgramResourceiv(handle->program, GL_PROGRAM_INPUT, i, 3, properties,
                           3, NULL, results);

    // get attribute information and store in ddQueryInfo
    GLint name_length = results[0] + 1;
    char *name = new char[name_length];
    glGetProgramResourceName(handle->program, GL_UNIFORM, i, name_length, NULL,
                             name);
    info[i] = {(int)results[2], name, get_uniform_type(results[1])};
    delete[] name;
  }
  return info;
}

dd_array<ddQueryInfo> ddShader::query_uniforms() {
  dd_array<ddQueryInfo> info;
  if (!handle) return info;

  GLint num_uniforms = 0;
  glGetProgramInterfaceiv(handle->program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &num_uniforms);

  // loop thru uniforms
  info.resize(num_uniforms);
  GLenum properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};
  for (int i = 0; i < (int)info.size(); i++) {
    GLint results[4];
    glGetProgramResourceiv(handle->program, GL_UNIFORM, i, 4, properties, 4,
                           NULL, results);

    // get uniform information and store in ddQueryInfo
    GLint name_length = results[0] + 1;
    char *name = new char[name_length];
    glGetProgramResourceName(handle->program, GL_UNIFORM, i, name_length, NULL,
                             name);
    info[i] = {(int)results[2], name, get_uniform_type(results[1])};
    delete[] name;
  }
  return info;
}

void ddShader::use() {
  POW2_VERIFY_MSG(handle, "use::Shader program is null", 0);
  glUseProgram(handle->program);
}

std::string read_shader_file(const char *filename) {
  std::ifstream file(filename);
  if (file.is_open()) {
    std::stringstream isstr;
    isstr << file.rdbuf();
    file.close();
    return isstr.str();
  }
  std::string msg = "read_shader_file::Could not open: ";
  msg += filename;
  POW2_VERIFY_MSG(false, msg.c_str(), 0);
  return "";
}

bool compile_shader(GLuint *s_handle, const GLchar *s_code,
                    const char *s_type) {
  // load shader w/ one or more sources
  const GLchar *code_array[] = {s_code};
  glShaderSource(*s_handle, 1, code_array, NULL);

  glCompileShader(*s_handle);
  GLint result;
  glGetShaderiv(*s_handle, GL_COMPILE_STATUS, &result);
  if (GL_FALSE == result) {
    std::string log_str;
    GLint log_len;
    glGetShaderiv(*s_handle, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
      char *log = new char[log_len];
      GLsizei written;
      glGetShaderInfoLog(*s_handle, log_len, &written, log);
      log_str = log;
      delete[] log;
    }
    std::string msg = std::string(s_type) + " shader compilation failed!\n" +
                      "Error log:\n" + log_str;
    POW2_VERIFY_MSG(false, msg.c_str(), 0);
  }
  return true;
}

void link_shader(GLuint *s_handle, const char *s_type, GLuint *program) {
  glAttachShader(*program, *s_handle);
  glLinkProgram(*program);

  GLint status;
  glGetProgramiv(*program, GL_LINK_STATUS, &status);
  if (GL_FALSE == status) {
    std::string log_str;
    GLint log_len;
    glGetShaderiv(*program, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
      char *log = new char[log_len];
      GLsizei written;
      glGetShaderInfoLog(*program, log_len, &written, log);
      log_str = log;
      delete[] log;
    }
    std::string msg = std::string(s_type) + " shader link failed!\n" +
                      "Error log:\n" + log_str;
    POW2_VERIFY_MSG(false, msg.c_str(), 0);
  }
}
