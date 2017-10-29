#include "LevelBuilder.h"
#pragma GCC diagnostic ignored "-Wformat-security"

namespace {
	const size_t char_buff_size = 512, max_frames = 300;
	char char_buff[char_buff_size];
	Navigation* myControl;

	DD_Light* light;
	DD_Camera* cam;
	DD_LineAgent *grid_h, *grid_v;

	glm::vec3 dir_up = glm::vec3(0.f, 1.f, 0.f);
	glm::vec3 dir_right = glm::vec3(1.f, 0.f, 0.f);
	glm::vec3 dir_forward = glm::vec3(0.f, 0.f, -1.f);
}

//typedef float m_params[3];

namespace gui {
	dd_array<std::string> mesh_IDs;
	dd_array<glm::vec3> inst_params;
	glm::vec3 selected_rot;
	const size_t buff_size = 64;
	size_t total_agents = 0;
	float f_2D[2] = { 0.f, 10000.f }, f_3D[3], lod[5][2];
	int mod_index[5], i_1Da = 1, i_1Db = 0;
	bool new_agent = false, selected_agent = false;
	char buff01[buff_size], buff02[buff_size];
	std::string agentIDs[500];

	DD_Agent* curr_agent;
}

LevelBuilder::~LevelBuilder() {}

void DeleteAgent_LB(DD_Resources* res, const size_t ID);

void LevelBuilder::Init() {
	// add mouse controller to level
	myControl = new Navigation("avatar");
	AddAgent(myControl);
	myControl->UpdatePosition(glm::vec3(0.0f, 0.0f, 0.0f));
	myControl->initRot(glm::radians(-30.f), glm::radians(45.f));

	// unitialize some parameters for gui
	for (size_t i = 0; i < 5; i++) {
		gui::mod_index[i] = -1;
	}
	gui::lod[0][1] = 1000.0f;

	// center ball
	/*
	DD_Agent* ball = GetNewObject<DD_Agent*>("center");
	ball->AddModel("sphere_prim", 0.f, 1000.f);
	ball->UpdateScale(glm::vec3(0.4f, 0.1f, 0.4f));
	//*/
	// gather meshes and add to select menu
	gui::mesh_IDs.resize(res_ptr->mdl_counter);
	for (size_t i = 0; i < res_ptr->mdl_counter; i++) {
		gui::mesh_IDs[i] = res_ptr->models[i].m_ID;
	}

	// Grid
	const float extent = 900.f;
	dd_array<LinePoint> h_lines = dd_array<LinePoint>(19);
	dd_array<LinePoint> v_lines = dd_array<LinePoint>(19);
	for (size_t i = 0; i < 10; i++) {
		h_lines[i] = {  glm::vec4(extent, 0.f, 100.f * i, 1.f),
							glm::vec4(-extent, 0.f, 100.f * i, 1.f) };
	}
	for (size_t i = 9; i > 0; i--) {
		h_lines[19 - i] = { glm::vec4(extent, 0.f, -100.f * i, 1.f),
								glm::vec4(-extent, 0.f, -100.f * i, 1.f) };
	}
	grid_h = ResSpace::getNewDD_LineAgent(res_ptr, "grid_h");
	grid_h->color = glm::vec4(1.f);
	grid_h->lines = std::move(h_lines);
	for (size_t i = 0; i < 10; i++) {
		v_lines[i] = {  glm::vec4(100.f * i, 0.f, extent, 1.f),
						glm::vec4(100.f * i, 0.f, -extent, 1.f) };
	}
	for (size_t i = 9; i > 0; i--) {
		v_lines[19 - i] = { glm::vec4(-100.f * i, 0.f, extent, 1.f),
							glm::vec4(-100.f * i, 0.f, -extent, 1.f) };
	}
	grid_v = ResSpace::getNewDD_LineAgent(res_ptr, "grid_v");
	grid_v->color = glm::vec4(1.f);
	grid_v->lines = std::move(v_lines);

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "main_light");
	light->_position = glm::vec3(1000.0f, 1000.f, 2000.0f);
	light->m_color = glm::vec3(0.1f);
	//light->m_flagShadow = true;

	// add generic camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.0f;
	cam->far_plane = 10000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	//  callbacks and posts
	EventHandler a = std::bind(&LevelBuilder::basePost, this, std::placeholders::_1);
	AddCallback("post", a);
}

// Setup imgui interface
void LevelBuilder::setInterface(const float dt) {
	const float debugY = m_screenH - 50.f;
	const float debugW = m_screenW - 10.f;
	ImGui::SetNextWindowPos(ImVec2(0.f, debugY));
	ImGui::SetNextWindowSize(ImVec2(debugW, 20.f));

	ImGuiWindowFlags toolbar_flags = 0;
	toolbar_flags |= ImGuiWindowFlags_NoTitleBar;
	toolbar_flags |= ImGuiWindowFlags_NoResize;
	toolbar_flags |= ImGuiWindowFlags_NoMove;
	ImGui::Begin("Level Buider", nullptr, toolbar_flags);

	ImGui::Text("Level Builder 1.0\t");
	ImGui::SameLine();

	// center camera
	if (ImGui::Button("Center Camera")) {
		myControl->UpdatePosition(glm::vec3(0.0f, 0.0f, 0.0f));
	}

	// agent creation button
	ImGui::SameLine();
	if (ImGui::Button("Create Agent")) { gui::new_agent ^= 1; }

	// agent deletion Pop-up
	ImGui::SameLine();
	if (ImGui::Button("Delete Agent")) { ImGui::OpenPopup("Agent IDs"); }
	int d_index = -1;
	if (ImGui::BeginPopup("Agent IDs")) {
		ImGui::Text("Agents");
		ImGui::Separator();
		for (size_t i = 0; i < gui::total_agents; i++) {
			if (ImGui::Selectable(gui::agentIDs[i].c_str())) { d_index = i; }
		}
		ImGui::EndPopup();
	}
	if (d_index >= 0 && !gui::selected_agent) { DeleteAgent_LB(res_ptr, d_index); }

	// agent selection Pop-up
	ImGui::SameLine();
	if (ImGui::Button("Select Agent")) { ImGui::OpenPopup("Select IDs"); }
	if (ImGui::BeginPopup("Select IDs")) {
		ImGui::Text("Agents");
		ImGui::Separator();
		for (size_t i = 0; i < gui::total_agents; i++) {
			if (ImGui::Selectable(gui::agentIDs[i].c_str())) {
				gui::selected_agent = true;
				std::string id = gui::agentIDs[i];
				gui::curr_agent = ResSpace::findDD_Agent(res_ptr, id.c_str());
				gui::inst_params.resize(gui::curr_agent->inst_m4x4.size() * 3);
			}
		}
		ImGui::EndPopup();
	}

	// New agent window
	if (gui::new_agent) {
		ImGui::Begin("Agent Init", &gui::new_agent, 0);
		ImGui::InputText("ID", gui::buff01, gui::buff_size);
		ImGui::SliderInt("LOD index", &gui::i_1Db, 0, 4);

		// pop-up menu
		if (ImGui::Button("Mesh-->")) { ImGui::OpenPopup("Mesh IDs"); }
		ImGui::SameLine();
		snprintf(
			char_buff, char_buff_size, "%s",
			gui::mod_index[gui::i_1Db] == -1 ?
				"<None>" : gui::mesh_IDs[gui::mod_index[gui::i_1Db]].c_str()
		);
		ImGui::Text(char_buff);
		if (ImGui::BeginPopup("Mesh IDs")) {
			ImGui::Text("Mesh IDs");
			ImGui::Separator();
			for (size_t i = 0; i < gui::mesh_IDs.size(); i++) {
				if (ImGui::Selectable(gui::mesh_IDs[i].c_str())) {
					gui::mod_index[gui::i_1Db] = i;
				}
			}
			ImGui::EndPopup();
		}

		ImGui::DragFloat2("LOD", gui::lod[gui::i_1Db]);
		ImGui::DragInt("Copies", &gui::i_1Da, 0.3f, 1);
		if (ImGui::Button("Instantiate")) {
			std::string id_check = gui::buff01;
			bool duplicate = false;
			for (size_t i = 0; i < gui::total_agents; i++) {
				if (gui::agentIDs[i] == id_check) { duplicate = true; }
			}
			if (id_check != "" && !duplicate && gui::mod_index[0] >= 0) {
				gui::new_agent ^= 1;
				DD_Agent* n_agent = ResSpace::getNewDD_Agent(res_ptr, gui::buff01);
				size_t m_index = 0;
				for (size_t i = 0; i < 5; i++) {
					if (gui::lod[i][0] >= 0.f && gui::lod[i][1] > 0.f) {
						n_agent->AddModel(gui::mesh_IDs[gui::mod_index[m_index]].c_str(),
							gui::lod[m_index][0], gui::lod[m_index][1]);
						m_index += 1;
					}
				}
				if (gui::i_1Da > 1) {
					dd_array<glm::mat4> m = dd_array<glm::mat4>(gui::i_1Da);
					n_agent->SetInstances(m);
				}
				gui::agentIDs[gui::total_agents] = gui::buff01;
				gui::total_agents += 1;
				ResSpace::loadAgentToGPU_M(res_ptr, gui::buff01);

				// reset
				snprintf(gui::buff01, gui::buff_size, " ");
				for (size_t i = 0; i < 5; i++) {
					gui::mod_index[i] = -1;
					gui::lod[i][0] = -1.f;
					gui::lod[i][1] = -1.f;
				}
				gui::i_1Da = 1;
				gui::i_1Db = 0;
				gui::lod[0][0] = 0.f;
				gui::lod[0][1] = 10000.f;
			}
		}
		ImGui::End();
	}

	// Modify attributes window
	if (gui::selected_agent) {
		ImGui::Begin("Edit", &gui::selected_agent, 0);
		snprintf(char_buff, char_buff_size, "%s", gui::curr_agent->m_ID.c_str());
		ImGui::Text(char_buff);
		if (gui::curr_agent->inst_m4x4.size() == 1) {
			ImGui::SameLine(0.f, 5.f);
			if (ImGui::Button("Focus Camera")) {
				glm::vec3 p = glm::vec3(gui::curr_agent->inst_m4x4[0][3]);
				myControl->UpdatePosition(p);
			}
			glm::vec3 temp = gui::curr_agent->pos();
			ImGui::DragFloat3("Position", &temp[0], 0.1f);
			gui::curr_agent->UpdatePosition(temp);

			ImGui::DragFloat3("Rotation", &gui::selected_rot[0], 0.1f);
			temp.x = glm::radians(gui::selected_rot.x);
			temp.y = glm::radians(gui::selected_rot.y);
			temp.z = glm::radians(gui::selected_rot.z);
			gui::curr_agent->UpdateRotation(glm::quat(temp));

			temp = gui::curr_agent->size();
			ImGui::DragFloat3("Scale", &temp[0], 0.1f);
			gui::curr_agent->UpdateScale(temp);
		}
		else {
			for (size_t i = 0; i < gui::inst_params.size() / 3; i++) {
				ImGui::Text(" ");
				glm::mat4 this_mat = gui::curr_agent->inst_m4x4[i];
				glm::quat curr_rot = glm::quat_cast(glm::mat3(this_mat));

				gui::inst_params[i * 3] = glm::vec3(this_mat[3]);
				gui::inst_params[i * 3 + 1] = glm::eulerAngles(curr_rot);
				gui::inst_params[i * 3 + 2] = glm::vec3(1.f);

				std::string _p = "Pos   (" + std::to_string(i + 1) + ")";
				std::string _r = "Rot   (" + std::to_string(i + 1) + ")";
				std::string _s = "Scale (" + std::to_string(i + 1) + ")";
				ImGui::DragFloat3(_p.c_str(), &gui::inst_params[i * 3][0], 0.1f);
				ImGui::DragFloat3(_r.c_str(), &gui::inst_params[i * 3 + 1][0], 0.05f);
				ImGui::DragFloat3(_s.c_str(), &gui::inst_params[i * 3 + 2][0], 0.1f, 1.f);

				glm::mat4 temp = glm::translate(glm::mat4(), gui::inst_params[i * 3]);
				temp = temp * glm::mat4_cast(glm::quat(gui::inst_params[i * 3 + 1]));
				temp = glm::scale(temp, gui::inst_params[i * 3 + 2]);

				gui::curr_agent->inst_m4x4[i] = std::move(temp);
			}
		}
		ImGui::End();
	}
	ImGui::End();
}

DD_Event LevelBuilder::basePost(DD_Event& event) {
	if (event.m_type.compare("post") == 0) {

		setInterface(event.m_time);

		// check if click intersects agent
		if (myControl->pressed[NavUI::LMOUSE] && myControl->pressed[NavUI::ALT_L]) {
			myControl->pressed[NavUI::LMOUSE] ^= 1;
			myControl->pressed[NavUI::ALT_L] ^= 1;

			raycastBuff rcb = DD_MathLib::rayCast(myControl->mouseX, myControl->mouseY);
			if (rcb.hit) {
				gui::curr_agent = ResSpace::findDD_Agent(res_ptr, rcb.index);
				gui::inst_params.resize(gui::curr_agent->inst_m4x4.size() * 3);
				gui::selected_agent = true;

				gui::selected_rot = glm::eulerAngles(gui::curr_agent->rot());
				float convert_deg = 180.f/3.14159f;
				gui::selected_rot.x = gui::selected_rot.x * convert_deg;
				gui::selected_rot.y = gui::selected_rot.y * convert_deg;
				gui::selected_rot.z = gui::selected_rot.z * convert_deg;
			}
		}
		if (myControl->pressed[NavUI::LMOUSE]) { myControl->pressed[NavUI::LMOUSE] ^= 1; }
		if (myControl->pressed[NavUI::ALT_L]) { myControl->pressed[NavUI::ALT_L] ^= 1; }

		return DD_Event();
	}
	return DD_Event();
}

void DeleteAgent_LB(DD_Resources * res, const size_t ID) {
	std::string _ID = gui::agentIDs[ID];
	const size_t range = (gui::total_agents - ID) - 1;
	for (size_t i = ID; i < (ID + range); i++) {
		gui::agentIDs[i] = gui::agentIDs[i + 1];
	}
	ResSpace::removeDD_Agent(res, _ID.c_str());
	gui::total_agents -= 1;
}
