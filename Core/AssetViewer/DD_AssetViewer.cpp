#include "DD_AssetViewer.h"
#include "DD_Terminal.h"
#include <fstream>

bool checkFile(const char* name);

enum class ImportType : unsigned
{
	FBX,
	DDM,
	DDM_S,
	DDM_S_A,
	NONE
};

namespace {
	DD_Agent* temp_agent;
	AssetNav* myControl;
	DD_Camera* cam;
	DD_Light* light;
	DD_LineAgent* grid_h;
	DD_LineAgent* grid_v;
	DD_Skeleton* temp_skeleton;
	DD_ModelSK* temp_modelsk;
	DD_AnimClip* temp_anim;

	std::future<dd_array<MeshData>> load_func;
	std::chrono::milliseconds ms_10(10);
	typedef std::future_status _status;
}

namespace AVGui
{
	const size_t buff_size = 512;
	float bkgr_col[4] = { 0.1f, 0.1f, 0.1f, 0.98f };
	int import = 1;
	unsigned imp_flag = (unsigned)ImportType::NONE;
	bool import_extra[2] = { false, false };
	bool import_success[4] = { false, false, false, false };
	bool load_win = false;
	bool thread_launch = false;
	char buff01[buff_size];
	char buff02[buff_size] = "\"ddg, .ddm, .ddb, or .dda\"";
	char buff03[buff_size];
}

/// \brief Initialze controllers, camera, ui, etc... for AssetViewer
void DD_AssetViewer::Load()
{
	// asset file
	m_assetfile = std::string(ROOT_DIR) + "Core/AssetViewer/assets";
	// Grid
	const float length = 90.f;
	const float _w = 10.f;
	dd_array<LinePoint> h_lines = dd_array<LinePoint>(19);
	dd_array<LinePoint> v_lines = dd_array<LinePoint>(19);
	for (size_t i = 0; i < 10; i++) {
		h_lines[i] = { glm::vec4(length, 0.f, _w * i, 1.f),
			glm::vec4(-length, 0.f, _w * i, 1.f) };
	}
	for (size_t i = 9; i > 0; i--) {
		h_lines[h_lines.size() - i] = { glm::vec4(length, 0.f, -_w * i, 1.f),
			glm::vec4(-length, 0.f, -_w * i, 1.f) };
	}
	grid_h = ResSpace::getNewDD_LineAgent(res_ptr, "grid_h");
	grid_h->color = glm::vec4(1.f);
	grid_h->lines = std::move(h_lines);
	for (size_t i = 0; i < 10; i++) {
		v_lines[i] = { glm::vec4(_w * i, 0.f, length, 1.f),
			glm::vec4(_w * i, 0.f, -length, 1.f) };
	}
	for (size_t i = 9; i > 0; i--) {
		v_lines[v_lines.size() - i] = { glm::vec4(-_w * i, 0.f, length, 1.f),
			glm::vec4(-_w * i, 0.f, -length, 1.f) };
	}
	grid_v = ResSpace::getNewDD_LineAgent(res_ptr, "grid_v");
	grid_v->color = glm::vec4(1.f);
	grid_v->lines = std::move(v_lines);

	// controller
	myControl = new AssetNav("avatar");
	res_ptr->agents[res_ptr->m_num_agents] = myControl;
	res_ptr->m_num_agents += 1;
	myControl->UpdatePosition(glm::vec3(110.0f, 110.0f, 110.0f));
	myControl->initRot(glm::radians(-35.f), glm::radians(45.f));

	// camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 1.0f;
	cam->far_plane = 10000.0f;
	cam->SetParent(myControl->m_ID.c_str());

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "main_light");
	light->_position = glm::vec3(1000.0f, 1000.f, 2000.0f);
	light->m_color = glm::vec3(0.1f);
	//light->m_flagShadow = true;
}


/// \brief Manages ImGui Interface
void DD_AssetViewer::setInterface(const float dt)
{
	const float debugY = m_screenH - 60.f;
	const float debugW = m_screenW / 5.f;

	// get io for mouse & keyboard management
	ImGuiIO& imgui_io = ImGui::GetIO();
	myControl->ignore_controls = imgui_io.WantCaptureMouse;
}

/// \brief Event handler for queue
DD_Event DD_AssetViewer::Update(DD_Event & event)
{
	if (event.m_type == "post") {
		setInterface(event.m_time);
	}
	return DD_Event();
}

/// \brief Checks if file exists
bool checkFile(const char *name)
{
	std::ifstream file(name);
	return file.is_open();
}
