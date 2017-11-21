#include "DD_AssetViewer.h"
#include "DD_Terminal.h"
#include "AssetViewAvatar.h"
#include <fstream>

void createNewModel(DD_Resources* res_ptr, const char* id, const char* path);
void createNewSkeleton(DD_Resources* res_ptr, const char* id, const char* path);
void createNewModelSk(DD_Resources* res_ptr, 
					const char* id, 
					const char* path, 
					const char* skeleton_id);
void createNewAgentSk(DD_Resources* res_ptr, 
					const char* agent_id, 
					const char* mdlsk_id);
void createNewAnimation(DD_Resources* res_ptr,
						const char* id,
						const char* path);

enum class ImportType : unsigned
{
	FBX,
	DDM,
	DDM_S,
	DDM_S_A,
	NONE
};

namespace {
	AssetNav* myControl;
	DD_Camera* cam;
	DD_Light* light;

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

DD_AssetViewer::DD_AssetViewer()
{
	// set background color
	bgcolor[0] = 0.5f;
	bgcolor[1] = 0.5f;
	bgcolor[2] = 0.5f;
	bgcolor[3] = 0.f;
}

/// \brief Initialze controllers, camera, ui, etc... for AssetViewer
void DD_AssetViewer::Load()
{
	// asset file
	m_assetfile = std::string(ROOT_DIR) + "Core/AssetViewer/assets";
	// Grid
	//*
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
	DD_LineAgent* grid_h = ResSpace::getNewDD_LineAgent(res_ptr, "grid_h");
	grid_h->color = glm::vec4( 1.f);
	grid_h->lines = std::move(h_lines);
	for (size_t i = 0; i < 10; i++) {
		v_lines[i] = { glm::vec4(_w * i, 0.f, length, 1.f),
			glm::vec4(_w * i, 0.f, -length, 1.f) };
	}
	for (size_t i = 9; i > 0; i--) {
		v_lines[v_lines.size() - i] = { glm::vec4(-_w * i, 0.f, length, 1.f),
			glm::vec4(-_w * i, 0.f, -length, 1.f) };
	}
	DD_LineAgent* grid_v = ResSpace::getNewDD_LineAgent(res_ptr, "grid_v");
	grid_v->color = glm::vec4(1.f);
	grid_v->lines = std::move(v_lines);
	//*/
	
	//*
	// floor
	DD_Agent* temp_agent = ResSpace::getNewDD_Agent(res_ptr, "Floor");
	if (temp_agent) {
		cbuff<256> plane_id;
		plane_id.format("%s%s", MESH_DIR, "primitives/plane.ddm");
		DD_Model* mdl = ResSpace::loadModel_DDM(res_ptr, "pl", plane_id.str());
		if (mdl) {
			temp_agent->AddModel("pl", 0.f, 1000.f);
			temp_agent->UpdateScale(glm::vec3(100.f));
			temp_agent->UpdatePosition(glm::vec3(0.f, -0.1f, 0.f));
		}
	}
	//*/

	// controller
	myControl = new AssetNav("avatar");
	res_ptr->agents[res_ptr->m_num_agents] = myControl;
	res_ptr->m_num_agents += 1;
	myControl->initRot(glm::radians(-35.f), glm::radians(45.f));

	// camera
	cam = ResSpace::getNewDD_Camera(res_ptr, "myCam");
	cam->active = true;
	cam->near_plane = 0.1f;
	cam->far_plane = 2500.0f;
	cam->SetParent(myControl->m_ID.c_str());

	// Add shadow light
	light = ResSpace::getNewDD_Light(res_ptr, "main_light");
	light->_position = glm::vec3(0.0f, 700.f, 500.0f);
	light->m_color = glm::vec3(1.f);
	light->m_flagShadow = true;

	myControl->lockRotMode(true);
	if (myControl->locked_rot) {
		myControl->UpdatePosition(glm::vec3(0.0f, 50.0f, 0.0f));
	} 
	else {
		myControl->UpdatePosition(glm::vec3(110.0f, 110.0f, 110.0f));
	}
}


/// \brief Manages ImGui Interface
void DD_AssetViewer::setInterface(const float dt)
{
	//const float debugY = m_screenH - 60.f;
	//const float debugW = m_screenW / 5.f;

	// get io for mouse & keyboard management
	ImGuiIO& imgui_io = ImGui::GetIO();
	myControl->ignore_controls = imgui_io.WantCaptureMouse;
}

/// \brief Event handler for queue
DD_Event DD_AssetViewer::Update(DD_Event & event)
{
	if (event.m_type == "viewer") {
		setInterface(event.m_time);
	}
	if (event.m_type == "new_mdl") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args = 
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 2) {
				createNewModel(res_ptr, args[0].str(), args[1].str());
			}
			else {
				DD_Terminal::post("new_mdl <model id> <model path>");
			}
		}
	}
	if (event.m_type == "new_sk") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args =
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 2) {
				createNewSkeleton(res_ptr, args[0].str(), args[1].str());
			}
			else {
				DD_Terminal::post("new_sk <skeleton id> <skeleton path>");
			}
		}
	}
	if (event.m_type == "new_mdlsk") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args =
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 3) {
				createNewModelSk(res_ptr, 
							   args[0].str(), 
							   args[1].str(), 
							   args[2].str());
			}
			else {
				DD_Terminal::post("new_mdlsk <model id> <model path> "
								  "<skeleton id>");
			}
		}
	}
	if (event.m_type == "new_agent_sk") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args =
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 2) {
				createNewAgentSk(res_ptr, args[0].str(), args[1].str());
			}
			else {
				DD_Terminal::post("new_agent_sk <agent id> <modelsk id>");
			}
		}
	}
	if (event.m_type == "new_anim") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args =
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 2) {
				createNewAnimation(res_ptr, args[0].str(), args[1].str());
			}
			else {
				DD_Terminal::post("new_anim <animation id> <animation path>");
			}
		}
	}
	return DD_Event();
}

/// \brief Load new DD_ModelSK object
void createNewModel(DD_Resources * res_ptr, const char * id, const char * path)
{
	DD_Model *mdl = ResSpace::loadModel_DDM(res_ptr, id, path);
	if (mdl) {
		DD_Terminal::f_post("[status] DD_Model loaded: %s", id);
	}
	else {
		DD_Terminal::f_post("[error] DD_Model not loaded: %s", id);
	}
	return;
}

/// \brief Load new DD_Skeleton object
void createNewSkeleton(DD_Resources * res_ptr, const char * id, const char * path)
{
	DD_Skeleton *sk = ResSpace::loadDDB(res_ptr, path, id);
	if (sk) {
		DD_Terminal::f_post("[status] DD_Skeleton loaded: %s", id);
	}
	else {
		DD_Terminal::f_post("[error] DD_Skeleton not loaded: %s", id);
	}
	return;
}

/// \brief Load new DD_ModelSk object
void createNewModelSk(DD_Resources * res_ptr, 
					const char * id, 
					const char * path, 
					const char * skeleton_id)
{
	DD_ModelSK* mdlsk = 
		ResSpace::loadSkinnedModel(res_ptr, id, path, skeleton_id, "");
	if (mdlsk) {
		DD_Terminal::f_post("[status] DD_ModelSk loaded: %s", id);
	}
	else {
		DD_Terminal::f_post("[error] DD_ModelSk not loaded: %s", id);
	}

	return;
}

/// \brief Create new agent w/ skinned Model object
void createNewAgentSk(DD_Resources * res_ptr, 
					const char * agent_id, 
					const char * mdlsk_id)
{
	DD_ModelSK *mdl = ResSpace::findDD_ModelSK(res_ptr, mdlsk_id);
	if (!mdl) {
		DD_Terminal::f_post("[error] DD_ModelSk <%s> not found. Aborting "
							"agent creation.", mdlsk_id);
		return;
	}
	// check if agent already exists using id
	DD_Agent *agent = nullptr;
	agent = ResSpace::findDD_Agent(res_ptr, agent_id);
	if (agent) {
		DD_Terminal::f_post("[error] DD_Agent <%s> already exists. Aborting "
							"agent creation.", agent_id);
		return;
	}
	// create agent using AssetViewAvatar
	AssetViewAvatar* avatar = new AssetViewAvatar(agent_id);
	// maybe wrap in if statement if failure to allocate?
	avatar->res_ptr = res_ptr;
	avatar->skinnedMdl_id = mdlsk_id;
	avatar->AddModelSK(mdlsk_id, 0.f, 1000.f);
	
	agent = ResSpace::AddAgent(res_ptr, avatar);
	if (!agent) {
		delete avatar;
		DD_Terminal::post("[error] Failed to add agent to resources. Aborting");
		return;
	}
	// load to gpu
	ResSpace::loadAgent_ID(res_ptr, agent_id);
	DD_Terminal::f_post("[status] Agent loaded: %s", agent_id);

	return;
}

/// \brief Create new DD_Animation
void createNewAnimation(DD_Resources * res_ptr, 
						const char * id, 
						const char * path)
{
	DD_AnimClip* clip = ResSpace::loadDDA(res_ptr, path, id);
	if (!clip) {
		DD_Terminal::post("[error] Failed to add animation to resources. Aborting");
		return;
	}
	DD_Terminal::f_post("[status] Animation loaded: <%s>", id);
	return;
}
