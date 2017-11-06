#include "LightingLvl.h"
#pragma GCC diagnostic ignored "-Wformat-security"

namespace 
{
	const char* ball_name = "ball_agent";
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

void LightingLvl::Init() 
{
	edit_window = false;

	// add mouse controller to level
	myControl = new LightingLvlNav("avatar");
	AddAgent(myControl);
	myControl->UpdatePosition(glm::vec3(0.0f, 0.0f, 0.0f));
	myControl->initRot(glm::radians(-30.f), 0.f);

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "main_light");
	light->_position = glm::vec3(10.0f, 200.f, 50.0f);
	light->m_color = glm::vec3(1.f);
	light->m_flagShadow = true;

	// add generic camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.f;
	cam->far_plane = 2000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	//  callbacks and posts
	EventHandler a = std::bind(&LightingLvl::basePost, this, std::placeholders::_1);
	AddCallback("post", a);
	AddCallback("edit_cam", a);

	// create some materials
	DD_Material* mat_r = ResSpace::getNewDD_Material(res_ptr, "red");
	mat_r->m_base_color = glm::vec4(0.8f, 0.1f, 0.1f, 1.f);
	DD_Material* mat_g = ResSpace::getNewDD_Material(res_ptr, "green");
	mat_g->m_base_color = glm::vec4(0.1f, 0.8f, 0.1f, 1.f);

	// create base agent
	cbuff<256> mdl_path;
	// balls
	mdl_path.format("%sLightingTests/%s", PROJECT_DIR, "sphere_main.ddm");
	DD_Model* mdl = ResSpace::loadModel_DDM(res_ptr, "spin_ball", mdl_path.str());
	if (mdl) {
		cbuff<16> ball_id;
		DD_Agent* ball = nullptr;
		for (unsigned i = 0; i < 5; i++) {
			ball_id.format("%s_%u", ball_name, i);
			ball = ResSpace::getNewDD_Agent(res_ptr, ball_id.str());
			if (ball) {
				ball->AddModel("spin_ball", 0.f, 1000.f);
				ball->UpdatePosition(glm::vec3(0.f, 0.f, -150.f * i));
				ball->UpdateScale(glm::vec3(100.f));
				ball->SetMaterial(ResSpace::getDD_Material_idx(res_ptr, "red"));
			}
		}
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
void LightingLvl::setInterface(const float dt) 
{
	// get io for mouse & keyboard management
	ImGuiIO& imgui_io = ImGui::GetIO();
	myControl->ignore_controls = imgui_io.WantCaptureMouse;

	// Manipulate camera and light
	if (edit_window) {
		cam = ResSpace::findDD_Camera(res_ptr, "myCam");
		light = ResSpace::findDD_Light(res_ptr, "main_light");
		if (!cam || !light) { 
			DD_Terminal::post("[error] LightingLvl::Camera or Light nullptr");
			return; 
		}

		ImGui::Begin("Lighting Control", &edit_window, 0);

		// Camera controls
		ImGui::Text("Camera");
		ImGui::DragFloat("Near plane", &cam->near_plane, 0.1f, 1.f);
		ImGui::DragFloat("Far plane", &cam->far_plane, 10.f, 1.f);
		
		// Light controls
		ImGui::Separator();

		// set light type
		ImGui::Text("Light");
		int light_type = (int)light->m_type;
		ImGui::RadioButton("Directional", &light_type, LightType::DIRECTION_L);
		ImGui::SameLine();
		ImGui::RadioButton("Point", &light_type, LightType::POINT_L);
		light->m_type = (LightType)light_type;
		// fall off
		ImGui::Text("Fall-off");
		ImGui::DragFloat("Linear", 
						 &light->m_linear, 
						 0.00000001, 
						 0.1,
						 0.f,
						 "%.9f");
		ImGui::DragFloat("Quadratic", 
						 &light->m_quadratic, 
						 0.00000001, 
						 0.1,
						 0.f,
						 "%.9f");

		ImGui::End();
	}
}

DD_Event LightingLvl::basePost(DD_Event& event) 
{
	if (event.m_type.compare("post") == 0) {
		setInterface(event.m_time);

		// rotate spheres
		cbuff<16> ball_id;
		DD_Agent* ball = nullptr;
		for (unsigned i = 0; i < 5; i++) {
			ball_id.format("%s_%u", ball_name, i);
			ball = ResSpace::findDD_Agent(res_ptr, ball_id.str());
			if (ball) {
				glm::quat rot = glm::rotate(glm::quat(), 
											event.m_total_runtime * (1.f + i), 
											glm::vec3(0.f, 1.f, 0.f));
				ball->UpdateRotation(rot);
			}
		}

		return DD_Event();
	}
	if (event.m_type.compare("edit_cam") == 0) {
		edit_window ^= 1;
	}
	return DD_Event();
}
