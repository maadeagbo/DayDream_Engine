#include "CrowdSim.h"

enum class CrowdScene { _1, _2, _3 };

namespace {
	const size_t max_agents = 100;
	ControllerC* myControl;
	DD_Agent *ground, *markerG, *markerY;
	cbuff<32> obstacle01_id;
	cbuff<32> main_ai_id;
	cbuff<32> wall_mdl_id;
	/*
		ground_id, 
		obstacle01_id, 
		markerG_id, 
		markerY_id,
		main_ai_id;*/\
	AI_Agent* myAgents[max_agents];

	unsigned red_mat_idx = 0;
	unsigned purple_mat_idx = 0;
	unsigned orange_mat_idx = 0;

	size_t num_agents = 0;
	const float agent_radius = 0.5f;
	bool scene_lock = true;

	cbuff<16> aica_name;
	cbuff<16> aiag_name;
	CrowdScene scene_selected;
}

namespace gui {
	float new_origin[2], new_goal[2];
	bool origin_flag = true, goal_flag = false, simulate_flag = true,
		bot_crowd = true, top_crowd = true, dynamic_flag = false;
}

void manhattan_dist(const glm::vec3 origin, const glm::vec3 goal,
					const dd_array<glm::vec3>& points, dd_array<float>& h_result)
{
	for (size_t i = 1; i < points.size(); i++) {
		h_result[i] = glm::distance(points[i], goal);
	}
}

void createDenseCrowd(dd_array<glm::vec3>& bin, const float min_x, const float min_z,
	const float width, const float height, const int size);


void CrowdSim::Init() {
	// register skybox map
	//m_cubeMapID = "water";
	// add mouse controller to level
	myControl = new ControllerC("avatar");
	AddAgent(myControl);
	myControl->clickedP = false;
	myControl->clickedT = false;
	myControl->clickedSpace = false;
	myControl->UpdatePosition(glm::vec3(2500.0f, 1200.0f, 0.0f));
	myControl->initRot(glm::radians(-25.f), glm::radians(90.f));

	// Add shadow light
	DD_Light* light = ResSpace::getNewDD_Light(res_ptr, "shadow");
	light->_position = glm::vec3(1000.0f, 1000.f, 2000.0f);
	light->m_color = glm::vec3(0.1f);
	light->m_flagShadow = true;

	// add generic camera
	DD_Camera* cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.0f;
	cam->far_plane = 10000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	//  callbacks and posts
	EventHandler a = std::bind(&CrowdSim::basePost, this, std::placeholders::_1);
	AddCallback("post", a);
	AddCallback("clear_slate", a);
	AddCallback("run_scene_01", a);
	AddCallback("run_scene_02", a);
	AddCallback("run_scene_03", a);

	// add floor
	const float floor_scale = 20.f;
	DD_Material* mat0 = ResSpace::getNewDD_Material(res_ptr, "my_mat_00");
	mat0->m_base_color = glm::vec4(1.f, 1.f, 1.f, 1.f);
	int m_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_00");
	m_idx = (m_idx == -1) ? 0 : m_idx;
	ground = ResSpace::getNewDD_Agent(res_ptr, "Ground");
	ground->AddModel("plane_prim", 0.f, 1000.f);
	ground->UpdateScale(glm::vec3(floor_scale, 1.f, floor_scale + 10.f));
	ground->SetMaterial(m_idx);

	// change agent materials
	DD_Material* mat = ResSpace::getNewDD_Material(res_ptr, "my_mat_red");
	mat->m_base_color = glm::vec4(1.f, 0.1f, 0.1f, 1.f);
	red_mat_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_red");
	red_mat_idx = (red_mat_idx == -1) ? 0 : red_mat_idx;
	// purple
	mat = ResSpace::getNewDD_Material(res_ptr, "my_mat_purp");
	mat->m_base_color = glm::vec4(1.f, 0.1f, 1.f, 1.f);
	purple_mat_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_purp");
	purple_mat_idx = (purple_mat_idx == -1) ? 0 : purple_mat_idx;
	// orange
	mat = ResSpace::getNewDD_Material(res_ptr, "my_mat_orange");
	mat->m_base_color = glm::vec4(1.f, 0.6f, 0.2f, 1.f);
	orange_mat_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_orange");
	orange_mat_idx = (orange_mat_idx == -1) ? 0 : orange_mat_idx;

	// change ground material
	DD_Material* mat2 = ResSpace::getNewDD_Material(res_ptr, "my_mat_02");
	mat2->m_base_color = glm::vec4(0.9f, 0.9f, 0.9f, 1.f);
	m_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_02");
	m_idx = (m_idx == -1) ? 0 : m_idx;
	ground->SetMaterial(m_idx);

	// create primitive object for start/end markers
	markerG = ResSpace::getNewDD_Agent(res_ptr, "green_node");
	markerG->AddModel("sphere_prim", 0.f, 1000.f);
	markerG->UpdateScale(glm::vec3(0.3f));
	DD_Material* mat3 = ResSpace::getNewDD_Material(res_ptr, "my_mat_03");
	mat3->m_base_color = glm::vec4(0.f, 1.f, 0.f, 1.f);
	m_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_03");
	m_idx = (m_idx == -1) ? 0 : m_idx;
	markerG->SetMaterial(m_idx);

	markerY = ResSpace::getNewDD_Agent(res_ptr, "blue_node");
	markerY->AddModel("sphere_prim", 0.f, 1000.f);
	markerY->UpdateScale(glm::vec3(0.3f));
	DD_Material* mat4 = ResSpace::getNewDD_Material(res_ptr, "my_mat_04");
	mat4->m_base_color = glm::vec4(0.f, 1.f, 1.f, 1.f);
	m_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_04");
	m_idx = (m_idx == -1) ? 0 : m_idx;
	markerY->SetMaterial(m_idx);

	// id names
	obstacle01_id = "Obstacle";
	main_ai_id = "main_ai";
}

// Setup imgui interface
void CrowdSim::setInterface() {
	// get io for mouse & keyboard management
	ImGuiIO& imgui_io = ImGui::GetIO();
	myControl->ignore_controls = imgui_io.WantCaptureMouse;

	ImGuiWindowFlags window_flags = 0;
	ImGui::Begin("Agent Management", nullptr, window_flags);

	if (ImGui::RadioButton("Start", gui::origin_flag)) {
		gui::origin_flag ^= 1;
		gui::goal_flag = !gui::origin_flag;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("End", gui::goal_flag)) {
		gui::goal_flag ^= 1;
		gui::origin_flag = !gui::goal_flag;
	}
	ImGui::SameLine();
	ImGui::Text("   ");
	ImGui::SameLine();
	if (ImGui::Button("Create Agent")) {
		float _y = 25.f;
		glm::vec3 _pos1 = glm::vec3(gui::new_origin[0], _y, gui::new_origin[1]);
		glm::vec3 _pos2 = glm::vec3(gui::new_goal[0], _y, gui::new_goal[1]);

		createNewAgent(_pos1, _pos2, red_mat_idx);
	}
	DD_AIObject* main_ai = ResSpace::findDD_AIObject(res_ptr, main_ai_id.str());
	if (main_ai) {
		if (main_ai->flag_render) {
			if (ImGui::Button("Hide PRM")) { main_ai->flag_render ^= 1; }
		}
		else {
			if (ImGui::Button("Show PRM")) { main_ai->flag_render ^= 1; }
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Simulate")) {
		for (size_t i = 0; i < num_agents; i++) {
			myAgents[i]->plan_now = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Dynamic")) {
		gui::dynamic_flag ^= 1;
		for (size_t i = 0; i < num_agents; i++) {
			myAgents[i]->replan = gui::dynamic_flag;
		}
	}
	ImGui::SameLine();
	if (gui::simulate_flag) {
		if (ImGui::Button("Move")) {
			for (size_t i = 0; i < num_agents; i++) {
				myAgents[i]->move_now = true;
			}
			gui::simulate_flag = false;
		}
	}
	else {
		if (ImGui::Button("Reset")) {
			for (size_t i = 0; i < num_agents; i++) {
				myAgents[i]->current_pos = myAgents[i]->origin;
				myAgents[i]->move_now = false;
				myAgents[i]->path_index = 0;
			}
			gui::simulate_flag = true;
		}
	}
	if (gui::bot_crowd) {
		if (ImGui::Button("Generate Crowd (Bot)")) {
			// get corners of the floor
			BoundingBox bb = ground->BBox.transformCorners(ground->inst_m4x4[0]);
			glm::vec3 bot_corner_l = glm::vec3(bb.min.x, 0.f, bb.max.z);
			glm::vec3 top_corner_r = glm::vec3(bb.max.x, 0.f, bb.min.z);
			float _w = abs(bot_corner_l.x - top_corner_r.x);
			float _h = abs(bot_corner_l.z - top_corner_r.z);

			const size_t cr_size = 30;
			dd_array<glm::vec3> bin = dd_array<glm::vec3>(1);
			switch (scene_selected) {
				case CrowdScene::_1:
					createDenseCrowd(bin, 
									 bot_corner_l.x + (_w * 0.15f),
									 bot_corner_l.z - (_h * 0.2f), 
									 _w * 0.8f, 
									 _h * 0.2f, 
									 cr_size);
					break;
				case CrowdScene::_2:
					createDenseCrowd(bin, 
									 bot_corner_l.x + (_w * 0.3f),
									 bot_corner_l.z - (_h * 0.4f), 
									 _w * 0.4f, 
									 _h * 0.4f, 
									 cr_size);
					break;
				case CrowdScene::_3:
					createDenseCrowd(bin, 
									 bot_corner_l.x + (_w * 0.05f),
									 bot_corner_l.z - (_h * 0.2f), 
									 _w * 0.6f, 
									 _h * 0.2f, 
									 cr_size);
					break;
				default:
					break;
			}

			for (size_t i = 0; i < cr_size; i++) {
				glm::vec3 endp = glm::vec3(bin[i].x, bin[i].y, bin[i].z - (_h * 0.78f));
				createNewAgent(bin[i], endp, orange_mat_idx);
			}
			gui::bot_crowd = false;
		}
	}
	else { ImGui::Text(" "); }

	if (gui::top_crowd) {
		ImGui::SameLine();
		if (ImGui::Button("Generate Crowd (Top)")) {
			// get corners of the floor
			BoundingBox bb = ground->BBox.transformCorners(ground->inst_m4x4[0]);
			glm::vec3 bot_corner_l = glm::vec3(bb.min.x, 0.f, bb.max.z);
			glm::vec3 top_corner_r = glm::vec3(bb.max.x, 0.f, bb.min.z);
			float _w = abs(bot_corner_l.x - top_corner_r.x);
			float _h = abs(bot_corner_l.z - top_corner_r.z);

			const size_t cr_size = 30;
			dd_array<glm::vec3> bin = dd_array<glm::vec3>(1);
			switch (scene_selected) {
				case CrowdScene::_1:
					createDenseCrowd(bin, 
									 bot_corner_l.x + (_w * 0.1f), 
									 top_corner_r.z,
									 _w * 0.8f, 
									 _h * 0.2f, 
									 cr_size);
					break;
				case CrowdScene::_2:
					createDenseCrowd(bin, 
									 bot_corner_l.x + (_w * 0.27f), 
									 top_corner_r.z,
									 _w * 0.4f, 
									 _h * 0.4f, 
									 cr_size);
					break;
				case CrowdScene::_3:
					createDenseCrowd(bin, 
									 bot_corner_l.x + (_w * 0.35f), 
									 top_corner_r.z,
									 _w * 0.6f, 
									 _h * 0.2f, 
									 cr_size);
					break;
				default:
					break;
			}

			for (size_t i = 0; i < cr_size; i++) {
				glm::vec3 endp = glm::vec3(bin[i].x, bin[i].y, bin[i].z + (_h * 0.78f));
				createNewAgent(bin[i], endp, purple_mat_idx);
			}
			gui::top_crowd = false;
		}
	}

	/*for (size_t i = 0; i < num_agents; i++) {
		ImGui::Text("");
		std::string n = std::to_string(i + 1) + "::Speed";
		ImGui::DragFloat(n.c_str(), &myAgents[i]->max_speed, 0.25f);

		ImGui::Text("Sensing radius  Time Horizon");
		ImGui::DragFloat2("", myAgents[i]->sense_n_horizon, 0.1f);
	}*/
	ImGui::End();
}


DD_Event CrowdSim::basePost(DD_Event& event) {
	bool line_switch = false;

	if (event.m_type.compare("clear_slate") == 0) {
		deleteCurrentScenario();
		return DD_Event();
	}
	if (event.m_type.compare("run_scene_01") == 0) {
		if (scene_lock) { scenario01(6, 20.f, 20.f + 10.f); }
	}
	if (event.m_type.compare("run_scene_02") == 0) {
		if (scene_lock) { scenario02(20, 20.f, 20.f + 10.f); }
	}
	if (event.m_type.compare("run_scene_03") == 0) {
		if (scene_lock) { scenario03(20.f, 20.f + 10.f); }
	}
	if (event.m_type.compare("post") == 0) {

		if (myControl->clickedLM) {
			// register clicks
			raycastBuff rcb = DD_MathLib::rayCast(myControl->mouseX,
				myControl->mouseY, "Ground");
			if (rcb.hit) {
				/*
				snprintf(char_buff, char_buff_size, "Hit: x: %f, y: %f, z: %f\n",
				rcb.pos.x, rcb.pos.y, rcb.pos.z);
				DD_Terminal::post(char_buff);
				//*/
				if (gui::origin_flag) {
					// set origin position
					gui::new_origin[0] = rcb.pos.x;
					gui::new_origin[1] = rcb.pos.z;
					markerG->UpdatePosition(glm::vec3(gui::new_origin[0], 0.f,
						gui::new_origin[1]));
				}
				else {
					// set goal position
					gui::new_goal[0] = rcb.pos.x;
					gui::new_goal[1] = rcb.pos.z;
					markerY->UpdatePosition(glm::vec3(gui::new_goal[0], 0.f,
						gui::new_goal[1]));
				}
			}
			myControl->clickedLM = false;
		}

		// ground level cam
		if (myControl->pos().y < 10.0f) {
			glm::vec3 _pos = myControl->pos();
			myControl->UpdatePosition(glm::vec3(_pos.x, 10.0f, _pos.z));
		}
		// imgui
		setInterface();

		DD_Event update_avatars;
		update_avatars.m_type = "update_ai";
		update_avatars.m_time = event.m_time;
		update_avatars.m_total_runtime = event.m_total_runtime;
		return update_avatars;
	}
	return DD_Event();
}

void CrowdSim::deleteCurrentScenario()
{
	for (unsigned i = 0; i < num_agents; i++) {
		aica_name.format("aica_%u", i);
		// delete AIControlledAvatar
		ResSpace::deleteAgent(res_ptr, aica_name.str());
		aica_name.format("ai_agent_%u", i);
		
		AI_Agent* ai = ResSpace::findAI_Agent(res_ptr, aica_name.str());
		if (ai) {
			// delete line agent
			ResSpace::removeDD_LineAgent(res_ptr, ai->line_ID.c_str());
		}
		// delete AI_Agent
		ResSpace::removeAI_Agent(res_ptr, aica_name.str());
	}
	// delete obstacle
	ResSpace::deleteAgent(res_ptr, obstacle01_id.str(), true);
	// delete wall object
	ResSpace::removeDD_Model(res_ptr, wall_mdl_id.str());
	// delete node
	ResSpace::removeDD_Agent(res_ptr, "node");
	// delete node_line
	ResSpace::removeDD_LineAgent(res_ptr, "node_lines");
	// delete ai_object
	ResSpace::removeDD_AIObject(res_ptr, main_ai_id.str());

	num_agents = 0;
	scene_lock = true;
}

void createDenseCrowd(dd_array<glm::vec3>& bin, 
					  const float min_x, 
					  const float min_z,
					  const float width, 
					  const float height, 
					  const int size)
{
	bin.resize(size);
	float _y = 25.f;
	for (size_t i = 0; i < size; i++) {
		float _x = (DD_MathLib::getHaltonValue(i, 5) * width) + min_x;
		float _z = (DD_MathLib::getHaltonValue(i, 7) * height) + min_z;
		bin[i] = glm::vec3(_x, _y, _z);
	}
}

void CrowdSim::createNewAgent(const glm::vec3 _A,
							  const glm::vec3 _B,
							  const unsigned mat_idx)
{
	// create new agent
	aica_name.format("aica_%u", (unsigned)num_agents);
	aiag_name.format("ai_agent_%u", (unsigned)num_agents);
	AiControlledAvatar* new_ai = 
		new AiControlledAvatar(aica_name.str(), 
							   aiag_name.str(),
							   res_ptr);
	new_ai->AddModel("cylinder_prim", 0.f, 1000.f);
	new_ai->UpdateScale(glm::vec3(agent_radius));
	if (!ResSpace::AddAgent(res_ptr, new_ai)) {
		DD_Terminal::f_post("[error] CrowdSim::createNewAgent:: Could not "
							"generate new agent <%s>", aica_name.str() );
		return;
	}
	ResSpace::loadAgent_ID(res_ptr, aica_name.str());
	if (gui::bot_crowd) {
		new_ai->SetMaterial(mat_idx);
	}
	else {
		new_ai->SetMaterial(mat_idx);
	}
	
	AI_Agent* new_ag = ResSpace::getNewAI_Agent(res_ptr, aiag_name.str());
	myAgents[num_agents] = new_ag;
	myAgents[num_agents]->init(_A, 
							   _B, 
							   (agent_radius * 100.f) / 2.f, 
							   200.f, 
							   main_ai_id.str(),
							   manhattan_dist);
	myAgents[num_agents]->sense_n_horizon[1] = 8.f;
	myAgents[num_agents]->sense_n_horizon[0] = 500.f;
	myAgents[num_agents]->replan = false;
	num_agents += 1;
}

void CrowdSim::initialize_aiobject(const char * ground_id, 
								   const char * obstacle_id)
{
	DD_Agent* g_agent = ResSpace::findDD_Agent(res_ptr, ground_id);
	DD_Agent* obs_agent = ResSpace::findDD_Agent(res_ptr, obstacle_id);
	// quit because agents cannot be found
	if (!g_agent || !obs_agent) {
		DD_Terminal::f_post("[error] CrowdSim::initialize_aiobject:: Null"
							"ground <%s> or obstacle object <%s>",
							ground_id,
							obstacle_id
		);
		return;
	}
	
	g_agent->cleanInst();
	BoundingBox bbox = g_agent->BBox.transformCorners(
		g_agent->inst_m4x4[0]);

	float _radius = (agent_radius * 100.f) / 2.f;

	glm::vec3 start = bbox.max - glm::vec3(100.f, 0.f, 100.f);
	start.y = 0.f;
	glm::vec3 end = bbox.min + glm::vec3(100.f, 0.f, 100.f);
	end.y = 0.f;

	// create primitive objects for line and node renders (for PRM)
	ResSpace::getNewDD_Agent(res_ptr, "node");
	ResSpace::getNewDD_LineAgent(res_ptr, "node_lines");

	// main ai_object initialization
	DD_AIObject* main_ai = ResSpace::getNewDD_AIObject(res_ptr, main_ai_id.str());
	main_ai->prm_search_radius = 300.f;

	main_ai->obstacles.resize(obs_agent->inst_m4x4.size());
	for (size_t i = 0; i < main_ai->obstacles.size(); i++) {
		main_ai->obstacles[i].init(obs_agent->inst_m4x4[i], obs_agent->BBox);
	}

	main_ai->init(AI_ALGORITHMS::A_STAR, glm::vec3(g_agent->inst_m4x4[0][3]),
				  bbox.max, bbox.min, 250, 5, _radius, "node", "node_lines");
}

void CrowdSim::scenario01(const size_t num_obst, 
						  const float floor_x, 
						  const float floor_z) 
{
	DD_Agent* obst = ResSpace::getNewDD_Agent(res_ptr, obstacle01_id.str());
	cbuff<128> temp;
	temp.format("%s%s", MESH_DIR, "primitives/cylinder_10.ddm");
	wall_mdl_id = "wall_01";
	DD_Model* mdl = ResSpace::loadModel_DDM(
		res_ptr, wall_mdl_id.str(), temp.str());
	obst->AddModel(wall_mdl_id.str(), 0.f, 1000.f);

	dd_array<glm::mat4> _m = dd_array<glm::mat4>(num_obst);
	const float obst_spacing = floor_x * 100.f / (float)num_obst;
	const float _start = -(obst_spacing * num_obst) / 2.f; // start halfway back
	for (size_t i = 0; i < num_obst; i++) {
		const float _x = (_start + (i * obst_spacing) + obst_spacing/2);
		glm::mat4 _mat = glm::translate(glm::mat4(), glm::vec3(_x, 250.f, 0.f));
		_mat = glm::scale(_mat, glm::vec3(1.f, 5.f, 1.f));
		_m[i] = _mat;
	}
	obst->SetInstances(_m);
	ResSpace::loadAgent_ID(res_ptr, obstacle01_id.str());

	initialize_aiobject(ground->m_ID.c_str(), obstacle01_id.str());
	scene_lock = false;
	scene_selected = CrowdScene::_1;

	gui::bot_crowd = true;
	gui::top_crowd = true;
}

void CrowdSim::scenario02(const size_t num_obst,
				const float floor_x, 
				const float floor_z) 
{
	DD_Agent* obst = ResSpace::getNewDD_Agent(res_ptr, obstacle01_id.str());
	cbuff<128> temp;
	temp.format("%s%s", MESH_DIR, "primitives/cylinder_10.ddm");
	wall_mdl_id = "wall_02";
	DD_Model* mdl = ResSpace::loadModel_DDM(
		res_ptr, wall_mdl_id.str(), temp.str());
	obst->AddModel(wall_mdl_id.str(), 0.f, 1000.f);

	dd_array<glm::mat4> _m = dd_array<glm::mat4>(num_obst * 2);
	const float obst_spacing = floor_z * 100.f / (float)num_obst;
	const float z_start = (floor_z * 100.f)/2;
	const float _x = ((floor_x * 100.f) * 0.6f)/2;

	for (size_t i = 0; i < num_obst; i++) {
		const float _z = (z_start + (i * -obst_spacing));
		glm::mat4 _mat = glm::translate(glm::mat4(), glm::vec3(_x, 250.f, _z));
		_mat = glm::scale(_mat, glm::vec3(1.f, 5.f, 1.f));
		_m[i] = _mat;
	}
	for (size_t i = 0; i < num_obst; i++) {
		const float _z = (z_start + (i * -obst_spacing));
		glm::mat4 _mat = glm::translate(glm::mat4(), glm::vec3(-_x, 250.f, _z));
		_mat = glm::scale(_mat, glm::vec3(1.f, 5.f, 1.f));
		_m[i + num_obst] = _mat;
	}
	obst->SetInstances(_m);
	ResSpace::loadAgent_ID(res_ptr, obstacle01_id.str());

	initialize_aiobject(ground->m_ID.c_str(), obstacle01_id.str());
	scene_lock = false;
	scene_selected = CrowdScene::_2;

	gui::bot_crowd = true;
	gui::top_crowd = true;
}

void CrowdSim::scenario03(const float floor_x, const float floor_z)
{
	DD_Agent* obst = ResSpace::getNewDD_Agent(res_ptr, obstacle01_id.str());
	cbuff<128> temp;
	temp.format("%s%s", MESH_DIR, "primitives/cylinder_10.ddm");
	wall_mdl_id = "wall_03";
	DD_Model* mdl = ResSpace::loadModel_DDM(
		res_ptr, wall_mdl_id.str(), temp.str());
	obst->AddModel(wall_mdl_id.str(), 0.f, 1000.f);

	dd_array<glm::mat4> _m = dd_array<glm::mat4>(24);
	float x_start = (floor_x * 100.f) / 2;
	for (size_t i = 0; i < _m.size()/2; i++) {
		const float _x = (-x_start + (i * 50.f));
		glm::mat4 _mat = glm::translate(glm::mat4(), glm::vec3(_x, 250.f, 0.f));
		_mat = glm::scale(_mat, glm::vec3(1.f, 5.f, 1.f));
		_m[i] = _mat;
	}
	for (size_t i = 0; i < _m.size() / 2; i++) {
		const float _x = (x_start - (i * 50.f));
		glm::mat4 _mat = glm::translate(glm::mat4(), glm::vec3(_x, 250.f, 0.f));
		_mat = glm::scale(_mat, glm::vec3(1.f, 5.f, 1.f));
		_m[i + (_m.size() / 2)] = _mat;
	}
	dd_array<glm::mat4> _m2 = dd_array<glm::mat4>(10);
	obst->SetInstances(_m);
	ResSpace::loadAgent_ID(res_ptr, obstacle01_id.str());

	initialize_aiobject(ground->m_ID.c_str(), obstacle01_id.str());
	scene_lock = false;
	scene_selected = CrowdScene::_3;

	gui::bot_crowd = true;
	gui::top_crowd = true;
}
