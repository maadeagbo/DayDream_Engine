#pragma once

#include "SDL.h"

#include "../Core/AssetViewer/DD_AssetViewer.h"
#include "DD_AISystem.h"
#include "DD_AnimSystem.h"
#include "DD_ComputeGPU.h"
#include "DD_EventQueue.h"
#include "DD_GameLevel.h"
#include "DD_Input.h"
#include "DD_LuaHooks.h"
#include "DD_MathLib.h"
#include "DD_RenderEngine.h"
#include "DD_ResourceLoader.h"
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
        main_lvl(10),
        num_lvls(0),
        current_lvl(0),
        load_assets(false),
        flag_debug(false) {}
  ~DD_Engine() {}

  bool InitGLLoadGen(const bool displayInfo = false);
  void CleanUpSDLContext();
  void cleanUpContexts();
  void startup_lua();
  void Launch();
  void Load();
  void LoadViewer();
  bool LevelSelect(const size_t w = 600, const size_t h = 600);
  void Run();
  inline SDL_Window* GetWin() { return main_window; }
  void AddLevel(DD_GameLevel* level, const char* assetLoc, const char* ID);
  void LevelAssets(const char* assets_file);
  void LoadQueue();
  DD_Event Update(DD_Event& event);
  void execScript(std::string script_file, float frametime);
  bool execTerminal(const char* cmd, float frametime);
  void openWindow(const size_t width, const size_t height,
                  EngineMode mode = EngineMode::DD_NOT_SET);
  void InitCurrentLevel();
  void updateSDL();
  void gameLoop();
  void cleanUp();

  int m_WIDTH, m_HEIGHT;
  GameState main_state;
  EngineState init_flag;
  SDL_Window* main_window;
  SDL_GLContext main_glcontext;
  lua_State* main_lstate;
  DD_Renderer main_renderer;
  DD_AISystem main_ai;
  DD_AnimSystem main_animator;
  DD_Timer main_timer;
  DD_Input main_input;
  DD_Queue main_q;
  DD_Resources main_res;
  DD_Compute main_comp;
  DD_AssetViewer main_viewer;
  DD_CallBackBuff main_cb;
  std::string lvl_resource;
  dd_array<DD_GameLevel*> main_lvl;
  size_t num_lvls;
  int current_lvl;
  bool load_assets;
  bool flag_debug;
};
