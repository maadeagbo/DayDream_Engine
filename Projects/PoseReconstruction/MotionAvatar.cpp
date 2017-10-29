#include "MotionAvatar.h"
#include "DD_Terminal.h"
#pragma GCC diagnostic ignored "-Wformat-security"

const float EPSILON = 0.001f;

void SkeletonDebug_PV::setup(DD_Resources* res,
							 const char* mdl_id,
							 const char* line_id,
							 const char* skeleton_id)
{
	res_ptr = res;
	// create new line agent
	lineRender.set(line_id);
	DD_LineAgent* l_agent = ResSpace::getNewDD_LineAgent(res, line_id);

	// setup boxes
	DD_Skeleton* sk = ResSpace::findDD_Skeleton(res, skeleton_id);
	if (!sk) {
		DD_Terminal::f_post("[error] SkeletonDebug_PV::setup::failed to locate"
							" skeleton <%s>. Quiting", skeleton_id);
		return;
	}
	dd_array<glm::mat4> mats(sk->m_bones.size());

	// create new agent and load to gpu at runtime
	boxRender.set(mdl_id);
	DD_Agent* b_agent = ResSpace::getNewDD_Agent(res, mdl_id);
	b_agent->AddModel("cube_prim", 0.f, 1000.f);
	b_agent->SetInstances(mats);
	ResSpace::loadAgent_ID(res, mdl_id);

	// setup limbs
	unsigned line_count = 0;
	unsigned line_idx = 0;
	// if parent doesn't exist (i.e. parent = 0), skip counter increment
	for (unsigned i = 0; i < sk->m_bones.size(); i++) {
		DD_Joint& jnt = sk->m_bones[i];
		line_count = (jnt.m_parent == i) ? line_count : (line_count + 1);
	}
	l_agent->color = lineColor;
	lineIdxs.resize(line_count);
	l_agent->lines.resize(line_count);
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

void SkeletonDebug_PV::render(const dd_array<glm::mat4> mats)
{
	if (flagLoaded) {
		DD_LineAgent* l_agent = 
			ResSpace::findDD_LineAgent(res_ptr, lineRender.str());
		DD_Agent* b_agent = ResSpace::findDD_Agent(res_ptr, boxRender.str());

		for (unsigned i = 0; i < mats.size(); i++) {
			b_agent->inst_m4x4[i] = glm::scale(mats[i], glm::vec3(boxSize));
		}
		// apply parent transform to lines
		glm::mat4 parent = b_agent->parent_transform;
		l_agent->color = lineColor;
		for (unsigned i = 0; i < lineIdxs.size(); i++) {
			l_agent->lines[i] =
			{ (parent * mats[lineIdxs[i].x])[3],
				(parent * mats[lineIdxs[i].y])[3] };
		}
	}
}

MotionAvatar::MotionAvatar(const char * ID) :
	clickedP(false),
	clickedR(false),
	clickedSpace(false),
	ignore_controls(false),
	edit_window(false)
{
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&MotionAvatar::Update, this, std::placeholders::_1);

	AddCallback("post", update);
	AddCallback("input", update);
	// create command for modifying position
	edit_cmd.format("edit_%s", ID);
	AddCallback(edit_cmd.str(), update);

	// turn off render
	flag_render = false;
}

DD_Event MotionAvatar::Update(DD_Event event) {
	DD_ModelSK* skinnedMdl =
		ResSpace::findDD_ModelSK(res_ptr, skinnedMdl_id.str());

	// Parse throught events
	if (event.m_type == "input") {
		lastClick += event.m_time;

		inputBuff* input = (inputBuff*)event.m_message;

		if (!ignore_controls) {
			if (input->rawInput[DD_Keys::P_Key] && lastClick > 0.5f) {
				clickedP ^= 1;
                lastClick = 0.f;
			}
			if (input->rawInput[DD_Keys::R_Key] && lastClick > 0.5f) {
				clickedR ^= 1;
                lastClick = 0.f;
			}
			if (input->rawInput[DD_Keys::Space_Key] && lastClick > 0.5f) {
				clickedSpace ^= 1;
                lastClick = 0.f;
			}
		}
    }
	if (event.m_type == "post") {
		DD_Event new_event = DD_Event();

		setInterface(event.m_time);

		if (skinnedMdl && skinnedMdl->debugStatus()) {
			//skeleton_viewer.boxRender
			skeleton_viewer.render(skinnedMdl->m_debugSkeleton);
		}

		return new_event;
	}
	if (event.m_type == edit_cmd.str()) { // turn on edit window
		edit_window = true;
	}
	return DD_Event();
}

// ImGui interface for interaction
void MotionAvatar::setInterface(const float dt)
{
	DD_ModelSK* skinnedMdl =
		ResSpace::findDD_ModelSK(res_ptr, skinnedMdl_id.str());
	if (!skinnedMdl) { return; }

	if (edit_window) {
		// get io for mouse & keyboard management
		ImGuiIO& imgui_io = ImGui::GetIO();
		ignore_controls = imgui_io.WantCaptureMouse;

		// Manipulate position and rotation
		glm::vec4& gui_color = skeleton_viewer.lineColor;
		ImColor col(gui_color.x, gui_color.y, gui_color.z, gui_color.w);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TitleBg, col);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TitleBgActive, col);

		ImGui::Begin(m_ID.c_str(), &edit_window, 0);

		glm::vec3 temp = pos();
		ImGui::DragFloat3("Position", &temp[0], 0.1f);
		UpdatePosition(temp);

		ImGui::DragFloat3("Rotation", &gui_rot[0], 0.1f);
		temp.x = glm::radians(gui_rot.x);
		temp.y = glm::radians(gui_rot.y);
		temp.z = glm::radians(gui_rot.z);
		UpdateRotation(glm::quat(temp));

		ImGui::DragFloat3("Color", &gui_color[0], 0.01f, 0.f, 1.f);

		ImGui::Checkbox("Global Animation", &skinnedMdl->m_daltonFlag);

		// control animations if visible
		//strbuff.format("%s_scrollregion", m_ID.c_str());
		//ImGui::BeginChild(strbuff.str());
		for (unsigned i = 0; i < skinnedMdl->m_animStates.size(); i++) {
			DD_AnimState* a_state = &skinnedMdl->m_animStates[i];

			// active state should always be true (if false, set true and pause)
			// for Dalton's implementation, freeze the frame at the end
			if (!a_state->active) {
				a_state->active = true;
				a_state->pause = true;
			}

			ImGui::Separator();
			ImGui::Text(a_state->m_ID.str());
			ImGui::DragFloat("Playback Speed", &a_state->play_back, EPSILON);

			const bool play_flag = !a_state->active || a_state->pause;
			const char* playpause = (play_flag) ? "Play" : "Pause";
			if (ImGui::Button(playpause)) {
				if (play_flag) {
					a_state->pause = false;
				}
				else {
					a_state->pause = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset")) { a_state->local_time = 0.f; }
			ImGui::SameLine();
			if (ImGui::Button("Last Frame")) {
				a_state->local_time = a_state->fscratch[1] - EPSILON;
			}
			ImGui::Checkbox("Loop", &a_state->flag_loop);
			ImGui::SameLine();
			ImGui::Checkbox("Interpolate", &a_state->interpolate);

			// step thru animation
			if (a_state->fscratch.isValid()) {
				ImGui::DragFloat(" ",
								 &a_state->local_time,
								 a_state->fscratch[0],
								 EPSILON,
								 a_state->fscratch[1] - EPSILON);
			}
			ImGui::SameLine();

			// Step thru animation frames
			int curr_idx =
				(int)(a_state->local_time / a_state->fscratch[0]);
			if (ImGui::Button("<")) {
				a_state->local_time = a_state->fscratch[0] * (curr_idx - 1);
				a_state->local_time = (a_state->local_time < EPSILON) ?
					EPSILON : a_state->local_time;
			}
			ImGui::SameLine();
			if (ImGui::Button(">")) {
				a_state->local_time = a_state->fscratch[0] * (curr_idx + 1);
				a_state->local_time =
					(a_state->local_time >= a_state->fscratch[1]) ?
					a_state->fscratch[1] - EPSILON : a_state->local_time;
			}
		}
		//ImGui::EndChild();

		ImGui::End();

		ImGui::PopStyleColor(2);
	}
}

/// \brief Set animations to process as global frames
void MotionAvatar::toggleGlobalConfig()
{
	DD_ModelSK* skinnedMdl = 
		ResSpace::findDD_ModelSK(res_ptr, skinnedMdl_id.str());
	skinnedMdl->m_daltonFlag ^= 1;
}

/// \brief Load Skinned mesh model to agent
void MotionAvatar::load(DD_Resources* res,
						const char * mdl_sk_id,
						const char * mdl_sk_path,
						const char * skeleton_id,
						const char * skeleton_path)
{
	// save id and resource bin pointer
	res_ptr = res;
	skinnedMdl_id = mdl_sk_id;

	DD_ModelSK* skinnedMdl = ResSpace::loadSkinnedModel(
		res, mdl_sk_id, mdl_sk_path, skeleton_id, skeleton_path);
	if (!skinnedMdl) {
		DD_Terminal::f_post("[error] MotionAvatar::Load::<%s>::failed to "
							"load skinned model <%s>", m_ID.c_str(), mdl_sk_id);
		return;
	}
	skinnedMdl->debugOn();
	skinnedMdl->m_daltonFlag = true;

	// set debug skeleton
	cbuff<64> mdl_debug_id, sk_debug_id;
	mdl_debug_id.format("%s_dbBox", m_ID.c_str());
	sk_debug_id.format("%s_dbLine", m_ID.c_str());

	skeleton_viewer.boxSize = 0.025f;
	skeleton_viewer.lineColor = glm::vec4(1.f, 0.f, 0.f, 1.f);
	skeleton_viewer.setup(
		res, mdl_debug_id.str(), sk_debug_id.str(), skeleton_id);
	// parent box render to this agent
	if (skeleton_viewer.active()) {
		DD_Agent* b_agent = ResSpace::findDD_Agent(res, mdl_debug_id.str());
		if (b_agent) { b_agent->SetParent(m_ID.c_str()); }
	}
}

bool MotionAvatar::addAnimation(const char * anim_id,
								const char * new_ref_id)
{
	DD_ModelSK* skinnedMdl = 
		ResSpace::findDD_ModelSK(res_ptr, skinnedMdl_id.str());
	if (!skinnedMdl) { return false; }

	unsigned idx = skinnedMdl->m_animStates.size();
	bool added = ResSpace::addAnimationToModel(
		res_ptr, skinnedMdl->m_ID.c_str(), anim_id, new_ref_id);
	if (added) {
		skinnedMdl->m_animStates[idx].active = true;
		skinnedMdl->m_animStates[idx].pause = true;
		// get step size and clip length
		DD_AnimClip* a_clip = ResSpace::findDD_AnimClip(res_ptr, anim_id);
		if (a_clip) {
			skinnedMdl->m_animStates[idx].fscratch.resize(3);
			skinnedMdl->m_animStates[idx].fscratch[0] = a_clip->step_size;
			skinnedMdl->m_animStates[idx].fscratch[1] = a_clip->length;
			skinnedMdl->m_animStates[idx].fscratch[2] = a_clip->num_frames;
		}
	}
	return added;
}
