#include "LightingLvl.h"
#pragma GCC diagnostic ignored "-Wformat-security"

namespace {
	const char* ball_name = "spin_ball";
	LightingLvlNav* myControl;

	DD_Light* light;
	DD_Camera* cam;

	glm::vec3 dir_up = glm::vec3(0.f, 1.f, 0.f);
	glm::vec3 dir_right = glm::vec3(1.f, 0.f, 0.f);
	glm::vec3 dir_forward = glm::vec3(0.f, 0.f, -1.f);
}

//typedef float m_params[3];

namespace gui {}

LightingLvl::~LightingLvl() {}

void DeleteAgent_LB(DD_Resources* res, const size_t ID);

void LightingLvl::Init() {
	// add mouse controller to level
	myControl = new LightingLvlNav("avatar");
	AddAgent(myControl);
	myControl->UpdatePosition(glm::vec3(0.0f, 0.0f, 0.0f));
	myControl->initRot(glm::radians(-30.f), glm::radians(45.f));

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "main_light");
	light->_position = glm::vec3(1000.0f, 1000.f, 2000.0f);
	light->m_color = glm::vec3(0.1f);
	light->m_flagShadow = true;

	// add generic camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.0f;
	cam->far_plane = 10000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	//  callbacks and posts
	EventHandler a = std::bind(&LightingLvl::basePost, this, std::placeholders::_1);
	AddCallback("post", a);

	// create some materials
	DD_Material* mat_r = ResSpace::getNewDD_Material(res_ptr, "red");
	mat_r->m_base_color = glm::vec4(0.8f, 0.1f, 0.1f, 1.f);
	DD_Material* mat_g = ResSpace::getNewDD_Material(res_ptr, "green");
	mat_g->m_base_color = glm::vec4(0.1f, 0.8f, 0.1f, 1.f);

	// create base agent
	cbuff<256> mdl_path;
	// ball
	mdl_path.format("%sLightingTests/%s", PROJECT_DIR, "sphere_main.ddm");
	DD_Model* mdl = ResSpace::loadModel_DDM(res_ptr, "spin_ball", mdl_path.str());
	if (mdl) {
		DD_Agent* ball = ResSpace::getNewDD_Agent(res_ptr, ball_name);
		ball->AddModel("spin_ball", 0.f, 1000.f);
		ball->UpdateScale(glm::vec3(100.f));
		ball->SetMaterial(ResSpace::getDD_Material_idx(res_ptr, "red"));
	}
	// base
	mdl_path.format("%sLightingTests/%s", PROJECT_DIR, "base.ddm");
	mdl = ResSpace::loadModel_DDM(res_ptr, "ball_base", mdl_path.str());
	if (mdl) {
		DD_Agent* ball_base = ResSpace::getNewDD_Agent(res_ptr, "ball_base");
		ball_base->AddModel("ball_base", 0.f, 1000.f);
		ball_base->UpdateScale(glm::vec3(100.f));
		ball_base->SetMaterial(ResSpace::getDD_Material_idx(res_ptr, "green"));
	}
	// floor
	DD_Agent* floor = ResSpace::getNewDD_Agent(res_ptr, "floor");
	floor->AddModel("plane_prim", 0.f, 1000.f);
	floor->UpdateScale(glm::vec3(100.f));
}

// Setup imgui interface
void LightingLvl::setInterface(const float dt) {
	//const float debugY = m_screenH - 50.f;
	//const float debugW = m_screenW - 10.f;
	//ImGui::SetNextWindowPos(ImVec2(0.f, debugY));
	//ImGui::SetNextWindowSize(ImVec2(debugW, 20.f));

	//ImGuiWindowFlags toolbar_flags = 0;
	//toolbar_flags |= ImGuiWindowFlags_NoTitleBar;
	//toolbar_flags |= ImGuiWindowFlags_NoResize;
	//toolbar_flags |= ImGuiWindowFlags_NoMove;
}

DD_Event LightingLvl::basePost(DD_Event& event) {
	if (event.m_type.compare("post") == 0) {

		setInterface(event.m_time);

		// rotate sphere
		DD_Agent* ball = ResSpace::findDD_Agent(res_ptr, ball_name);
		if (ball) {
			glm::quat rot = glm::rotate(
				glm::quat(), event.m_total_runtime, glm::vec3(0.f, 1.f, 0.f));
			ball->UpdateRotation(rot);
		}

		return DD_Event();
	}
	return DD_Event();
}
