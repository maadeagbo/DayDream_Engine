#pragma once

#include "SDL.h"

#include "DD_AssetManager.h"
#include "DD_EventQueue.h"
#include "DD_Input.h"
#include "DD_LuaHooks.h"
#include "DD_PhysicsEngine.h"
#include "DD_Timer.h"
#include "DD_Types.h"

enum class GameState { LOADING, PAUSE, ACTIVE, NUM_STATES };

enum class EngineMode {
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

struct DD_Engine {
 public:
  DD_Engine()
      : main_state(GameState::LOADING),
        main_window(nullptr),
        num_lvls(0),
        current_lvl(0),
        load_screen(false),
        flag_debug(false) {}
  ~DD_Engine() {}

  bool InitGLLoadGen(const bool displayInfo = false);
  void CleanUpSDLContext();
  void cleanUpContexts();
  void startup_lua();
  void Launch();
  void Load();
	void register_lfuncs();
  bool LevelSelect(const size_t w = 600, const size_t h = 600);
  void Run();
  inline SDL_Window* GetWin() { return main_window; }
  void update(DD_LEvent& _event);
  // void execScript(std::string script_file);
  bool execTerminal(const char* cmd);
  void openWindow(const size_t width, const size_t height,
                  EngineMode mode = EngineMode::DD_NOT_SET);
  void updateSDL();
  void cleanUp();

  int m_WIDTH, m_HEIGHT;
  GameState main_state;
  EngineState init_flag;
  SDL_Window* main_window;
  SDL_GLContext main_glcontext;
  lua_State* main_lstate;

  DD_Timer main_timer;
  InputData main_input;
  DD_Queue main_q;
  DD_Physics main_physics;
  
	PushFunc q_push;
  DD_FuncBuff main_fb;
  size_t num_lvls;
  int current_lvl;
  bool load_screen;
  bool flag_debug;
};
