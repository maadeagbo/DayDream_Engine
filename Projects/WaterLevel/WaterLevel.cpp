#include "WaterLevel.h"
#include <experimental/filesystem>

namespace {
	const size_t char_buff_size = 512, max_frames = 300;
	char char_buff[char_buff_size];
	ControllerW* myControl;
	DD_Light* light;
	DD_Camera* cam;
	DD_Agent *test_obj, *ball_obj, *tree1;

	DD_Water* my_water;
	bool create_water = true;
	const glm::vec3 ball_pos = glm::vec3(1800.f, 150.f, 1800.f);
}

WaterLevel::~WaterLevel() {}

void WaterLevel::Init() {
	// register skybox map
	m_cubeMapID = "sky";
	// turn on dynamic cube map generation
	m_flagDynamicCubeMap = true;
	// add mouse controller to level
	myControl = new ControllerW("avatar");
	AddAgent(myControl);
	myControl->clickedSpace = false;
	myControl->UpdatePosition(glm::vec3(0.0f, 1000.0f, 3000.0f));
	myControl->initRot(glm::radians(0.f), glm::radians(0.f));

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "shadow");
	light->_position = glm::vec3(1000.0f, 1000.f, 2000.0f);
	light->m_color = glm::vec3(0.1f);
	light->m_flagShadow = false;

	// add point light

	// add generic camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.0f;
	cam->far_plane = 15000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	//  callbacks and posts
	EventHandler a = std::bind(&WaterLevel::basePost, this,
		std::placeholders::_1);
	AddCallback("post", a);

	// add agent model
	///*
	test_obj = ResSpace::getNewDD_Agent(res_ptr, "test_obj");
	test_obj->UpdatePosition(glm::vec3(0.f, 500.f, 0.f));
	test_obj->UpdateScale(glm::vec3(10.f));
	test_obj->AddModel("Dillo", 0.f, 15000.f);
	//*/

	ball_obj = ResSpace::getNewDD_Agent(res_ptr, "sphere");
	ball_obj->AddModel("sphere_prim", 0.f, 10000.f);
	ball_obj->UpdatePosition(ball_pos);
	ball_obj->UpdateScale(glm::vec3(3.f));

	/*tree1 = GetNewObject<DD_Agent*>("tree_1");
	tree1->AddModel("tree01", 0.f, 10000.f);
	tree1->UpdatePosition(glm::vec3(500.f, 100.f, 0.f));*/

	// change ball material
	DD_Material* mat = ResSpace::getNewDD_Material(res_ptr, "my_mat_01");
	mat->m_base_color = glm::vec4(1.f, 0.1f, 0.1f, 1.f);
	int m_idx = ResSpace::getDD_Material_idx(res_ptr, "my_mat_01");
	m_idx = (m_idx == -1) ? 0 : m_idx;
	ball_obj->SetMaterial(m_idx);
}

// Setup imgui interface
void WaterLevel::setInterface(const float dt) {
	//ImGuiWindowFlags window_flags = 0;
	//ImGui::Begin("Water Controls", nullptr, window_flags);
	//ImGui::End();
}

DD_Event WaterLevel::basePost(DD_Event& event) {
	if (event.m_type.compare("post") == 0) {

		// restrict movement
		glm::vec3 _p = myControl->pos();
		if (_p.y < 50.f) {
			myControl->UpdatePosition(glm::vec3(_p.x, 50.f, _p.z));
		}

		// rotate ball
		float x_p = glm::sin(glm::radians(event.m_total_runtime * 100.f));
		float z_p = glm::cos(glm::radians(event.m_total_runtime * 100.f));
		_p = glm::vec3(ball_pos.x * x_p, ball_pos.y, ball_pos.z * z_p);
		ball_obj->UpdatePosition(_p);

		if (create_water && event.m_total_runtime > 10.f) {
			// create water 10 seconds after program start
			create_water = false;
			my_water = ResSpace::getNewDD_Water(res_ptr, "Test_water");

			my_water->initialize(10, 10, 1100.f, 1100.f, glm::vec3(-5000.f,
				300.f, -5000.f), 1);
			//my_water->m_skyb01 = "sky"; 
			my_water->m_skyb01 = "Dynamic"; // use if dynamic
			my_water->m_normTex = "simple_water_n";
			//my_water->pause = true;
		}
		if (!create_water && my_water) {
			if (myControl->clickedLM) {
				myControl->clickedLM = false;

				/*my_water->addDrop(glm::vec2(myControl->mouseX, myControl->mouseY),
					50.f, 50.f);*/
			}
		}
		return DD_Event();
	}
	return DD_Event();
}
