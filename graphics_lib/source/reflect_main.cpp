#include <Pow2Assert.h>
#include <fstream>
#include <string>
#include "GLFW/glfw3.h"
#include "GPUFrontEnd.h"
#include "ddShader.h"

struct ShFlags {
  bool vs = false;
  bool fs = false;
  bool gs = false;
  bool cs = false;
};

// initialize glfw for hidden window & context creation
void init_glfw();
// create file for outputing shader enum
std::ofstream create_file(const char *name, const bool append);
// test function for creating header
void test_reflect_func(const char *outfile, const char *e_name, bool app);
// output uniform reflection enums
void output_reflection(ddShader &shader, ShFlags flags, const char *fname,
                       const char *ename, bool append);

std::string fix_slashes(const char *file) {
#ifdef WIN32
  char bad_s = '/';
  char good_s = '\\';
#else
  char bad_s = '\\'; 
  char good_s = '/';
#endif  // _WIN32

  std::string out_str = file;
  std::replace(out_str.begin(), out_str.end(), bad_s, good_s);
  return out_str;
}

int main(const int argc, const char *argv[]) {
  // process input argument
  cbuff<512> help;
  help =
      "Shader Reflection Creator::Creates enums with type suffix and prefix "
      "names based on glsl shader uniforms (also exhibits secondary function "
      "of "
      "debugging shaders).\n\t-h Help info\n\t-a Append to file\n\t-f fragment "
      "shader\n\t-v vertex shader\n\t-c compute shader\n\t-g geometry "
      "shader\n\t"
      "-e Enum name\n\t-o output file name\n\t-t run default test";
  if (argc < 2) {
    printf("%s\n", help.str());
    return 1;
  }

  init_glfw();

  // initialize OpenGL
  if (!ddGPUFrontEnd::load_api_library(false)) {
    fprintf(stderr, "Failed to load graphics api library\n");
    std::exit(EXIT_FAILURE);
  }

  cbuff<512> file_str;
  bool append_file = false;
  bool run_defualt_test = false;
  bool enum_set = false;
  bool file_set = false;
  ShFlags flags;
  cbuff<64> enum_name;
  ddShader shader;
  // parse arguments
  printf("Reflect args:\n");
  int i = 1;
  while (i < argc) {
    const char *str = argv[i];
    switch (*str) {
      case '-':
        str++;
        switch (*str) {
          case 'a':
            append_file = true;
            printf("- Append enum to file\n");
            break;
          case 'e':
            enum_name = (i + 1 < argc) ? fix_slashes(argv[i + 1]).c_str()
                                       : "unset_enum";
            printf("- Enum set: %s\n", enum_name.str());
            enum_set = true;
            break;
          case 'v':
            shader.vs =
                (i + 1 < argc) ? fix_slashes(argv[i + 1]).c_str() : "vs_unset";
            printf("- Vertex: %s\n", shader.vs.str());
            flags.vs = true;
            break;
          case 'f':
            shader.fs =
                (i + 1 < argc) ? fix_slashes(argv[i + 1]).c_str() : "fs_unset";
            printf("- Fragment: %s\n", shader.fs.str());
            flags.fs = true;
            break;
          case 'g':
            shader.gs =
                (i + 1 < argc) ? fix_slashes(argv[i + 1]).c_str() : "gs_unset";
            printf("- Geometry: %s\n", shader.gs.str());
            flags.gs = true;
            break;
          case 'c':
            shader.cs =
                (i + 1 < argc) ? fix_slashes(argv[i + 1]).c_str() : "cs_unset";
            printf("- Compute: %s\n", shader.cs.str());
            flags.cs = true;
            break;
          case 'o':
            file_str = (i + 1 < argc) ? fix_slashes(argv[i + 1]).c_str()
                                      : "out_file.txt";
            printf("- File output: %s\n", file_str.str());
            file_set = true;
            break;
          case 't':
            run_defualt_test = true;
            printf("- Default test execution\n");
            break;
          case 'h':
            printf("%s\n", help.str());
            return 0;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
    i++;
  }
  printf("\n");

  if (run_defualt_test) {
    test_reflect_func("test.h", "test_e", append_file);
  } else {
    if (!file_set || !enum_set) {
      if (!file_set) fprintf(stderr, "File not provided\n");
      if (!enum_set) fprintf(stderr, "Enum name not provided\n");
      return 2;
    }
    if (flags.vs && flags.gs && flags.fs) {
      // vertex, geometry, & fragment shader
      ShFlags f;
      f.vs = f.gs = f.fs = true;
      output_reflection(shader, f, file_str.str(), enum_name.str(),
                        append_file);
    } else if (flags.vs && flags.fs) {
      // vertex & fragment shader
      ShFlags f;
      f.vs = f.fs = true;
      output_reflection(shader, f, file_str.str(), enum_name.str(),
                        append_file);
    } else if (flags.vs) {
      // vertex
      ShFlags f;
      f.vs = true;
      output_reflection(shader, f, file_str.str(), enum_name.str(),
                        append_file);
    } else if (flags.gs) {
      // geometry
      ShFlags f;
      f.gs = true;
      output_reflection(shader, f, file_str.str(), enum_name.str(),
                        append_file);
    } else if (flags.fs) {
      // fragment
      ShFlags f;
      f.fs = true;
      output_reflection(shader, f, file_str.str(), enum_name.str(),
                        append_file);
    }
    if (flags.cs) {
      // compute shader
      ShFlags f;
      f.cs = true;
      output_reflection(shader, f, file_str.str(), enum_name.str(),
                        append_file);
    }
  }
  printf("\n");

  // close resources and exit
  shader.cleanup();
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

  if (!append) out_file << "// Enums for managing shader uniforms\n\n";

  return out_file;
}

void test_reflect_func(const char *outfile, const char *e_name, bool app) {
  ddShader shader;
  shader.init();

  cbuff<512> file;
  file.format("%s%s", SHADER_DIR, "Lighting_V.vert");
  shader.create_vert_shader(file.str());
  file.format("%s%s", SHADER_DIR, "Lighting_F.frag");
  shader.create_frag_shader(file.str());

  file.format("%sinclude/%s", ROOT_DIR, outfile);
  std::ofstream out = create_file(fix_slashes(file.str()).c_str(), app);

  printf("Attributes:\n");
  // retrieve info atrribute info
  dd_array<ddQueryInfo> shader_info = shader.query_shader_attributes();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    const char *suffix = get_uniform_type(info.ptr->type);
    printf("- %d::%s%s\n", info.ptr->location, info.ptr->name.str(), suffix);
  }

  printf("Uniforms:\n");
  // retrieve uniform info and output enum
  out << "enum class " << e_name << " : int {\n";
  shader_info = shader.query_uniforms();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    const char *suffix = get_uniform_type(info.ptr->type);
    printf("- %d::%s%s\n", info.ptr->location, info.ptr->name.str(), suffix);
    out << "  " << info.ptr->name.str() << suffix << " = "
        << info.ptr->location;

    if (info.i != shader_info.size() - 1) out << ",\n";
  }
  out << "\n};\n";
  shader.cleanup();

  return;
}

void output_reflection(ddShader &shader, ShFlags flags, const char *fname,
                       const char *ename, bool append) {
  shader.init();

  if (flags.vs) shader.create_vert_shader(shader.vs.str());
  if (flags.gs) shader.create_geom_shader(shader.gs.str());
  if (flags.fs) shader.create_frag_shader(shader.fs.str());
  if (flags.cs) shader.create_comp_shader(shader.cs.str());

  std::ofstream out = create_file(fix_slashes(fname).c_str(), append);

  printf("\nAttributes:\n");
  // retrieve info atrribute info
  dd_array<ddQueryInfo> shader_info = shader.query_shader_attributes();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    const char *suffix = get_uniform_type(info.ptr->type);
    printf("%d::%s%s\n", info.ptr->location, info.ptr->name.str(), suffix);
  }

  printf("\nUniforms:\n");
  // retrieve uniform info and output enum
  out << "enum class " << ename << " : int {\n";
  shader_info = shader.query_uniforms();
  DD_FOREACH(ddQueryInfo, info, shader_info) {
    const char *suffix = get_uniform_type(info.ptr->type);
    printf("- %d::%s%s\n", info.ptr->location, info.ptr->name.str(), suffix);
    out << "  " << info.ptr->name.str() << suffix << " = "
        << info.ptr->location;

    if (info.i != shader_info.size() - 1) out << ",\n";
  }
  out << "\n};\n\n";
}
