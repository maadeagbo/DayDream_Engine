#include "DD_Engine.h"
#include "DD_FileIO.h"
#include "DD_Terminal.h"

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR)))

namespace {
bool windowShouldClose = false;
// size_t lvl_agents = 0;

// bool loading_lvl = false, async_done = false, p_tex_loading = true,
//     loading_agents = false;
// std::future<void> load_RES, load_RAM, load_LVL;
std::chrono::milliseconds timespan(10);
std::chrono::milliseconds chrono_msec(1);

const bool UNLOCK_FRAMEPACE = true;
const size_t FRAME_CAP = 100;
const u64 SECOND_LL = 1000000000LL;

// int resolution_set[2] = {1400, 800};
bool engine_mode_flags[] = {true, false, false, false};

const cbuff<32> exit_hash("sys_exit");
const cbuff<32> frame_enter_hash("frame_init");
const cbuff<32> frame_exit_hash("frame_exit");
const cbuff<32> load_hash("load_screen");
const cbuff<32> lvl_init_hash("_lvl_init_done");
const cbuff<32> lvl_asset_hash("_asset_init");
const cbuff<32> terminal_hash("poll_terminal");
const cbuff<32> init_screen_hash("init_screen");
}  // namespace

void DD_Engine::startup_lua() {
  main_lstate = init_lua_state();
  main_q.set_lua_ptr(main_lstate);

  register_lfuncs();
}

void DD_Engine::openWindow(const size_t width, const size_t height,
                           EngineMode mode) {
  window_w = (int)width;
  window_h = (int)height;

  u32 flags = SDL_WINDOW_OPENGL;
  int monitor_choice = 0, vsync_option = 0;

  // engine flags
  if (bool(mode & EngineMode::DD_DEBUG)) {
    flag_debug = true;
    DD_Terminal::flipDebugFlag();
  }
  if (bool(mode & EngineMode::DD_NO_CONSOLE)) {
#ifdef WIN32
    FreeConsole();
#endif
  }
  if (bool(mode & EngineMode::DD_FULLSCREEN)) {
    flags |= SDL_WINDOW_FULLSCREEN;
    // flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; //use this for fake fullscreen
  }
  if (bool(mode & EngineMode::DD_SECOND_DISPLAY)) {
    monitor_choice = 1;
  }
  if (bool(mode & EngineMode::DD_VSYNC)) {
    vsync_option = 1;
  }

  // init SDL2
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Failed to init SDL\n");
    std::exit(EXIT_FAILURE);
  }
  // create centered window (set hints before)
  // SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
  main_window = SDL_CreateWindow("DayDream Engine Alpha",
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_choice),
                                 SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_choice),
                                 window_w, window_h, flags);
  // reset base width and height incase fullscreen set
  SDL_GetWindowSize(main_window, &window_w, &window_h);

  // Setup ImGui binding
  ImGui_ImplSdlGL3_Init(main_window);

  // set opengl version
  // SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated
  // functions are disabled
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  main_glcontext = SDL_GL_CreateContext(main_window);
  if (main_glcontext == NULL) {
    fprintf(stderr,
            "Failed to create OpenGL 4.3 context and double buffering\n");
    std::exit(EXIT_FAILURE);
  }
  // This makes our buffer swap un-syncronized with the monitor's vertical
  // refresh
  SDL_GL_SetSwapInterval(vsync_option);

  if (!InitGLLoadGen(bool(mode & EngineMode::DD_GPU_INFO))) {
    fprintf(stderr, "Failed to load OpenGL generator\n");
    std::exit(EXIT_FAILURE);
  }
}

/* Load OpenGL generator
 * \param displayInfo Set to true to display info of system
 */
bool DD_Engine::InitGLLoadGen(const bool displayInfo) {
  // Initialize GLLoadGen to setup the OpenGL Function pointers
  int loaded = ogl_LoadFunctions();
  if (loaded == ogl_LOAD_FAILED) {
    int num_failed = loaded - ogl_LOAD_SUCCEEDED;
    std::string temp = "Number of functions that failed to load: " +
                       std::to_string(num_failed) + ".\n";
    DD_Terminal::post(temp);
    return false;
  }

  if (displayInfo) {
    // check opengl version supported
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    // check number of color attachments and drawbuffer attachments
    GLint maxAttach = 0, maxDrawBuf = 0;
    ;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuf);

    // check compute shader limitations
    int maxComputeWG_COUNT_0 = 0, maxComputeWG_COUNT_1 = 0,
        maxComputeWG_COUNT_2 = 0, maxComputeWG_SIZE_0 = 0,
        maxComputeWG_SIZE_1 = 0, maxComputeWG_SIZE_2 = 0,
        maxComputeWG_INVOCATIONS = 0;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxComputeWG_COUNT_0);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxComputeWG_COUNT_1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxComputeWG_COUNT_2);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxComputeWG_SIZE_0);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxComputeWG_SIZE_1);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxComputeWG_SIZE_2);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                  &maxComputeWG_INVOCATIONS);

    char buff[100];
    snprintf(buff, 100, "GL Vendor             :%s\n", vendor);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GL Renderer           :%s\n", renderer);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GL Version (string)   :%s\n", version);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GL Version (integer)  :%d.%d\n", major, minor);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GLSL Version          :%s\n\n", glslVersion);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Color Attachments    :%d\n", maxAttach);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Draw Buffers         :%d\n\n", maxDrawBuf);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Count (0)    :%d\n",
             maxComputeWG_COUNT_0);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Count (1)    :%d\n",
             maxComputeWG_COUNT_1);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Count (2)    :%d\n",
             maxComputeWG_COUNT_2);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Size  (0)    :%d\n",
             maxComputeWG_SIZE_0);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Size  (1)    :%d\n",
             maxComputeWG_SIZE_1);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Size  (2)    :%d\n",
             maxComputeWG_SIZE_2);
    DD_Terminal::post(buff);
    snprintf(buff, 100, "GPU Max Compute Work Group Invocations  :%d\n\n",
             maxComputeWG_INVOCATIONS);
    DD_Terminal::post(buff);
  }
  return true;
}

/* Close SDL and OpenGL context
 */
void DD_Engine::CleanUpSDLContext() {
  // delete context
  SDL_GL_DeleteContext(main_glcontext);
  // destroy window
  SDL_DestroyWindow(main_window);
}

/* Close miscellanious pieces*/
void DD_Engine::cleanUpContexts() {
  ImGui_ImplSdlGL3_Shutdown();
  CleanUpSDLContext();
}

/// \brief Load correct sub-system based on init_flag
void DD_Engine::Launch() {
  switch (init_flag) {
    case EngineState::VIEWER:
      Load();
      break;
    case EngineState::WORLD_BUILDER:
      break;
    case EngineState::MAIN:
      Load();
      break;
    default:
      break;
  }
}

/// \brief Select level and set parameters
bool DD_Engine::LevelSelect(const size_t w, const size_t h) {
  openWindow(w, h, EngineMode::DD_GPU_INFO | EngineMode::DD_VSYNC);
  // grab levels from startup script
  cbuff<512> startup_script;
  startup_script.format("%sscripts/startup.lua", ROOT_DIR);
  bool file_loaded = parse_luafile(main_lstate, startup_script.str());

  // create array of levels
  dd_array<const char *> lvls_list;
  if (file_loaded) {
    // get reference to function
    int func_ref = get_lua_ref(main_lstate, nullptr, "generate_levels");
    DD_LEvent init_event;
    init_event.handle = "get_lvls";
    callback_lua(main_lstate, init_event, main_fb, func_ref, -1);

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
  glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
  int wh_idx[2] = {8, 4};

  while (!windowShouldClose && !launch) {
    glClear(GL_COLOR_BUFFER_BIT);
    // start imgui window processing
    ImGui_ImplSdlGL3_NewFrame(main_window);

    // pole SDL events
    updateSDL();

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

    DD_Terminal::dumpTerminalToImGuiText();
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

    ImGui::Checkbox("VSYNC", &engine_mode_flags[0]);
    ImGui::SameLine();
    ImGui::Checkbox("Full Screen", &engine_mode_flags[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Second Display", &engine_mode_flags[2]);
    ImGui::Checkbox("(Windows) Close Console", &engine_mode_flags[3]);

    ImGui::Dummy(ImVec2(0, 10));
    if (lvls_list.size() > 0) {
      ImGui::Dummy(ImVec2(scrW / 2 - 60 / 2 - 10, 20));
      ImGui::SameLine();
      launch = ImGui::Button("Launch", ImVec2(60, 20));
      if (launch) {
        // set up queue handlers
        main_q.init_level_scripts(lvls_list[current_lvl]);
      }
    }
    /*ImGui::Dummy(ImVec2(scrW / 2 - 180 / 2 - 10, 20));
    ImGui::SameLine();
    if (ImGui::Button("Launch Asset Viewer", ImVec2(180, 20))) {
      launch = true;
      init_flag = EngineState::VIEWER;
    };*/

    ImGui::End();
    ImGui::Render();
    // swap buffers
    SDL_GL_SwapWindow(main_window);
  }

  DD_Terminal::clearTerminal();
  cleanUpContexts();
  return launch;
}

void DD_Engine::Load() {
#ifdef _WIN32
  std::_Ph<1> arg_1 = std::placeholders::_1;
#else
  std::_Placeholder<1> arg_1 = std::placeholders::_1;
#endif
  SysEventHandler _sh;
  PushFunc push_func = std::bind(&DD_Queue::push, &main_q, arg_1);
  q_push = push_func;
  /*
  EngineMode init_options = EngineMode::DD_NOT_SET;
  init_options = (engine_mode_flags[0])
                ? EngineMode(init_options | EngineMode::DD_VSYNC)
                : init_options;
  init_options = (engine_mode_flags[1])
                ? EngineMode(init_options | EngineMode::DD_FULLSCREEN)
                : init_options;
  init_options = (engine_mode_flags[2])
                ? EngineMode(init_options | EngineMode::DD_SECOND_DISPLAY)
                : init_options;
  init_options = (engine_mode_flags[3])
                ? EngineMode(init_options | EngineMode::DD_NO_CONSOLE)
                : init_options;

  openWindow(window_w, window_h, init_options);
  // set useful lua globals
  set_lua_global(main_lstate, "SCR_W", window_w);
  set_lua_global(main_lstate, "SCR_H", window_h);
//*/
  // initialize time
  DD_Time::initialize();

  // initialize bullet physics library
  main_physics.initialize_world();

  // Initialize resource manager
  dd_assets_initialize(main_physics.world);

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
  _sh = std::bind(&DD_Terminal::get_input, std::placeholders::_1);
  main_q.register_sys_func(sys_terminal_hash, _sh);
  main_q.subscribe(getCharHash("input"), sys_terminal_hash);

  // add engine callback
  _sh = std::bind(&DD_Engine::update, this, arg_1);
  main_q.register_sys_func(sys_engine_hash, _sh);
  main_q.subscribe(getCharHash("sys_exit"), sys_engine_hash);
  main_q.subscribe(getCharHash("frame_init"), sys_engine_hash);
  main_q.subscribe(getCharHash("frame_exit"), sys_engine_hash);
  main_q.subscribe(getCharHash("load_screen"), sys_engine_hash);
  main_q.subscribe(getCharHash("init_resources"), sys_engine_hash);

  // load terminal history
  DD_Terminal::inTerminalHistory();
}

void DD_Engine::register_lfuncs() {
  // add DD_MeshData creation function
  add_func_to_scripts(main_lstate, dd_assets_create_mesh, "load_ddm");
  // camera creation
  add_func_to_scripts(main_lstate, dd_assets_create_cam, "create_cam");
  // light creation
  add_func_to_scripts(main_lstate, dd_assets_create_light, "create_light");
  // agent creation
  add_func_to_scripts(main_lstate, dd_assets_create_agent, "create_agent");
}

void DD_Engine::updateSDL() {
  // get keyboard events
  SDL_Event sdlEvent;
  DD_Input::new_frame(main_input);
  while (SDL_PollEvent(&sdlEvent)) {
    // imgui process event
    ImGui_ImplSdlGL3_ProcessEvent(&sdlEvent);

    switch (sdlEvent.type) {
      case SDL_QUIT:
        windowShouldClose = true;
        break;
      /* Look for a keypress */
      case SDL_KEYDOWN:
        DD_Input::update_keydown(main_input, sdlEvent.key.keysym);
        if (sdlEvent.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
          DD_Terminal::flipDebugFlag();
          flag_debug ^= 1;
        }
        break;
      case SDL_KEYUP:
        DD_Input::update_keyup(main_input, sdlEvent.key.keysym);
        break;
      case SDL_MOUSEBUTTONDOWN:
        DD_Input::update_mouse_button(main_input, sdlEvent.button, true);
        break;
      case SDL_MOUSEBUTTONUP:
        DD_Input::update_mouse_button(main_input, sdlEvent.button, false);
        break;
      case SDL_MOUSEMOTION:
        DD_Input::update_mouse_pos(main_input, sdlEvent.motion);
        break;
      case SDL_MOUSEWHEEL:
        DD_Input::update_mouse_wheel(main_input, sdlEvent.wheel);
        break;
    }
  }
}

/* Call in main function to begin up game loop
 */
void DD_Engine::Run() {
  // push some events to kick start the queue
  DD_LEvent _event;

  // create new screen

  // turn on load screen
  _event.handle = load_hash;
  //add_arg_LEvent<const char *>(&_event, "tag", load_hash.str());
  q_push(_event);
  // add async level assets load
  _event.active = 0;
  // add async level init
  _event.active = 0;
  _event.handle = "_async_call";
  //add_arg_LEvent<const char *>(&_event, "tag", lvl_init_hash.str());
  //add_arg_LEvent<const char *>(&_event, "reciever", "dd_engine");
  //q_push(_event);
  // add frame update
  _event.active = 0;
  _event.handle = "dd_engine";
  //add_arg_LEvent<const char *>(&_event, "tag", frame_enter_hash.str());
  //q_push(_event);

  // exit (debug testing)
  _event.active = 0;
  _event.handle = exit_hash;
  _event.delay = 1000;
  q_push(_event);

  main_q.process_queue();
  /*
  if (init_flag == EngineState::MAIN) {
  // get resource for current level
  lvl_resource = main_lvl[current_lvl]->m_assetFile;
  }

  const double frame_time = 1.0 / FRAME_CAP;
  u64 last_update_time = main_timer.getTime();
  double unprocessed_time = 0.0;

  glClearColor(main_renderer.bgcol[0], main_renderer.bgcol[1],
          main_renderer.bgcol[2], main_renderer.bgcol[3]);
  while (!windowShouldClose) {
  glClear(GL_COLOR_BUFFER_BIT);

  // update time
  if (UNLOCK_FRAMEPACE) {
  main_timer.update();
  } else {
  main_timer.update((float)frame_time);
  }

  // fixed frame loop
  u64 start_time = main_timer.getTime();
  u64 elapsed_time = start_time - last_update_time;
  last_update_time = start_time;

  unprocessed_time += elapsed_time / (double)SECOND_LL;

  // start imgui window processing
  ImGui_ImplSdlGL3_NewFrame(main_window);

  unprocessed_time -= frame_time;
  // pole SDL events
  updateSDL();
  // run event loop
  ResSpace::updateSceneGraph(&main_res, main_timer.getTimeFloat());
  gameLoop();

  if (main_state == GameState::ACTIVE) {
  // add render event
  float frametime = main_timer.getFrameTime();
  DD_Event rendE = DD_Event();
  rendE.m_time = frametime;
  rendE.m_type = "render";
  main_q.push(rendE);
  // add debug (text rendering must be last (gl_blend))
  if (flag_debug) {
    DD_Event debugE = DD_Event();
    debugE.m_time = frametime;
    debugE.m_type = "debug";
    main_q.push(debugE);
  }
  main_q.ProcessQueue();

  // render IMGUI ui
  DD_Terminal::display((float)window_w, (float)window_h);
  ImGui::Render();
  // swap buffers
  SDL_GL_SwapWindow(main_window);
  } else if (main_state == GameState::LOADING) {
  // Show load screen
  main_renderer.DrawLoadScreen(main_timer.getTimeFloat());

  // render IMGUI ui
  DD_Terminal::display((float)window_w, (float)window_h);
  ImGui::Render();
  // swap buffers
  SDL_GL_SwapWindow(main_window);
  } else {
  std::this_thread::sleep_for(chrono_msec);
  }
}
  //*/
}
/*
void DD_Engine::gameLoop() {
  switch (main_state) {
    case GameState::LOADING:
      // ********* start loading resources to RAM *********
      if (!load_assets) {
        if (init_flag == EngineState::VIEWER) {
          lvl_resource = std::string(ROOT_DIR) + "Core/AssetViewer/assets";
        }
        // load resources on separate async thread
        load_RES = std::async(std::launch::async, ResSpace::Load, &main_res,
                              lvl_resource.c_str());
        load_assets = true;
        loading_lvl = false;
        DD_Terminal::post("[status] Initializing level...\n");
      } else if (load_assets && !loading_lvl) {
        // wait for resources to load
        if (load_RES.wait_for(timespan) == std::future_status::ready) {
          main_res.queue = &main_q;
          // load level on separate thread
          switch (init_flag) {
            case EngineState::VIEWER:
              load_LVL = std::async(std::launch::async, &DD_AssetViewer::Load,
                                    &main_viewer);
              break;
            case EngineState::WORLD_BUILDER:
              break;
            case EngineState::MAIN:
              load_LVL = std::async(std::launch::async,
                                    &DD_Engine::InitCurrentLevel, this);
              break;
            default:
              break;
          }
          loading_lvl = true;
          loading_agents = false;
          DD_Terminal::post("[status] Loading assets to RAM...\n");
        }
      } else if (loading_lvl && !loading_agents) {
        if (load_LVL.wait_for(timespan) == std::future_status::ready) {
          // load agents to RAM on separate thread
          load_RAM = std::async(std::launch::async,
                                ResSpace::loadAgentsToMemory, &main_res);
          loading_agents = true;
        }
      }
      // ********* start loading resources to GPU *********
      else if (loading_agents && !async_done) {
        if (load_RAM.wait_for(timespan) == std::future_status::ready) {
          async_done = true;
          DD_Terminal::post("[status] Loading assets to GPU\n");
        }
      } else if (async_done) {
        if (p_tex_loading) {
          // load all particle textures
          p_tex_loading = main_renderer.m_particleSys->Init();
        } else {
          // finished loading particle textures
          if (lvl_agents < main_res.m_num_agents) {
            ResSpace::loadAgentToGPU(&main_res, lvl_agents);
            lvl_agents += 1;
          } else {
            main_renderer.LoadEngineAssets();
            DD_Terminal::post("[status] Finished loading to GPU\n");
            main_state = GameState::ACTIVE;
          }
        }
      }
      break;
    case GameState::PAUSE:
      break;
    case GameState::ACTIVE:
      // ACTIVE GAME STATE
      LoadQueue();
      main_q.ProcessQueue();
      break;
    default:
      break;
  }
}
//*/

/// \brief Clean up engine resources
void DD_Engine::cleanUp() {
  DD_Terminal::outTerminalHistory();  // save history
  cleanUpContexts();
  // shutdown SDL2
  SDL_Quit();
}

/*
void DD_Engine::LoadQueue() {
  // THINGS TO DO ONCE PER FRAME IN ORDER
  float frametime = main_timer.getFrameTime();
  float gametime = main_timer.getTimeFloat();

  DD_Event inputE;
  inputE.m_total_runtime = gametime;
  inputE.m_time = frametime;
  inputE.m_type = "input";
  inputE.m_message = main_input.GetInput();
  main_q.push(inputE);

  DD_Event vrE;
  vrE.m_total_runtime = gametime;
  vrE.m_time = frametime;
  vrE.m_type = "VR";
  main_q.push(vrE);

  // update Viewer (if active)
  if (init_flag == EngineState::VIEWER) {
    DD_Event vrV;
    vrV.m_total_runtime = gametime;
    vrV.m_time = frametime;
    vrV.m_type = "viewer";
    main_q.push(vrV);
  }

  // update AI
  DD_Event aiE;
  aiE.m_total_runtime = gametime;
  aiE.m_time = frametime;
  aiE.m_type = "update_AI";
  main_q.push(aiE);

  // query for posts
  main_q.GetPosts("post", frametime, gametime);
  main_q.GetPosts("post_compute", frametime, gametime);

  // process terminal commands
  bool more_cmds = true;
  while (more_cmds) {
    const char* cmd = DD_Terminal::pollBuffer();
    more_cmds = execTerminal(cmd, frametime);
  }

  // update animation (last thing to update before rendering)
  DD_Event animE;
  animE.m_time = frametime;
  animE.m_total_runtime = gametime;
  animE.m_type = "update_animation";
  main_q.push(animE);
}
//*/

// void DD_Engine::execScript(std::string script_file) {
//  DD_IOhandle script;
//  if (!script.open(script_file.c_str(), DD_IOflag::READ)) {
//    return;
//  }
//
//  const char* line = script.readNextLine();
//  while (line) {
//    execTerminal(line);
//    line = script.readNextLine();
//  }
//}

bool DD_Engine::execTerminal(const char *cmd) {
  if (cmd) {
    cbuff<32> str_arg;

    // split arguments and tags if possible
    std::string head;
    std::string buff = cmd;
    size_t head_idx = buff.find_first_of(" ");
    if (head_idx != std::string::npos) {
      head = buff.substr(0, head_idx);
      str_arg = buff.substr(head_idx + 1).c_str();
    } else {
      head = buff;
    }

    DD_LEvent _event;
    _event.handle = head.c_str();
    add_arg_LEvent<const char *>(&_event, "tag", str_arg.str());
    q_push(_event);
    return true;
  } else {
    return false;
  }
}

void DD_Engine::update(DD_LEvent &_event) {
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

    openWindow(window_w, window_h, init_options);
    // set useful lua globals
    set_lua_global(main_lstate, "WINDOW_WIDTH", (int64_t)window_w);
    set_lua_global(main_lstate, "WINDOW_HEIGHT", (int64_t)window_h);
  } else if (e_sig == frame_enter_hash.gethash()) {  // setup new frame
    // clear framebuffer
    // glClearColor(main_renderer.bgcol[0], main_renderer.bgcol[1],
    // main_renderer.bgcol[2], main_renderer.bgcol[3]);
    // glClear(GL_COLOR_BUFFER_BIT);

    DD_Time::update();

    // start imgui window processing
    ImGui_ImplSdlGL3_NewFrame(main_window);
    // pole SDL events
    updateSDL();
    // update terminal button presses

    // run scene graph
    // ResSpace::updateSceneGraph(&main_res, main_timer.getTimeFloat());

    if (load_screen) {
      // Show load screen
      // main_renderer.DrawLoadScreen(main_timer.getTimeFloat());
    } else {
      // send next event in sequence
      // terminal, VR, AI, level update,
      // post update (periodic update for scripts),
      // physics, animation, render, frame exit
      DD_LEvent new_event;
    }
  } else if (e_sig == frame_exit_hash.gethash()) {  // exit frame
    // query terminal
    DD_Terminal::display((float)window_w, (float)window_h);
    // render IMGUI ui
    ImGui::Render();
    // swap buffers
    SDL_GL_SwapWindow(main_window);
    // push event for starting next frame
    DD_LEvent new_event;
    new_event.handle = "dd_engine";
    add_arg_LEvent<const char *>(&new_event, "tag", frame_enter_hash.str());
    q_push(new_event);
  } else if (e_sig == load_hash.gethash()) {  // load screen
    load_screen ^= 1;
  } else if (e_sig == terminal_hash.gethash()) {  // process queue
    bool more_cmds = true;
    while (more_cmds) {
      const char *cmd = DD_Terminal::pollBuffer();
      more_cmds = execTerminal(cmd);
    }
  } else if (e_sig == lvl_init_hash.gethash()) {  // set level attributes
    // add functionality to set active skybox in scripts
    // add function from DD_Engine that sets: main_renderer.m_lvl_cubMap
  }
}

/*
void DD_Engine::InitCurrentLevel() {
  if (init_flag != EngineState::MAIN) {
    return;
  }

  DD_GameLevel* lvl = main_lvl[current_lvl];
  // log dimensions
  lvl->m_screenH = window_h;
  lvl->m_screenW = window_w;
  // log resource bin
  lvl->res_ptr = &main_res;
  lvl->Init();
  // register handlers
  size_t range = lvl->num_handlers;
  for (size_t i = 0; i < range; i++) {
    if (lvl->tickets[i] == "post") {
      main_q.RegisterPoster(lvl->handlers[i]);
    } else {
      main_q.RegisterHandler(lvl->handlers[i], lvl->tickets[i].c_str());
    }
  }
  // skybox
  if (main_lvl[current_lvl]->m_cubeMapID != "") {
    main_renderer.m_lvl_cubMap = main_lvl[current_lvl]->m_cubeMapID;
  }
  // vr attributes
  main_renderer.m_scrHorzDist = main_lvl[current_lvl]->m_scrHorzDist;
  main_renderer.m_scrVertDist = main_lvl[current_lvl]->m_scrVertDist;
  main_renderer.m_flagDCM = main_lvl[current_lvl]->m_flagDynamicCubeMap;
}
//*/
