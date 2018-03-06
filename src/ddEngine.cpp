#include "ddEngine.h"
#include "ddFileIO.h"
#include "ddTerminal.h"

// imgui
#include <imgui_impl_glfw_gl3.h>

//#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR)))

namespace {
bool windowShouldClose = false;
std::chrono::milliseconds timespan(10);
std::chrono::milliseconds chrono_msec(1);

float PHYSICS_TICK = 0.f;

// int resolution_set[2] = {1400, 800};
unsigned num_monitors = 0;
int display_select = 0;
bool engine_mode_flags[] = {true, false, true, false};

const cbuff<32> exit_hash("sys_exit");
const cbuff<32> frame_enter_hash("frame_init");
const cbuff<32> frame_exit_hash("frame_exit");
const cbuff<32> load_hash("load_screen");
const cbuff<32> draw_hash("draw_world");
const cbuff<32> physics_hash("physics_step");
const cbuff<32> lvl_init_hash("_lvl_init_done");
const cbuff<32> lvl_asset_hash("_load_resource_done");
const cbuff<32> terminal_hash("poll_terminal");
const cbuff<32> process_terminal_hash("process_terminal");
const cbuff<32> init_screen_hash("init_screen");
const cbuff<32> reset_lvl_script_hash("reset_lvl");
}  // namespace

static void error_callback_glfw(int error, const char *description) {
  fprintf(stderr, "Error: %s\n", description);
}

void ddEngine::startup_lua() {
  main_lstate = init_lua_state();
  main_q.setup_lua(main_lstate);

  register_dd_libraries(main_lstate);
  register_lfuncs();
}

void ddEngine::dd_open_window(const size_t width, const size_t height,
                              EngineMode mode) {
  if (DD_GRAPHICS_API == 0) {
    ddTerminal::post("Graphics API::OpenGL 4.3");
  } else {
    ddTerminal::post("Graphics API::Vulkan");
  }
  window_w = (int)width;
  window_h = (int)height;

  // window_load_SDL2(mode);
  window_load_GLFW(mode);
}

void ddEngine::window_load_GLFW(EngineMode mode) {
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

  /* Create window */
  int count;
  GLFWmonitor *mntr = nullptr;
  GLFWmonitor **mntrs = glfwGetMonitors(&count);
  // log # of monitors
  num_monitors = count;

  // select monitor
  uint8_t m_selected = 0;
  if (bool(mode & EngineMode::DD_SECOND_DISPLAY)) {
    m_selected = display_select;
  }
  uint8_t mntr_idx = (m_selected >= (uint8_t)count) ? 0 : m_selected;
  if (bool(mode & EngineMode::DD_FULLSCREEN)) {
    // set to native resolution
    get_GLFW_native_res(mntrs, window_w, window_h, mntr_idx);
    mntr = mntrs[mntr_idx];
  }

  main_window_glfw =
      glfwCreateWindow(window_w, window_h, "DayDream Engine", mntr, NULL);
  if (main_window_glfw == nullptr) {
    printf("Error::GLFW::Failed to create GLFW window.\n");
    glfwTerminate();
    std::exit(EXIT_FAILURE);
  }
  // set window position
  glfwSetWindowPos(main_window_glfw, 100, 100);

  // set callbacks for input
  glfwSetKeyCallback(main_window_glfw, dd_key_callback);
  glfwSetCursorPosCallback(main_window_glfw, dd_mouse_pos_callback);
  glfwSetErrorCallback(error_callback_glfw);
  glfwSetMouseButtonCallback(main_window_glfw, dd_mouse_click_callback);
  glfwSetScrollCallback(main_window_glfw, dd_scroll_callback);
  glfwSetCharCallback(main_window_glfw, ImGui_ImplGlfwGL3_CharCallback);

  // initialize GLFW3
  ImGui_ImplGlfwGL3_Init(main_window_glfw, false);

  glfwMakeContextCurrent(main_window_glfw);
  /* Set vsync */
  const int vsyncflag = bool(mode & EngineMode::DD_VSYNC) ? 1 : 0;
  glfwSwapInterval(vsyncflag);

  if (!ddGPUFrontEnd::load_api_library(bool(mode & EngineMode::DD_GPU_INFO))) {
    fprintf(stderr, "Failed to load graphics api library\n");
    std::exit(EXIT_FAILURE);
  }
}

/// \brief Get native monitor resolution
void ddEngine::get_GLFW_native_res(GLFWmonitor **monitors, int &_w, int &_h,
                                   const unsigned win_idx) {
  const GLFWvidmode *mode = glfwGetVideoMode(monitors[win_idx]);
  _w = mode->width;
  _h = mode->height;
}

/* Close miscellanious pieces*/
void ddEngine::clean_up() {
  // ImGui_ImplSdlGL3_Shutdown();
  ImGui_ImplGlfwGL3_Shutdown();
  // clean_up_SDL();
  clean_up_GLFW();
}

void ddEngine::clean_up_GLFW() {
  // delete context and destroy window
  if (main_window_glfw) glfwDestroyWindow(main_window_glfw);
}

/// \brief Load correct sub-system based on init_flag
void ddEngine::launch() {
  switch (init_flag) {
    case EngineState::VIEWER:
      load();
      break;
    case EngineState::WORLD_BUILDER:
      break;
    case EngineState::MAIN:
      load();
      break;
    default:
      break;
  }
}

/// \brief Select level and set parameters
bool ddEngine::level_select(const size_t w, const size_t h) {
  dd_open_window(w, h, EngineMode::DD_GPU_INFO | EngineMode::DD_VSYNC);
  // grab levels from startup script
  cbuff<512> startup_script;
  startup_script.format("%s/scripts/startup.lua", RESOURCE_DIR);
  bool file_loaded = parse_luafile(main_lstate, startup_script.str());

  if (file_loaded) {
    // get reference to function
    int func_ref = get_lua_ref(main_lstate, nullptr, "generate_levels");
    DD_LEvent init_event;
    init_event.handle = "get_lvls";
    callback_lua(main_lstate, init_event, func_ref, main_fb, -1);

    int64_t *num_levels = main_fb.get_func_val<int64_t>("num_levels");
    if (num_levels) {
      // printf("Found # of levels: %d\n", *num_levels);
      lvls_list.resize((unsigned)(*num_levels));
      cbuff<4> _lvl;
      for (unsigned i = 0; i < (unsigned)lvls_list.size(); i++) {
        _lvl.format("%u", i + 1);
        lvls_list[i] = main_fb.get_func_val<const char>(_lvl.str());
      }
    }
  }

  init_flag = EngineState::MAIN;
  bool launch = false;
  int wh_idx[2] = {8, 4};

  while (!windowShouldClose && !launch) {
    ddGPUFrontEnd::clear_screen(0.3f, 0.3f, 0.3f, 1.0f);
    // start imgui window processing
    ImGui_ImplGlfwGL3_NewFrame();

    // poll GLFW events
    update_GLFW();

    // Query for level to load
    float scrW = (float)w / 1.25f;
    float scrH = (float)h / 1.1f;
    ImGui::SetNextWindowPos(ImVec2(w / 2 - scrW / 2, h / 2 - scrH / 2));
    ImGui::SetNextWindowSize(ImVec2(scrW, scrH));
    ImGuiWindowFlags _flags = 0;
    _flags |= ImGuiWindowFlags_NoTitleBar;
    _flags |= ImGuiWindowFlags_NoResize;
    _flags |= ImGuiWindowFlags_NoMove;
    ImGui::Begin("Level Select", nullptr, _flags);

    ddTerminal::dumpTerminalToImGuiText();
    ImGui::Dummy(ImVec2(0, 10));

    // Set options for loading
    if (lvls_list.size() > 0) {
      ImGui::ListBox("<-- Select Scene", &current_lvl, &lvls_list[0],
                     (int)lvls_list.size(), 4);
    } else {
      ImGui::Text("No levels present");
    }

    const int choices = 13;
    const char *res_[choices] = {"320",  "480",  "620",  "720",  "800",
                                 "1024", "1080", "1280", "1440", "1600",
                                 "1920", "2048", "3200"};
    ImGui::Combo("Width", &wh_idx[0], res_, choices);
    // ImGui::SameLine();
    ImGui::Combo("Height", &wh_idx[1], res_, choices);

    // ImGui::DragInt2("<-- Resolution", resolution_set, 10.f, 100);
    window_w = (int)strtod(res_[wh_idx[0]], nullptr);
    window_h = (int)strtod(res_[wh_idx[1]], nullptr);

    // stop edge clipping
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);
    // ImGui::PushItemWidth(-100);

    ImGui::Checkbox("VSYNC", &engine_mode_flags[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Full Screen", &engine_mode_flags[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Select Display", &engine_mode_flags[2]);
    ImGui::Checkbox("(Windows) Close Console", &engine_mode_flags[3]);
    // select monitor
    ImGui::SliderInt("Display monitor", &display_select, 0, num_monitors - 1);

    ImGui::Dummy(ImVec2(0, 10));
    if (lvls_list.size() > 0) {
      ImGui::Dummy(ImVec2(scrW / 2 - 60 / 2 - 10, 20));
      ImGui::SameLine();
      launch = ImGui::Button("Launch", ImVec2(60, 20));
      if (launch) {
        // set up queue handlers
        main_q.init_level_scripts(lvls_list[current_lvl]);
        // cpp lua functions for the level
        cbuff<64> key = lvls_list[current_lvl];
        std::map<cbuff<64>, std::function<void(lua_State *)>> lvl_funcs =
            get_reflections();
        POW2_VERIFY(lvl_funcs.size() != 0);
        lvl_funcs[key](main_lstate);
      }
    }

    ImGui::End();
    ImGui::Render();
    // swap buffers
    // SDL_GL_SwapWindow(main_window);
    glfwSwapBuffers(main_window_glfw);
  }

  ddTerminal::clearTerminal();
  clean_up();
  return launch;
}

void ddEngine::load() {
#ifdef _WIN32
  std::_Ph<1> arg_1 = std::placeholders::_1;
#else
  std::_Placeholder<1> arg_1 = std::placeholders::_1;
#endif
  SysEventHandler _sh;
  PushFunc push_func = std::bind(&ddQueue::push, &main_q, arg_1);
  q_push = push_func;

  // initialize time
  ddTime::initialize();

  // initialize bullet physics library
  main_physics.initialize_world();

  // Initialize resource manager
  ddAssets::initialize(main_physics.world);

  // initialize scene manager
  ddSceneManager::initialize(window_w, window_h);

  // set up math/physics library
  // DD_MathLib::setResourceBin(&main_res);

  // Load render engine
  /*
  main_renderer.m_resourceBin = &main_res;
  main_renderer.push = push_func;
  main_renderer.m_time = &main_timer;
  main_renderer.LoadRendererEngine((GLfloat)window_w, (GLfloat)window_h);
  main_renderer.m_particleSys->m_resBin = &main_res;
  // add render engine handler
  system_id = getCharHash("dd_renderer");
  _sh = std::bind(&DD_Renderer::render_handler, &main_renderer, arg_1);
  main_q.register_sys_func(system_id, _sh);
  main_q.subscribe(getCharHash("render"), system_id);
  main_q.subscribe(getCharHash("toggle_screenshots"), system_id);
  main_q.subscribe(getCharHash("rendstat"), system_id);

  // load compute unit and handler
  main_comp.init();
  main_comp.res_ptr = &main_res;
  main_comp.push = push_func;
  system_id = getCharHash("dd_compute");
  _sh = std::bind(&DD_Compute::compute, &main_comp, arg_1);
  main_q.register_sys_func(system_id, _sh);
  main_q.subscribe(getCharHash("compute_texA"), system_id);
  //*/

  // initialize Animation system and register handlers
  /*
        main_animator.res_ptr = &main_res;
  main_animator.push = push_func;
  system_id = getCharHash("dd_animation");
  _sh = std::bind(&DD_AnimSystem::anim_update, &main_animator, arg_1);
  main_q.register_sys_func(system_id, _sh);
  main_q.subscribe(getCharHash("update_animation"), system_id);

  // add particle handler
  main_renderer.m_particleSys->push = push_func;
  system_id = getCharHash("dd_particles");
  _sh = std::bind(&DD_ParticleSys::create, main_renderer.m_particleSys, arg_1);
  main_q.register_sys_func(system_id, _sh);
  main_q.subscribe(getCharHash("generate_emitter"), system_id);
  main_q.subscribe(getCharHash("create_cloth"), system_id);
  main_q.subscribe(getCharHash("debug"), system_id);
  main_q.subscribe(getCharHash("system_pause"), system_id);
  main_q.subscribe(getCharHash("update_cloth"), system_id);
  //_sh = std::bind(&DD_ParticleSys::add_job, main_renderer.m_particleSys,
  // arg_1); register_func("particle_jobA");
        //*/

  // add AI program handler and setup
  /*
        main_ai.res_ptr = &main_res;
  main_ai.push = push_func;
  system_id = getCharHash("dd_ai");
  _sh = std::bind(&DD_AISystem::ai_update, &main_ai, arg_1);
  main_q.register_sys_func(system_id, _sh);
  main_q.subscribe(getCharHash("update_AI"), system_id);
        //*/

  // terminal input callback
  _sh = std::bind(&ddTerminal::get_input, std::placeholders::_1);
  main_q.register_sys_func(sys_terminal_hash, _sh);
  main_q.subscribe(terminal_hash.gethash(), sys_terminal_hash);

  // add engine callback
  _sh = std::bind(&ddEngine::update, this, arg_1);
  main_q.register_sys_func(sys_engine_hash, _sh);
  main_q.subscribe(exit_hash.gethash(), sys_engine_hash);
  main_q.subscribe(frame_enter_hash.gethash(), sys_engine_hash);
  main_q.subscribe(frame_exit_hash.gethash(), sys_engine_hash);
  main_q.subscribe(load_hash.gethash(), sys_engine_hash);
  main_q.subscribe(lvl_init_hash.gethash(), sys_engine_hash);
  main_q.subscribe(lvl_asset_hash.gethash(), sys_engine_hash);
  main_q.subscribe(init_screen_hash.gethash(), sys_engine_hash);
  main_q.subscribe(draw_hash.gethash(), sys_engine_hash);
  main_q.subscribe(physics_hash.gethash(), sys_engine_hash);
  main_q.subscribe(process_terminal_hash.gethash(), sys_engine_hash);
  main_q.subscribe(reset_lvl_script_hash.gethash(), sys_engine_hash);

  // load terminal history
  ddTerminal::inTerminalHistory();
}

void ddEngine::register_lfuncs() {
  // add asset function
  ddAssets::log_lua_func(main_lstate);
  // register globals from ddRenderer
  ddRenderer::init_lua_globals(main_lstate);
}

void ddEngine::update_GLFW() {
  // get events
  glfwPollEvents();
  // track close event
  InputData idata = ddInput::get_input();
  if (idata.keys[(int)DD_Keys::Escape_Key].active) {
    glfwSetWindowShouldClose(main_window_glfw, GLFW_TRUE);  // default exit
  }
  if (glfwWindowShouldClose(main_window_glfw)) {
    windowShouldClose = true;  // for launch screen
    main_q.shutdown_queue();
  }
}

/* Call in main function to begin up game loop
 */
void ddEngine::run() {
  // set certain default parameters for assets
  ddAssets::default_params(window_w, window_h);
  ddSceneManager::initialize(window_w, window_h);

  // create new screen
  DD_LEvent _e1;
  _e1.handle = init_screen_hash;
  q_push(_e1);

  // turn on load screen
  DD_LEvent _e2;
  _e2.handle = load_hash;
  q_push(_e2);

  // add async level assets load
  DD_LEvent _e3;
  _e3.handle = main_q.res_call;
  q_push(_e3);

  // add frame update
  DD_LEvent _e4;
  _e4.handle = frame_enter_hash;
  q_push(_e4);

  // exit (debug testing)
  DD_LEvent _e5;
  _e5.handle = exit_hash;
  _e5.delay = 1000;
  // q_push(_event);

  main_q.process_queue();
}

/// \brief Clean up engine resources
void ddEngine::shutdown() {
  ddTerminal::outTerminalHistory();  // save history
  // render engine
  ddRenderer::shutdown();
  // cleanup particles
  ddParticleSys::cleanup();
  // clean up assets
  ddAssets::cleanup_assets();
  // shutdown glfw
  glfwTerminate();

  // close lua
  lua_close(main_lstate);
  // close physics engine
  main_physics.cleanup_world();
}

bool ddEngine::execTerminal(const char *cmd) {
  if (cmd) {
    cbuff<32> str_arg;
    bool args_present = false;

    // split arguments and tags if possible
    std::string head;
    std::string buff = cmd;
    size_t head_idx = buff.find_first_of(" ");
    if (head_idx != std::string::npos) {
      args_present = true;
      head = buff.substr(0, head_idx);
      str_arg = buff.substr(head_idx + 1).c_str();
    } else {
      head = buff;
    }

    DD_LEvent _event;
    _event.handle = head.c_str();
    // add events after spltting
    if (args_present) {
      dd_array<cbuff<32>> args = StrSpace::tokenize1024<32>(str_arg.str(), " ");
      for (unsigned i = 0; i < args.size() && i < MAX_EVENT_ARGS; ++i) {
        str_arg.format("%u", i);
        add_arg_LEvent(&_event, str_arg.str(), args[i].str()); 
      }
    }

    q_push(_event);
    return true;
  } else {
    return false;
  }
}

void ddEngine::update(DD_LEvent &_event) {
  size_t e_sig = _event.handle.gethash();
  if (e_sig == exit_hash.gethash()) {  // close app
    main_q.shutdown_queue();
  } else if (e_sig == init_screen_hash.gethash()) {  // open new window
    EngineMode init_options = EngineMode::DD_NOT_SET;
    init_options = (engine_mode_flags[0])
                       ? EngineMode(init_options | EngineMode::DD_VSYNC)
                       : init_options;
    init_options = (engine_mode_flags[1])
                       ? EngineMode(init_options | EngineMode::DD_FULLSCREEN)
                       : init_options;
    init_options =
        (engine_mode_flags[2])
            ? EngineMode(init_options | EngineMode::DD_SECOND_DISPLAY)
            : init_options;
    init_options = (engine_mode_flags[3])
                       ? EngineMode(init_options | EngineMode::DD_NO_CONSOLE)
                       : init_options;

    dd_open_window(window_w, window_h, init_options);
    // set useful lua globals
    set_lua_global(main_lstate, "WINDOW_WIDTH", (int64_t)window_w);
    set_lua_global(main_lstate, "WINDOW_HEIGHT", (int64_t)window_h);

    // initialize rendering engine
    ddRenderer::initialize((unsigned)window_w, (unsigned)window_h);

    // initialize particle engine
    ddParticleSys::initialization((unsigned)window_w, (unsigned)window_h);

  } else if (e_sig == frame_enter_hash.gethash()) {  // setup new frame
    // clear framebuffer
    ddGPUFrontEnd::clear_screen();

    ddTime::update();
    PHYSICS_TICK += ddTime::get_avg_frame_time();

    // start imgui window processing
    ImGui_ImplGlfwGL3_NewFrame();
    // poll GLFW events
    update_GLFW();

    // run scene graph
    // ResSpace::updateSceneGraph(&main_res, main_timer.getTimeFloat());

    // send event to check "future" events status
    DD_LEvent new_event;
    new_event.handle = main_q.check_future;
    q_push(new_event);

    // process terminal
    new_event.handle = terminal_hash;
    q_push(new_event);
    new_event.handle = process_terminal_hash;
    q_push(new_event);

    // update input for scripts
    ddInput::send_upstream_to_lua(main_lstate);

    if (load_screen) {
      // Show load screen
      ddRenderer::render_load_screen();

      // send frame exit event
      new_event.handle = frame_exit_hash;
      q_push(new_event);
    } else {
      // send next event in sequence
      // terminal, VR, AI, level update,
      // post update (periodic update for scripts),
      // physics, animation, render

      // send lvl update event
      new_event.handle = main_q.lvl_call;
      q_push(new_event);

      // send physics event (locked to 60 fps update or vsync on)
      if (engine_mode_flags[0] || PHYSICS_TICK > 1.f / 60.f) {
        PHYSICS_TICK = 0.f;

        new_event.handle = main_q.physics_tick;
        q_push(new_event);
      }
      new_event.handle = physics_hash;
      q_push(new_event);

      // send render event
      new_event.handle = draw_hash;
      q_push(new_event);

      // send frame exit event
      new_event.handle = frame_exit_hash;
      q_push(new_event);
    }
  } else if (e_sig == frame_exit_hash.gethash()) {  // exit frame
    // query terminal
    ddTerminal::display((float)window_w, (float)window_h);
    // render IMGUI ui
    ImGui::Render();
    // swap buffers
    glfwSwapBuffers(main_window_glfw);

    // push event for starting next frame
    DD_LEvent new_event;
    new_event.handle = frame_enter_hash;
    q_push(new_event);

  } else if (e_sig == load_hash.gethash()) {  // load screen
    load_screen ^= 1;
    ddAssets::set_load_screen_flag(load_screen);
  } else if (e_sig == process_terminal_hash.gethash()) {  // process terminal
    bool more_cmds = true;
    while (more_cmds) {
      const char *cmd = ddTerminal::pollBuffer();
      more_cmds = execTerminal(cmd);
    }
  } else if (e_sig == lvl_init_hash.gethash()) {  // post lvl init function
    // add functionality to set active skybox in scripts
    // add function from ddEngine that sets: main_renderer.m_lvl_cubMap

    // load assets to gpu
    ddAssets::load_to_gpu();

    // turn off load screen
    DD_LEvent new_event;
    new_event.handle = load_hash;
    q_push(new_event);

  } else if (e_sig == lvl_asset_hash.gethash()) {  // post resource load
    // add async level init
    DD_LEvent new_event;
    new_event.handle = main_q.lvl_call_i;
    q_push(new_event);
  } else if (e_sig == draw_hash.gethash()) {  // draw call
    // render 3D world
    ddRenderer::draw_world();
  } else if (e_sig == physics_hash.gethash()) {  // physics update
    // scene graph
    ddSceneManager::update_scene_graph();
    // physics simulation
    main_physics.step_simulate(ddTime::get_avg_frame_time());
  } else if (e_sig == reset_lvl_script_hash.gethash()) {  // lvl update script
    // reset lua script
    main_q.init_level_scripts(lvls_list[current_lvl], true);
  }
}
