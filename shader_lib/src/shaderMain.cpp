#include <fstream>
#include "gl_core_4_3.h"
#include "GLFW/glfw3.h"
#include "GPUFrontEnd.h"
#include "ddShader.h"

// initialize glfw for hidden window & context creation
void init_glfw();
// create file for outputing shader enum
std::ofstream create_file(const char *name, const bool append);
// return suffix for uniform type based on string
const char *uniform_suffix(GLint type);
// test function for creating header
void create_default_reflect_header();

int main(const int argc, const char *argv[]) {
  // process input argument
  cbuff<512> help;
  help =
      "Shader Reflection Creator::Creates enums with type suffix and prefix "
      "names based on glsl shader uniforms (also exhibits secondary function of "
      "debugging shaders).\n\t-h Help info\n\t-a Append to file\n\t-f Path to "
      "shader\n\t-o Enum name";
  cbuff<512> arg_str;
  if (argc < 5) {
    printf("%s\n", help.str());
    return 1;
  }

  init_glfw();

  // initialize OpenGL
  if (!ddGPUFrontEnd::load_api_library()) {
    fprintf(stderr, "Failed to load graphics api library\n");
    std::exit(EXIT_FAILURE);
  }

  // run default
  create_default_reflect_header();

  // close hidden window and exit
  glfwTerminate();
  return 0;
}

void init_glfw() {
  // Initialize the library
  if (!glfwInit()) {
    printf("Error::GLFW::Could not initialize library.\n");
    std::exit(EXIT_FAILURE);
  }
  // load OpenGL api
  if (DD_GRAPHICS_API == 0) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  } else {
    // load Vulkan api
  }
  GLFWwindow *win = glfwCreateWindow(1, 1, "DayDream Engine", NULL, NULL);
  if (win == nullptr) {
    printf("Error::GLFW::Failed to create GLFW window.\n");
    glfwTerminate();
    std::exit(EXIT_FAILURE);
  }
  // get OpenGL context
  glfwMakeContextCurrent(win);
  // hide window
  glfwHideWindow(win);
}

std::ofstream create_file(const char *name, const bool append) {
  std::ios_base::openmode ios_flag = std::ios_base::out;
  if (append) {
    ios_flag = std::ios_base::app;
  }
  std::ofstream out_file(name, ios_flag);

  POW2_VERIFY_MSG(out_file.is_open(), "create_file::Failed to create file", 0);
  out_file << "// Enums for managing shader uniforms\n\n";
  return out_file;
}

const char *uniform_suffix(GLint type) {
  switch (type) {
    case GL_FLOAT:
      return "_f";
    case GL_FLOAT_VEC2:
      return "_v2";
    case GL_FLOAT_VEC3:
      return "_v3";
    case GL_FLOAT_VEC4:
      return "_v4";
    case GL_DOUBLE:
      return "_d";
    case GL_INT:
      return "_i";
    case GL_UNSIGNED_INT:
      return "_ui";
    case GL_BOOL:
      return "_b";
    case GL_FLOAT_MAT2:
      return "_m2x2";
    case GL_FLOAT_MAT3:
      return "_m3x3";
    case GL_FLOAT_MAT4:
      return "_m4x4";
    case GL_SAMPLER_2D:
      return "_smp2d";
    case GL_SAMPLER_CUBE:
      return "_smpCube";
    default:
      return "_";
  }
}

void create_default_reflect_header() {
  ddShader shader;
  shader.init();

  cbuff<512> file;
  file.format("%s%s", SHADER_DIR, "Lighting_V.vert");
  shader.create_vert_shader(file.str());
  file.format("%s%s", SHADER_DIR, "Lighting_F.frag");
  shader.create_frag_shader(file.str());

  file.format("%sinclude/%s", ROOT_DIR, "ddShaderReflect.h");
  std::ofstream out = create_file(file.str(), false);

  printf("Attributes:\n");
  // retrieve info atrribute info
  dd_array<ddQueryInfo> shader_info = shader.query_shader_attributes();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    const char *suffix = uniform_suffix((GLint)info.ptr->type);
    printf("%d::%s%s\n", info.ptr->location, info.ptr->name.str(), suffix);
  }

  printf("Uniforms:\n");
  // retrieve uniform info and output enum
  out << "enum class shaderTest : unsigned {\n";
  shader_info = shader.query_uniforms();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    const char *suffix = uniform_suffix((GLint)info.ptr->type);
    printf("%d::%s%s\n", info.ptr->location, info.ptr->name.str(), suffix);
    out << "  " << info.ptr->name.str() << suffix << " = "
        << info.ptr->location;

    if (info.i != shader_info.size() - 1) out << ",\n";
  }
  out << "\n};\n";

  return;
}
