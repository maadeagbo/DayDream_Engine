#pragma once

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include "GLFW/glfw3.h"
//#include "SDL.h"

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddEventQueue.h"
#include "ddIncludes.h"
#include "ddInput.h"
#include "ddPhysicsEngine.h"
#include "ddRenderEngine.h"
#include "ddTimer.h"

enum class GameState { LOADING, PAUSE, ACTIVE, NUM_STATES };

enum class EngineMode : unsigned {
  DD_NOT_SET = 0x0,
  DD_DEBUG = 0x1,
  DD_SECOND_DISPLAY = 0x2,
  DD_NO_CONSOLE = 0x4,
  DD_FULLSCREEN = 0x8,
  DD_VSYNC = 0x10,
  DD_GPU_INFO = 0x20
};
ENABLE_BITMASK_OPERATORS(EngineMode)

enum class EngineState { VIEWER, WORLD_BUILDER, MAIN };

struct ddEngine {
 public:
  ddEngine()
      : main_state(GameState::LOADING),
        // main_window(nullptr),
        main_window_glfw(nullptr),
        num_lvls(0),
        current_lvl(0),
        load_screen(false),
        flag_debug(false) {}
  ~ddEngine() {
    lua_close(main_lstate);
    main_physics.cleanup_world();
    // clean up assets
    ddAssets::cleanup();
  }

  void clean_up_GLFW();
  void clean_up();
  void startup_lua();
  void launch();
  void load();
  void register_lfuncs();
  bool level_select(const size_t w = 600, const size_t h = 600);
  void run();
  void update(DD_LEvent& _event);
  bool execTerminal(const char* cmd);
  void dd_open_window(const size_t width, const size_t height,
                      EngineMode mode = EngineMode::DD_NOT_SET);
  void window_load_GLFW(EngineMode mode);
  void update_GLFW();
  void shutdown();

  void get_GLFW_native_res(GLFWmonitor** monitors, int& _w, int& _h,
                           const unsigned win_idx);

  int window_w, window_h;
  GameState main_state;
  EngineState init_flag;
  GLFWwindow* main_window_glfw;
  lua_State* main_lstate;

  ddQueue main_q;
  ddPhysics main_physics;

  PushFunc q_push;
  DD_FuncBuff main_fb;
  size_t num_lvls;
  int current_lvl;
  bool load_screen;
  bool flag_debug;
};
