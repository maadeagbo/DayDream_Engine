#include "GLFW\glfw3.h"
#include "GPUFrontEnd.h"
#include "ddShader.h"

void init_glfw();
void create_default_reflect_header();

int main(const int argc, const char *argv[]) {
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

void create_default_reflect_header() {
  ddShader shader;
  shader.init();

  cbuff<512> file;
  file.format("%s%s", SHADER_DIR, "GBuffer_V.vert");
  shader.create_vert_shader(file.str());
  file.format("%s%s", SHADER_DIR, "GBuffer_F.frag");
  shader.create_frag_shader(file.str());

  // retrieve info
  dd_array<ddQueryInfo> shader_info = shader.query_shader_attributes();

  DD_FOREACH(ddQueryInfo, info, shader_info) {
    printf("%d :: %s :: %s\n", info.ptr->location, info.ptr->name.str(),
           info.ptr->type.str());
  }
  shader_info = shader.query_uniforms();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    printf("%d :: %s :: %s\n", info.ptr->location, info.ptr->name.str(),
           info.ptr->type.str());
  }

  return;
}
