#include "PoseView.h"
#include "ControllerP.h"

#define CHAR_BUFF_SIZE 512
#define MAX_M_AVATERS 50

namespace {
	ControllerP* myControl;
	DD_Camera* cam;
	DD_Light* light;
	DD_LineAgent* grid_h;
    DD_LineAgent* grid_v;

	std::future<dd_array<MeshData>> load_func;
	std::chrono::milliseconds ms_10(10);
	typedef std::future_status _status;

	cbuff<64> mdlsk_names[MAX_M_AVATERS];
}

namespace PGui
{
	float bkgr_col[4] = { 0.1f, 0.1f, 0.1f, 0.98f };
	bool thread_launch = false;
	char buff01[CHAR_BUFF_SIZE];
	char buff02[CHAR_BUFF_SIZE] = "\"ddg, .ddm, .ddb, or .dda\"";
	char buff03[CHAR_BUFF_SIZE];
}

/// \brief Initialze controllers, camera, ui, etc... for AssetViewer
void PoseView::Init()
{
	// asset file
	m_assetFile = std::string(PROJECT_DIR) + "PoseReconstruction/assets";

	// register skybox map
	//m_cubeMapID = "water";

	// Grid horizontal
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

	// Grid vertical
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
	myControl = new ControllerP("avatar");
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
    
    //  callbacks and posts
	EventHandler a = std::bind(&PoseView::Update, this, std::placeholders::_1);
	AddCallback("post", a);
	AddCallback("load_anim", a);
	AddCallback("load_sk", a);
	AddCallback("load_agent", a);
	AddCallback("add_anim", a);
	AddCallback("play_all", a);
	AddCallback("pause_all", a);
	AddCallback("del_agent", a);

	total_agents = 0;
}

/// \brief Manages ImGui Interface
void PoseView::setInterface(const float dt) 
{
	// get io for mouse & keyboard management
	ImGuiIO& imgui_io = ImGui::GetIO();
	myControl->ignore_controls = imgui_io.WantCaptureMouse;
}

/// \brief Event handler for queue
DD_Event PoseView::Update(DD_Event & event)
{
	if (event.m_type == "post") {
		setInterface(event.m_time);
	}
	if (event.m_type == "play_all") {
		playAll();
	}
	if (event.m_type == "pause_all") {
		pauseAll();
	}
	if (event.m_type == "load_anim") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		dd_array<cbuff<256>> args = 
			StrSpace::tokenize512<256>(_m->message512, " ");
		if (args.size() >= 2) {
			loadAnimDDA(args[0].str(), args[1].str());
		}
	}
	if (event.m_type == "load_sk") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		dd_array<cbuff<256>> args =
			StrSpace::tokenize512<256>(_m->message512, " ");
		if (args.size() >= 2) {
			loadSkeletonDDB(args[0].str(), args[1].str());
		}
	}
	if (event.m_type == "load_agent") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		dd_array<cbuff<256>> args =
			StrSpace::tokenize512<256>(_m->message512, " ");
		if (args.size() >= 2) {
			createAgent(args[0].str(), args[1].str());
		}
	}
	if (event.m_type == "add_anim") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		dd_array<cbuff<256>> args =
			StrSpace::tokenize512<256>(_m->message512, " ");
		if (args.size() >= 3) {
			addAnimationToAgent(args[0].str(), args[1].str(), args[2].str());
		}
	}
	if (event.m_type == "del_agent") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		dd_array<cbuff<256>> args =
			StrSpace::tokenize512<256>(_m->message512, " ");
		if (args.size() >= 1) { deleteAgent(args[0].str()); }
	}
	return DD_Event();
}

/// \brief Load DD_AnimClip to memory
bool PoseView::loadAnimDDA(const char * anim_id, const char * path) 
{
	DD_AnimClip* clip = ResSpace::loadDDA(res_ptr, path, anim_id);
	if (!clip) {
		DD_Terminal::f_post("[error] PoseView::loadAnimDDA::<%s> failed to load",
							anim_id);
		return false;
	}
	DD_Terminal::f_post("PoseView::Added <%s> to memory", anim_id);
	return true;
}

/// \brief Load DD_Skeleton to memory
bool PoseView::loadSkeletonDDB(const char * sk_id, const char * path)
{
	DD_Skeleton* sk = ResSpace::loadDDB(res_ptr, path, sk_id);
	if (!sk) {
		DD_Terminal::f_post("[error] PoseView::loadSkeletonDDB::<%s>"
							" failed to load", sk_id);
		return false;
	}
	DD_Terminal::f_post("PoseView::Skeleton::Added <%s> to memory", sk_id);
	return true;
}

/// \brief Attach DD_ModelSK to current agent (name length must be < 64)
bool PoseView::createAgent(const char * agent_id, const char * sk_id)
{
	MotionAvatar* test_dude = new MotionAvatar(agent_id);
	// add skinned model
	mdlsk_names[total_agents].format("%s_model", agent_id);
	test_dude->load(res_ptr, mdlsk_names[total_agents].str(), "", sk_id, "");

	if (!ResSpace::AddAgent(res_ptr, test_dude)) { return false; };
	ResSpace::loadAgent_ID(res_ptr, agent_id);

	total_agents += 1;
	return true;
}

/// \brief Create animation state and attach to agent
bool PoseView::addAnimationToAgent(const char * agent_id,
								   const char * anim_id,
								   const char * new_state_id)
{
	MotionAvatar* agent = 
		static_cast<MotionAvatar*>(ResSpace::findDD_Agent(res_ptr, agent_id));
	if (!agent) {
		DD_Terminal::f_post("[error] PoseView::AddAnim::Invalid agent id <%s>", 
							agent_id);
		return false;
	}
	agent->addAnimation(anim_id, new_state_id);
	return true;
}

/// \brief Delete agent and debug objects
void PoseView::deleteAgent(const char * agent_id)
{
	MotionAvatar* m_avatar = static_cast<MotionAvatar*>(
		ResSpace::findDD_Agent(res_ptr, agent_id));
	if (m_avatar) {
		// delete box render agent
		ResSpace::removeDD_Agent(
			res_ptr, m_avatar->skeleton_viewer.boxRender.str());
		// delete line render agent
		ResSpace::removeDD_LineAgent(
			res_ptr, m_avatar->skeleton_viewer.lineRender.str());
		// delete agent
		ResSpace::deleteAgent(res_ptr, agent_id);
	}
	else {
		DD_Terminal::f_post("[error] PoseView::Delete::Invalid agent id <%s>", 
							agent_id);
	}
}

/// \brief Activate every created MotionAvatar
void PoseView::playAll()
{
	for (unsigned i = 0; i < total_agents; i++) {
		DD_ModelSK* mdlsk = 
			ResSpace::findDD_ModelSK(res_ptr, mdlsk_names[i].str());
		if (mdlsk) {
			for (unsigned j = 0; j < mdlsk->m_animStates.size(); j++) {
				mdlsk->m_animStates[j].active = true;
				mdlsk->m_animStates[j].pause = false;
			}
		}
	}
}

/// \brief Pause every created MotionAvatar
void PoseView::pauseAll()
{
	for (unsigned i = 0; i < total_agents; i++) {
		DD_ModelSK* mdlsk =
			ResSpace::findDD_ModelSK(res_ptr, mdlsk_names[i].str());
		if (mdlsk) {
			for (unsigned j = 0; j < mdlsk->m_animStates.size(); j++) {
				mdlsk->m_animStates[j].pause = true;
			}
		}
	}
}
