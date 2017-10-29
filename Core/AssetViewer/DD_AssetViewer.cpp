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

struct SkeletonDebug
{
	float boxSize = 1.f;
	glm::vec4 lineColor = glm::vec4(1.f);
	DD_Agent* boxRender;
	DD_LineAgent* lineRender;
	dd_array<glm::uvec2> lineIdxs;
	void setup(DD_Resources* res,
			   const char* mdl_id,
			   const char* line_id,
			   const DD_ModelSK* mdlsk,
			   const DD_Skeleton* sk);
	void render(const dd_array<glm::mat4> mats);
private:
	bool flagLoaded = false;
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
	SkeletonDebug dbSk;

	std::future<dd_array<MeshData>> load_func;
	std::chrono::milliseconds ms_10(10);
	typedef std::future_status _status;

	dd_array<MeshData> temp_mesh;
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

	preLoad(); // for testing
}


/// \brief Manages ImGui Interface
void DD_AssetViewer::setInterface(const float dt)
{
	const float debugY = m_screenH - 60.f;
	const float debugW = m_screenW / 5.f;

	// get io for mouse & keyboard management
	ImGuiIO& imgui_io = ImGui::GetIO();
	myControl->ignore_controls = imgui_io.WantCaptureMouse;

	// main menu
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Panels")) {
			if (ImGui::MenuItem("Asset Importer")) {}
			if (ImGui::MenuItem("Export Agent")) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::SetNextWindowPos(ImVec2(0.f, 20.f));
	ImGui::SetNextWindowSize(ImVec2(debugW, debugY));
	ImGuiWindowFlags toolbar_flags = 0;
	toolbar_flags |= ImGuiWindowFlags_NoTitleBar;
	toolbar_flags |= ImGuiWindowFlags_NoResize;
	toolbar_flags |= ImGuiWindowFlags_NoMove;
	toolbar_flags |= ImGuiWindowFlags_NoCollapse;

	// set background window color
	ImColor col = ImColor(AVGui::bkgr_col[0],
						  AVGui::bkgr_col[1],
						  AVGui::bkgr_col[2],
						  AVGui::bkgr_col[3]);
	//ImGui::PushStyleColor(ImGuiCol_WindowBg, col);
	//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1.f);
	//ImGui::PopStyleVar();
	ImGui::Begin("Asset Viewer", nullptr, toolbar_flags);

	//ImGui::PopStyleColor(); // background window color

	// Import panel
	ImGui::Text("Import Assets");
	ImGui::Separator();
	ImGui::InputText("Asset ID", AVGui::buff01, AVGui::buff_size);
	if (ImGui::InputText("file",
						 AVGui::buff02,
						 AVGui::buff_size,
						 ImGuiInputTextFlags_EnterReturnsTrue)) {
		if (*AVGui::buff02 && *AVGui::buff01 && *AVGui::buff02 != ' ') {
			// check extension
			if (strstr(AVGui::buff02, ".ddg")) {
				DD_Terminal::f_post("Importing group mesh: %s", AVGui::buff01);
			}
			else if (strstr(AVGui::buff02, ".ddm")) {
				DD_Terminal::f_post("Importing mesh: %s", AVGui::buff01);
			}
			else if (strstr(AVGui::buff02, ".ddb")) {
				DD_Terminal::f_post("Importing skeleton: %s", AVGui::buff01);
			}
			else if (strstr(AVGui::buff02, ".dda")) {
				DD_Terminal::f_post("Importing animation: %s", AVGui::buff01);
			}
		}
	}
	//ImGui::RadioButton("FBX", &AVGui::import, 0); ImGui::SameLine();
	//ImGui::RadioButton("DDM", &AVGui::import, 1); ImGui::SameLine();
	//ImGui::RadioButton("NONE", &AVGui::import, 2);
	//const char* ext = (AVGui::import == 0) ?
	//	" .fbx file" : (AVGui::import == 1) ? " .ddm file" : " ";
	//ImGui::InputText(ext, AVGui::buff01, AVGui::buff_size);
	//if (AVGui::import == 1) {
	//	ImGui::Checkbox("Skeleton", &AVGui::import_extra[0]);
	//}
	//if (AVGui::import_extra[0] && AVGui::import == 1) {
	//	ImGui::InputText(" .dds file", AVGui::buff02, AVGui::buff_size);
	//	// can only import animation if there's a skeleton
	//	ImGui::Checkbox("Animation", &AVGui::import_extra[1]);
	//}
	//if (AVGui::import_extra[1] && AVGui::import == 1) {
	//	ImGui::InputText(" .dda file", AVGui::buff03, AVGui::buff_size);
	//}
	//// Import button
	//if (AVGui::import == 0) { // fbx only
	//	if (ImGui::Button("Convert FBX")) {
	//		AVGui::import_success[3] = checkFile(AVGui::buff01);
	//		AVGui::load_win = true;
	//		AVGui::imp_flag = 0;
	//	}
	//}
	//else if (AVGui::import == 1) {
	//	if (AVGui::import_extra[0] && AVGui::import_extra[1]) { // ddm, dda, dds
	//		if (ImGui::Button("Import All")) {
	//			AVGui::import_success[0] = checkFile(AVGui::buff01);
	//			AVGui::import_success[1] = checkFile(AVGui::buff02);
	//			AVGui::import_success[2] = checkFile(AVGui::buff03);
	//			AVGui::load_win = true;
	//			AVGui::imp_flag = 3;
	//		}
	//	}
	//	else if (AVGui::import_extra[0]) { // ddm, dds
	//		if (ImGui::Button("Import mesh and skeleton")) {
	//			AVGui::import_success[0] = checkFile(AVGui::buff01);
	//			AVGui::import_success[1] = checkFile(AVGui::buff02);
	//			AVGui::load_win = true;
	//			AVGui::imp_flag = 2;
	//		}
	//	}
	//	else { // ddm
	//		if (ImGui::Button("Import mesh")) {
	//			AVGui::import_success[0] = checkFile(AVGui::buff01);
	//			AVGui::load_win = true;
	//			AVGui::imp_flag = 1;
	//		}
	//	}
	//}

	// **************************************************************

	// Loading window popup
	if (AVGui::load_win) {
		float _Y = m_screenH / 15.f;
		float _W = m_screenW / 5.5f;
		ImGui::SetNextWindowPos(ImVec2(m_screenW/2 - _W/2, m_screenH/2 - _Y/2));
		ImGui::SetNextWindowSize(ImVec2(_W, _Y));

		toolbar_flags = 0;
		toolbar_flags |= ImGuiWindowFlags_NoTitleBar;
		toolbar_flags |= ImGuiWindowFlags_NoResize;
		toolbar_flags |= ImGuiWindowFlags_NoMove;
		toolbar_flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Status Window", nullptr, toolbar_flags);

		// context window for background thread
		switch (AVGui::imp_flag) {
			case 0:
				if (!AVGui::import_success[3]) {
					ImGui::Text("Error loading fbx file."); ImGui::SameLine();
					if (ImGui::Button("Close")) {
						AVGui::load_win = false;
						AVGui::imp_flag = (unsigned)ImportType::NONE;
					}
				}
				else {
					ImGui::Text("Loading and Converting FBX...");
					if (!AVGui::thread_launch) {
						// load on separate thread
						AVGui::thread_launch = true;
					}
				}
				break;
			case 1:
				if (!AVGui::import_success[0]) {
					ImGui::Text("Error loading ddm file."); ImGui::SameLine();
					if (ImGui::Button("Close")) {
						AVGui::load_win = false;
						AVGui::imp_flag = (unsigned)ImportType::NONE;
					}
				}
				else {
					ImGui::Text("Loading Mesh file...");
					if (!AVGui::thread_launch) {
						// load on separate thread
						load_func = std::async(std::launch::async,
											   ResSpace::loadDDM,
											   (const char*)AVGui::buff01);
						AVGui::thread_launch = true;
					}
					else if (AVGui::thread_launch && AVGui::load_win) {
						if (load_func.wait_for(ms_10) == _status::ready) {
							AVGui::thread_launch = false;
							AVGui::load_win = false;
							AVGui::imp_flag = (unsigned)ImportType::NONE;

							// create new agent
							temp_mesh = load_func.get();
							ResSpace::loadModel_MD(res_ptr, "new_mesh", temp_mesh);
							temp_agent = ResSpace::getNewDD_Agent(res_ptr, "Dummy");
							temp_agent->AddModel("new_mesh", 0.f, 1000.f);
							ResSpace::loadAgentToGPU_M(res_ptr, "Dummy");
						}
					}
				}
				break;
			case 2:
				break;
			case 3:
				break;
			default:
				break;
		}

		ImGui::End();
	}

	ImGui::Separator();

	ImGui::End();
}

/// \brief Event handler for queue
DD_Event DD_AssetViewer::Update(DD_Event & event)
{
	if (event.m_type == "post") {
		setInterface(event.m_time);

		if (temp_modelsk && temp_modelsk->debugStatus()) {
			dbSk.render(temp_modelsk->m_debugSkeleton);
		}
	}
	return DD_Event();
}

void DD_AssetViewer::preLoad()
{
	const char* mesh_name = "mesh1";
	const char* mesh_name2 = "mesh2";
	const char* anim_name = "anim1";

	// skeleton
	temp_skeleton = ResSpace::loadDDB(
		res_ptr,
		//"/home/maadeagbo/Documents/FBX_Exporter/bin/skeleton.ddb",
		//"/home/maadeagbo/Documents/FBX_Exporter/bin/sit_right_t1.ddb",
		//"C:/Users/Moses/Documents/FBX_Exporter/meshes/vicon/vicon_moses.ddb",
		"C:/Users/Moses/Downloads/skele_t2.ddb",
		//"C:/Users/Moses/Documents/FBX_Exporter/meshes/mixamo/dancing_girl.ddb",
		"test_skele"
	);

	//mesh
	/*temp_mesh = ResSpace::loadDDG(
		"C:/Users/Moses/Documents/FBX_Exporter/meshes/mixamo/girlbot.ddg"
	);*/
	temp_mesh = ResSpace::loadDDM(
		//"/home/maadeagbo/Documents/FBX_Exporter/bin/Beta_Joints.ddm"
		"C:/Users/Moses/Documents/FBX_Exporter/meshes/vicon/Moses.ddm"
	);
	//temp_mesh = ResSpace::loadDDM(
	//	//"/home/maadeagbo/Documents/FBX_Exporter/bin/Beta_Joints.ddm"
	//	"C:/Users/Moses/Documents/FBX_Exporter/bin/Beta_Joints.ddm"
	//);

	//dd_array<MeshData> temp_mesh2 = ResSpace::loadDDM(
	//	//"/home/maadeagbo/Documents/FBX_Exporter/bin/Beta_Surface.ddm"
	//	"C:/Users/Moses/Documents/FBX_Exporter/bin/Beta_Surface.ddm"
	//);
	//dd_array<MeshData> all_mesh(temp_mesh.size() + temp_mesh2.size());
	//unsigned m_idx = 0;
	//for(auto& bin : {temp_mesh, temp_mesh2}) {
	//	for(unsigned i = 0; i < bin.size(); i++) {
	//		all_mesh[m_idx] = std::move(bin[i]);
	//		m_idx += 1;
	//	}
	//}

	temp_modelsk = ResSpace::loadSkinnedModel(
		res_ptr, mesh_name, "test_skele", temp_mesh);
	temp_modelsk->debugOn();
	temp_modelsk->m_daltonFlag = true;

	// animation
	temp_anim = ResSpace::loadDDA(
		res_ptr,
		//"/home/maadeagbo/Documents/FBX_Exporter/bin/Take 001_animLayer_0.dda",
		//"/home/maadeagbo/Documents/FBX_Exporter/bin/sit_right_t1.dda",
		//"C:/Users/Moses/Documents/FBX_Exporter/meshes/vicon/walk_fox_1_0.dda",
		"C:/Users/Moses/Downloads/23_h10_f10_t11_xpd_st.dda",
		//"C:/Users/Moses/Documents/FBX_Exporter/meshes/mixamo/dancing_girl_0.dda",
		anim_name
	);
	//temp_anim->length -= 2.5f;

	bool clip_added = ResSpace::addAnimationToModel(
		res_ptr, mesh_name, anim_name, "test");
	if (clip_added) {
		temp_modelsk->m_animStates[0].active = true;
		temp_modelsk->m_animStates[0].flag_loop = true;
		temp_modelsk->m_animStates[0].play_back = 0.8f;
	}

	//ResSpace::loadModel_MD(res_ptr, "soldier", temp_mesh);
	// dummy agent
	temp_agent = ResSpace::getNewDD_Agent(res_ptr, "Dummy");
	temp_agent->AddModelSK(mesh_name, 0.f, 1000.f);
	temp_agent->flag_render = false;

	// Debug skeleton
	dbSk.boxSize = 0.025f;
	dbSk.lineColor = glm::vec4(1.f, 0.f, 0.f, 1.f);
	dbSk.setup(res_ptr, "mdl_1", "ln_1", temp_modelsk, temp_skeleton);
}

/// \brief Checks if file exists
bool checkFile(const char *name)
{
	std::ifstream file(name);
	return file.is_open();
}

void SkeletonDebug::setup(DD_Resources* res,
					      const char* mdl_id,
					      const char* line_id,
						  const DD_ModelSK* mdlsk,
						  const DD_Skeleton* sk)
{
	boxRender = ResSpace::getNewDD_Agent(res, mdl_id);
	boxRender->AddModel("cube_prim", 0.f, 1000.f);
	lineRender = ResSpace::getNewDD_LineAgent(res, line_id);

	// setup boxes
	boxRender->SetInstances(mdlsk->m_debugSkeleton);

	// setup limbs
	unsigned line_count = 0;
	unsigned line_idx = 0;
	// if parent doesn't exist (i.e. parent = 0), skip counter increment
	for (unsigned i = 0; i < sk->m_bones.size(); i++) {
		DD_Joint& jnt = sk->m_bones[i];
		line_count = (jnt.m_parent == i) ? line_count : (line_count + 1);
	}
	lineRender->color = lineColor;
	lineIdxs.resize(line_count);
	lineRender->lines.resize(line_count);
	for (unsigned i = 0; i < sk->m_bones.size(); i++) {
		// create lines connecting joints (limb)
		DD_Joint& jnt = sk->m_bones[i];
		if (jnt.m_parent != i) {
			lineIdxs[line_idx] = glm::uvec2(i, jnt.m_parent);
			line_idx += 1;
		}
	}

	flagLoaded = true;
}

void SkeletonDebug::render(const dd_array<glm::mat4> mats)
{
	if (flagLoaded) {
		for (unsigned i = 0; i < mats.size(); i++) {
			boxRender->inst_m4x4[i] = glm::scale(mats[i], glm::vec3(boxSize));
		}
		for (unsigned i = 0; i < lineIdxs.size(); i++) {
			lineRender->lines[i] =
				{ mats[lineIdxs[i].x][3], mats[lineIdxs[i].y][3] };
		}
	}
}
