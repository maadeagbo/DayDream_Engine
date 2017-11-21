#include "DD_Engine.h"
#include "DD_Terminal.h"
#include "DD_FileIO.h"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

namespace {
	bool windowShouldClose = false;
	size_t lvl_agents = 0;

	bool loading_lvl = false, async_done = false, p_tex_loading = true,
		loading_agents = false;
	std::future<void> load_RES, load_RAM, load_LVL;
	std::chrono::milliseconds timespan(10);
	std::chrono::milliseconds chrono_msec(1);

	const bool UNLOCK_FRAMEPACE = true;
	const size_t FRAME_CAP = 100;
	const u64 SECOND_LL = 1000000000LL;

	int resolution_set[2] = { 1400, 800 };
	bool engine_mode_flags[] = { true, false, false, false };
}

void DD_Engine::openWindow(const size_t width,
					  const size_t height,
					  EngineMode mode)
{
	m_WIDTH = (int)width;
	m_HEIGHT = (int)height;

	u32 flags = SDL_WINDOW_OPENGL;
	int monitor_choice = 0, vsync_option = 0;

	// engine flags
	if( bool(mode & EngineMode::DD_DEBUG) ) {
		flag_debug = true;
		DD_Terminal::flipDebugFlag();
	}
	if( bool(mode & EngineMode::DD_NO_CONSOLE) ) {
#ifdef WIN32
		FreeConsole();
#endif
	}
	if( bool(mode & EngineMode::DD_FULLSCREEN) ) {
		flags |= SDL_WINDOW_FULLSCREEN;
		//flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; //use this for fake fullscreen
	}
	if( bool(mode & EngineMode::DD_SECOND_DISPLAY) ) {
		monitor_choice = 1;
	}
	if( bool(mode & EngineMode::DD_VSYNC) ) {
		vsync_option = 1;
	}

	// init SDL2
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Failed to init SDL\n");
		std::exit(EXIT_FAILURE);
	}
	// create centered window (set hints before)
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	main_window = SDL_CreateWindow(
		"DayDream Engine Alpha",
		SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_choice),
		SDL_WINDOWPOS_CENTERED_DISPLAY(monitor_choice),
		m_WIDTH,
		m_HEIGHT,
		flags
	);
	// reset base width and height incase fullscreen set
	SDL_GetWindowSize(main_window, &m_WIDTH, &m_HEIGHT);

	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(main_window);

	// set opengl version
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated
	// functions are disabled
	SDL_GL_SetAttribute(
		SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(
		SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	main_glcontext = SDL_GL_CreateContext(main_window);
	if( main_glcontext == NULL ) {
		fprintf(stderr,
			"Failed to create OpenGL 4.3 context and double buffering\n");
		std::exit(EXIT_FAILURE);
	}
	// This makes our buffer swap un-syncronized with the monitor's vertical refresh
	SDL_GL_SetSwapInterval(vsync_option);

	if( !InitGLLoadGen(bool(mode & EngineMode::DD_GPU_INFO)) ) {
		fprintf(stderr, "Failed to load OpenGL generator\n");
		std::exit(EXIT_FAILURE);
	}
}

/* Load OpenGL generator
 * \param displayInfo Set to true to display info of system
*/
bool DD_Engine::InitGLLoadGen(const bool displayInfo)
{
	// Initialize GLLoadGen to setup the OpenGL Function pointers
	int loaded = ogl_LoadFunctions();
	if( loaded == ogl_LOAD_FAILED ) {
		int num_failed = loaded - ogl_LOAD_SUCCEEDED;
		std::string temp = "Number of functions that failed to load: " +
			std::to_string(num_failed) + ".\n";
		DD_Terminal::post(temp);
		return false;
	}

	if( displayInfo ) {
		// check opengl version supported
		const GLubyte *renderer = glGetString(GL_RENDERER);
		const GLubyte *vendor = glGetString(GL_VENDOR);
		const GLubyte *version = glGetString(GL_VERSION);
		const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
		GLint major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);

		// check number of color attachments and drawbuffer attachments
		GLint maxAttach = 0, maxDrawBuf = 0;;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuf);

		// check compute shader limitations
		int maxComputeWG_COUNT_0 = 0,
			maxComputeWG_COUNT_1 = 0,
			maxComputeWG_COUNT_2 = 0,
			maxComputeWG_SIZE_0 = 0,
			maxComputeWG_SIZE_1 = 0,
			maxComputeWG_SIZE_2 = 0,
			maxComputeWG_INVOCATIONS = 0;

		glGetIntegeri_v(
			GL_MAX_COMPUTE_WORK_GROUP_COUNT,
			0,
			&maxComputeWG_COUNT_0);
		glGetIntegeri_v(
			GL_MAX_COMPUTE_WORK_GROUP_COUNT,
			1,
			&maxComputeWG_COUNT_1);
		glGetIntegeri_v(
			GL_MAX_COMPUTE_WORK_GROUP_COUNT,
			2,
			&maxComputeWG_COUNT_2);
		glGetIntegeri_v(
			GL_MAX_COMPUTE_WORK_GROUP_SIZE,
			0,
			&maxComputeWG_SIZE_0);
		glGetIntegeri_v(
			GL_MAX_COMPUTE_WORK_GROUP_SIZE,
			1,
			&maxComputeWG_SIZE_1);
		glGetIntegeri_v(
			GL_MAX_COMPUTE_WORK_GROUP_SIZE,
			2,
			&maxComputeWG_SIZE_2);
		glGetIntegerv(
			GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
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
void DD_Engine::CleanUpSDLContext()
{
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
void DD_Engine::Launch()
{
	switch (init_flag)
	{
	case EngineState::VIEWER:
		Load();
		LoadViewer();
		break;
	case EngineState::WORLD_BUILDER:
		break;
	case EngineState::MAIN:
		Load();
		break;
	default:
		break;
	}
	// load terminal history
	DD_Terminal::inTerminalHistory();
}

/// \brief Select level and set parameters
bool DD_Engine::LevelSelect(const size_t w, const size_t h) {
	openWindow(
		w, h, EngineMode::DD_GPU_INFO | EngineMode::DD_VSYNC
	);
	dd_array<const char*> lvls_list(num_lvls);
	for (size_t i = 0; i < num_lvls; i++) {
		lvls_list[i] = main_lvl[i]->m_lvlID.c_str();
	}

	init_flag = EngineState::MAIN;
	bool launch = false; 
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	int wh_idx[2] = { 8, 4 };

	while(!windowShouldClose && !launch) {
		glClear(GL_COLOR_BUFFER_BIT);
		// start imgui window processing
		ImGui_ImplSdlGL3_NewFrame(main_window);

		// pole SDL events
		updateSDL();

		// Query for level to load
		float scrW = (float)w/1.25f;
		float scrH = (float)h/1.1f;
		ImGui::SetNextWindowPos(ImVec2(w/2 - scrW/2, h/2 - scrH/2));
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
			ImGui::ListBox("<-- Select Scene", &current_lvl,
						   &lvls_list[0], (int)lvls_list.size(), 4);
		}
		else {
			ImGui::Text("No levels present");
		}

		const int choices = 13;
		const char* res_[choices] = { 
			"320", "480", "620", "720", "800", "1024", "1080", "1280",
			"1440", "1600", "1920", "2048", "3200"
		};
		ImGui::Combo("Width", &wh_idx[0], res_, choices); 
		//ImGui::SameLine();
		ImGui::Combo("Height", &wh_idx[1], res_, choices);

		//ImGui::DragInt2("<-- Resolution", resolution_set, 10.f, 100);
		m_WIDTH = (int)strtod(res_[wh_idx[0]], nullptr);
		m_HEIGHT = (int)strtod(res_[wh_idx[1]], nullptr);

		ImGui::Checkbox("VSYNC", &engine_mode_flags[0]); ImGui::SameLine();
		ImGui::Checkbox("Full Screen", &engine_mode_flags[1]); ImGui::SameLine();
		ImGui::Checkbox("Second Display", &engine_mode_flags[2]);
		ImGui::Checkbox("(Windows) Close Console", &engine_mode_flags[3]);

		ImGui::Dummy(ImVec2(0, 10));
		if (lvls_list.size() > 0) {
			ImGui::Dummy(ImVec2(scrW / 2 - 60 / 2 - 10, 20)); ImGui::SameLine();
			launch = ImGui::Button("Launch", ImVec2(60, 20));
		}
		ImGui::Dummy(ImVec2(scrW / 2 - 180 / 2 - 10, 20)); ImGui::SameLine();
		if (ImGui::Button("Launch Asset Viewer", ImVec2(180, 20))) {
			launch = true;
			init_flag = EngineState::VIEWER;
		};

		ImGui::End();
		ImGui::Render();
		// swap buffers
		SDL_GL_SwapWindow(main_window);
	}

	DD_Terminal::clearTerminal();
	cleanUpContexts();
	return launch;
}

void DD_Engine::Load()
{
	EngineMode init_options = EngineMode::DD_NOT_SET;
	init_options = (engine_mode_flags[0]) ?
		EngineMode(init_options | EngineMode::DD_VSYNC) : init_options;
	init_options = (engine_mode_flags[1]) ?
		EngineMode(init_options | EngineMode::DD_FULLSCREEN) : init_options;
	init_options = (engine_mode_flags[2]) ?
		EngineMode(init_options | EngineMode::DD_SECOND_DISPLAY) : init_options;
	init_options = (engine_mode_flags[3]) ?
		EngineMode(init_options | EngineMode::DD_NO_CONSOLE) : init_options;

	openWindow( m_WIDTH, m_HEIGHT, init_options );
	// Load resources
	main_res.queue = &main_q;
	// set up math/physics library
	DD_MathLib::setResourceBin(&main_res);
	// Load Renderer
	//main_renderer = new DD_Renderer();
	main_renderer.m_resourceBin = &main_res;
	main_renderer.m_time = &main_timer;
	main_renderer.LoadRendererEngine((GLfloat)m_WIDTH, (GLfloat)m_HEIGHT);
	main_renderer.m_particleSys->m_resBin = &main_res;
	// load compute unit
	main_comp.init();

	// add compute handler
	main_comp.res_ptr = &main_res;
	EventHandler handlerC = std::bind(&DD_Compute::Compute, &main_comp,
									  std::placeholders::_1);
	main_q.RegisterHandler(handlerC, "compute_texA");
	main_q.RegisterPoster(handlerC);

	// initialize Animation system and register handlers
	main_animator.res_ptr = &main_res;
	EventHandler handlerAn = std::bind(&DD_AnimSystem::update, 
									   &main_animator,
									   std::placeholders::_1);
	main_q.RegisterHandler(handlerAn, "update_animation");

	// add render engine handler
	EventHandler handlerR = std::bind(&DD_Renderer::RenderHandler, 
									  &main_renderer,
									  std::placeholders::_1);
	main_q.RegisterHandler(handlerR, "render");
	main_q.RegisterHandler(handlerR, "toggle_screenshots");
	main_q.RegisterHandler(handlerR, "rendstat");

	// add particle handler
	EventHandler handlerP = std::bind(&DD_ParticleSys::Create,
									  main_renderer.m_particleSys, 
									  std::placeholders::_1);
	EventHandler handlerP2 = std::bind(&DD_ParticleSys::AddJobToQueue,
									   main_renderer.m_particleSys, 
									   std::placeholders::_1);
	main_q.RegisterHandler(handlerP, "generate_emitter");
	main_q.RegisterHandler(handlerP, "create_cloth");
	main_q.RegisterHandler(handlerP, "debug");
	main_q.RegisterHandler(handlerP, "system_pause");
	main_q.RegisterHandler(handlerP, "update_cloth");
	main_q.RegisterHandler(handlerP2, "particle_jobA");
	main_q.RegisterPoster(handlerP);

	// add AI program handler and setup
	main_ai.res_ptr = &main_res;
	EventHandler handlerA = std::bind(&DD_AISystem::update, &main_ai,
									  std::placeholders::_1);
	main_q.RegisterHandler(handlerA, "update_AI");

	// terminal input callback
	EventHandler handlerT = std::bind(&DD_Terminal::getInput, 
									  std::placeholders::_1);
	main_q.RegisterHandler(handlerT, "input");
	main_q.main_clock = &main_timer;

	// add engine callback
	EventHandler handlerE = std::bind(&DD_Engine::Update, 
									  this, 
									  std::placeholders::_1);
	main_q.RegisterHandler(handlerE, "EXIT");
}

/// \brief Load engine compoenets for DD_AssetViewer
void DD_Engine::LoadViewer()
{
	// initialize Asset viewer
	// log dimensions
	main_viewer.m_screenH = m_HEIGHT;
	main_viewer.m_screenW = m_WIDTH;
	// log resource bin
	main_viewer.res_ptr = &main_res;
	// register handlers
	EventHandler handlerV = std::bind(&DD_AssetViewer::Update, &main_viewer,
		std::placeholders::_1);
	main_q.RegisterHandler(handlerV, "viewer");
	main_q.RegisterHandler(handlerV, "new_mdl");
	main_q.RegisterHandler(handlerV, "new_sk");
	main_q.RegisterHandler(handlerV, "new_mdlsk");
	main_q.RegisterHandler(handlerV, "new_agent_sk");
	main_q.RegisterHandler(handlerV, "new_anim");
}

void DD_Engine::updateSDL()
{
	// get keyboard events
	SDL_Event sdlEvent;
	while( SDL_PollEvent(&sdlEvent) ) {
		// imgui process event
		ImGui_ImplSdlGL3_ProcessEvent(&sdlEvent);

		switch( sdlEvent.type ) {
			case SDL_QUIT:
				windowShouldClose = true;
				break;
				/* Look for a keypress */
			case SDL_KEYDOWN:
				main_input.UpdateKeyDown(sdlEvent.key.keysym);
				if( sdlEvent.key.keysym.scancode == SDL_SCANCODE_GRAVE ) {
					DD_Terminal::flipDebugFlag();
					flag_debug ^= 1;
				}
				break;
			case SDL_KEYUP:
				main_input.UpdateKeyUp(sdlEvent.key.keysym);
				break;
			case SDL_MOUSEBUTTONDOWN:
				main_input.UpdateMouse(sdlEvent.button, true);
				break;
			case SDL_MOUSEBUTTONUP:
				main_input.UpdateMouse(sdlEvent.button, false);
				break;
			case SDL_MOUSEMOTION:
				main_input.UpdateMousePos(sdlEvent.motion);
				break;
			case SDL_MOUSEWHEEL:
				main_input.UpdateMouseWheel(sdlEvent.wheel);
				break;
		}
	}
}

/* Call in main function to begin up game loop
*/
void DD_Engine::Run()
{
	if (init_flag == EngineState::MAIN) {
		// get resource for current level
		lvl_resource = main_lvl[current_lvl]->m_assetFile;
	}

	const double frame_time = 1.0 / FRAME_CAP;
	u64 last_update_time = main_timer.getTime();
	double unprocessed_time = 0.0;

	glClearColor(main_renderer.bgcol[0], 
				 main_renderer.bgcol[1], 
				 main_renderer.bgcol[2], 
				 main_renderer.bgcol[3]);
	while( !windowShouldClose ) {
		glClear(GL_COLOR_BUFFER_BIT);

		// update time
		if( UNLOCK_FRAMEPACE ) {
			main_timer.update();
		}
		else {
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

		if( main_state == GameState::ACTIVE) {
			// add render event
			float frametime = main_timer.getFrameTime();
			DD_Event rendE = DD_Event();
			rendE.m_time = frametime;
			rendE.m_type = "render";
			main_q.push(rendE);
			// add debug (text rendering must be last (gl_blend))
			if( flag_debug ) {
				DD_Event debugE = DD_Event();
				debugE.m_time = frametime;
				debugE.m_type = "debug";
				main_q.push(debugE);
			}
			main_q.ProcessQueue();

			// render IMGUI ui
			DD_Terminal::display((float)m_WIDTH, (float)m_HEIGHT);
			ImGui::Render();
			// swap buffers
			SDL_GL_SwapWindow(main_window);
		}
		else if( main_state == GameState::LOADING) {
			// Show load screen
			main_renderer.DrawLoadScreen(main_timer.getTimeFloat());

			// render IMGUI ui
			DD_Terminal::display((float)m_WIDTH, (float)m_HEIGHT);
			ImGui::Render();
			// swap buffers
			SDL_GL_SwapWindow(main_window);
		}
		else { std::this_thread::sleep_for(chrono_msec); }
	}
}

void DD_Engine::gameLoop()
{
	switch( main_state ) {
		case GameState::LOADING:
			// ********* start loading resources to RAM *********
			if( !load_assets ) {
				if (init_flag == EngineState::VIEWER) {
					lvl_resource = 
						std::string(ROOT_DIR) + "Core/AssetViewer/assets";
				}
				// load resources on separate async thread
				load_RES = std::async(std::launch::async, 
									  ResSpace::Load, 
									  &main_res,
									  lvl_resource.c_str());
				load_assets = true;
				loading_lvl = false;
				DD_Terminal::post("[status] Initializing level...\n");
			}
			else if( load_assets && !loading_lvl ) {
				// wait for resources to load
				if( load_RES.wait_for(timespan) == std::future_status::ready ) {
					main_res.queue = &main_q;
					// load level on separate thread
					switch (init_flag) {
						case EngineState::VIEWER:
							load_LVL = std::async(std::launch::async,
												  &DD_AssetViewer::Load,
												  &main_viewer);
							break;
						case EngineState::WORLD_BUILDER:
							break;
						case EngineState::MAIN:
							load_LVL = std::async(std::launch::async,
												  &DD_Engine::InitCurrentLevel,
												  this);
							break;
						default:
							break;
					}
					loading_lvl = true;
					loading_agents = false;
					DD_Terminal::post("[status] Loading assets to RAM...\n");
				}
			}
			else if( loading_lvl && !loading_agents ) {
				if( load_LVL.wait_for(timespan) == std::future_status::ready ) {
					// load agents to RAM on separate thread
					load_RAM = std::async(std::launch::async,
										  ResSpace::loadAgentsToMemory, 
										  &main_res);
					loading_agents = true;
				}
			}
			// ********* start loading resources to GPU *********
			else if( loading_agents && !async_done ) {
				if( load_RAM.wait_for(timespan) == std::future_status::ready ) {
					async_done = true;
					DD_Terminal::post("[status] Loading assets to GPU\n");
				}
			}
			else if( async_done ) {
				if( p_tex_loading ) {
					// load all particle textures
					p_tex_loading = main_renderer.m_particleSys->Init();
				}
				else {
					// finished loading particle textures
					if( lvl_agents < main_res.m_num_agents ) {
						ResSpace::loadAgentToGPU(&main_res, lvl_agents);
						lvl_agents += 1;
					}
					else {
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

/// \brief Clean up engine resources
void DD_Engine::cleanUp() 
{
	DD_Terminal::outTerminalHistory(); // save history
	cleanUpContexts();
	// shutdown SDL2
	SDL_Quit();
}

void DD_Engine::AddLevel(DD_GameLevel * level,
						 const char* assetLoc,
						 const char* ID)
{
	main_lvl[num_lvls] = std::move(level);
	main_lvl[num_lvls]->m_lvlID = ID;
	main_lvl[num_lvls]->m_assetFile = std::string(PROJECT_DIR) + assetLoc;
	num_lvls += 1;
}

void DD_Engine::LevelAssets(const char * assets_file)
{
	lvl_resource = assets_file;
}

void DD_Engine::LoadQueue()
{
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

void DD_Engine::execScript(std::string script_file, float frametime)
{
	DD_IOhandle script;
	if (!script.open(script_file.c_str(), DD_IOflag::READ)) {
		return;
	}

	const char *line = script.readNextLine();
	while (line) {
		execTerminal(line, frametime);
		line = script.readNextLine();
	}
}

bool DD_Engine::execTerminal(const char* cmd, float frametime)
{
	if (cmd) {
		DD_Event cmdE = DD_Event();
		messageBuff* m = new messageBuff();

		// split arguments and commands if possible
		std::string head, tail, buff;
		buff = cmd;
		size_t head_idx = buff.find_first_of(" ");
		if (head_idx != std::string::npos) {
			head = buff.substr(0, head_idx);
			tail = buff.substr(head_idx + 1);
			snprintf(m->message512, 512, "%s", tail.c_str());
		}
		else {
			head = buff; tail = "";
			m->message512[0] = '\0';
		}

		//override with scripting
		if (head == ".") {
			execScript(tail, frametime);
		}
		else {
			cmdE.m_message = m;
			cmdE.m_type = head;
			cmdE.m_time = frametime;
			main_q.push(cmdE);
		}
		return true;
	}
	else {
		return false;
	}
}

DD_Event DD_Engine::Update(DD_Event & event)
{
	if( event.m_type.compare("EXIT") == 0 ) {
		flagBuff* fb = static_cast<flagBuff*>(event.m_message);
		windowShouldClose = fb->flag;
	}
	return DD_Event();
}

void DD_Engine::InitCurrentLevel()
{
	if (init_flag != EngineState::MAIN) { return; }

	DD_GameLevel* lvl = main_lvl[current_lvl];
	// log dimensions
	lvl->m_screenH = m_HEIGHT;
	lvl->m_screenW = m_WIDTH;
	// log resource bin
	lvl->res_ptr = &main_res;
	lvl->Init();
	// register handlers
	size_t range = lvl->num_handlers;
	for( size_t i = 0; i < range; i++ ) {
		if( lvl->tickets[i] == "post" ) {
			main_q.RegisterPoster(lvl->handlers[i]);
		}
		else {
			main_q.RegisterHandler(lvl->handlers[i], lvl->tickets[i].c_str());
		}
	}
	// skybox
	if( main_lvl[current_lvl]->m_cubeMapID != "" ) {
		main_renderer.m_lvl_cubMap = main_lvl[current_lvl]->m_cubeMapID;
	}
	// vr attributes
	main_renderer.m_scrHorzDist = main_lvl[current_lvl]->m_scrHorzDist;
	main_renderer.m_scrVertDist = main_lvl[current_lvl]->m_scrVertDist;
	main_renderer.m_flagDCM = main_lvl[current_lvl]->m_flagDynamicCubeMap;
}
