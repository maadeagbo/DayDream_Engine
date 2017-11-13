#include "AssetViewAvatar.h"
#include "DD_Terminal.h"

#ifdef __linux__
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

const float EPSILON = 0.001f;

AssetViewAvatar::AssetViewAvatar(const char * ID) :
	clickedP(false),
	clickedR(false),
	clickedSpace(false),
	ignore_controls(false),
	edit_window(false)
{
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&AssetViewAvatar::Update, this, std::placeholders::_1);

	AddCallback("viewer", update);
	AddCallback("input", update);
	AddCallback("add_anim", update);
	AddCallback("edit", update);
}

DD_Event AssetViewAvatar::Update(DD_Event event)
{
	if (event.m_type == "input") {
		lastClick += event.m_time;
		inputBuff* input = (inputBuff*)event.m_message;

		if (!ignore_controls) {
			if (input->rawInput[DD_Keys::P_Key] && lastClick > 0.5f) {
				clickedP = true;
				lastClick = 0.f;
			}
			if (input->rawInput[DD_Keys::R_Key] && lastClick > 0.5f) {
				clickedR = true;
				lastClick = 0.f;
			}
			if (input->rawInput[DD_Keys::Space_Key] && lastClick > 0.5f) {
				clickedSpace = true;
				lastClick = 0.f;
			}
		}
	}
	if (event.m_type == "viewer") {
		setInterface(event.m_time);
	}

	if (event.m_type == "add_anim") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args =
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 3) {
				// check if this agent id matches arg 0
				bool same = args[0].compare(m_ID.c_str()) == 0;
				if (same) { appendAnimation(args[1].str(), args[2].str()); }
			}
			else {
				DD_Terminal::post("add_anim <agent id> "
								  "<anim id> <agent anim reference id>");
			}
		}
	}
	if (event.m_type == "edit") {
		messageBuff* _m = static_cast<messageBuff*>(event.m_message);
		if (_m) {
			dd_array<cbuff<256>> args =
				StrSpace::tokenize512<256>(_m->message512, " ");
			if (args.size() >= 1) {
				// check if this agent id matches arg 0
				bool same = args[0].compare(m_ID.c_str()) == 0;
				if (same) { edit_window ^= 1; }
			}
		}
	}
	return DD_Event();
}

void AssetViewAvatar::setInterface(const float dt)
{
	DD_ModelSK* skinnedMdl =
		ResSpace::findDD_ModelSK(res_ptr, skinnedMdl_id.str());
	if (!skinnedMdl) { return; }

	if (edit_window) {
		// get io for mouse & keyboard management
		ImGuiIO& imgui_io = ImGui::GetIO();
		ignore_controls = imgui_io.WantCaptureMouse;

		ImGui::Begin(m_ID.c_str(), &edit_window, 0);

		glm::vec3 temp = pos();
		ImGui::DragFloat3("Position", &temp[0], 0.1f);
		UpdatePosition(temp);

		ImGui::DragFloat3("Rotation", &gui_rot[0], 0.1f);
		temp.x = glm::radians(gui_rot.x);
		temp.y = glm::radians(gui_rot.y);
		temp.z = glm::radians(gui_rot.z);
		UpdateRotation(glm::quat(temp));

		ImGui::Checkbox("Global Animation", &skinnedMdl->m_daltonFlag);

		for (unsigned i = 0; i < skinnedMdl->m_animStates.size(); i++) {
			DD_AnimState* a_state = &skinnedMdl->m_animStates[i];

			ImGui::Separator();
			ImGui::Text(a_state->m_ID.str());
			strbuff.format("%s_%u", "Playback Speed", i);
			ImGui::DragFloat(strbuff.str(), &a_state->play_back, EPSILON);
			strbuff.format("%s_%u", "Active", i);
			if (ImGui::Checkbox(strbuff.str(), &a_state->active)) {
				a_state->local_time = 0.f;
			}
			ImGui::SameLine();
			strbuff.format("%s_%u", "Loop", i);
			ImGui::Checkbox(strbuff.str(), &a_state->flag_loop);
			ImGui::SameLine();
			strbuff.format("%s_%u", "Interpolate", i);
			ImGui::Checkbox(strbuff.str(), &a_state->interpolate);
			strbuff.format("%s_%u", "Reset", i);
			if (ImGui::Button(strbuff.str())) {
				a_state->active = true;
				a_state->local_time = 0.f;
			}
			ImGui::SameLine();
			strbuff.format("%s_%u", "Weight", i);
			ImGui::DragFloat(strbuff.str(), &a_state->weight, 0.1f, 0.f, 1.f);
		}

		ImGui::End();
	}
}

void AssetViewAvatar::appendAnimation(const char * animation_id, 
									  const char * new_ref_id)
{
	DD_ModelSK* skinnedMdl =
		ResSpace::findDD_ModelSK(res_ptr, skinnedMdl_id.str());
	if (!skinnedMdl) { 
		DD_Terminal::f_post("[error] DD_ModelSk <%s> could not be found. " 
							"Aborting animation append.", animation_id);
		return; 
	}


	unsigned idx = (unsigned)skinnedMdl->m_animStates.size();
	bool added = ResSpace::addAnimationToModel(
		res_ptr, skinnedMdl->m_ID.c_str(), animation_id, new_ref_id);

	if (added) {
		//skinnedMdl->m_animStates[idx].active = true;
		// get step size and clip length
		DD_AnimClip* a_clip = ResSpace::findDD_AnimClip(res_ptr, animation_id);

		if (a_clip) {
			skinnedMdl->m_animStates[idx].fscratch.resize(3);
			skinnedMdl->m_animStates[idx].fscratch[0] = a_clip->step_size;
			skinnedMdl->m_animStates[idx].fscratch[1] = a_clip->length;
			skinnedMdl->m_animStates[idx].fscratch[2] = (float)a_clip->num_frames;
		}
	}
	else {
		DD_Terminal::f_post("[error] Error when adding animation <%s>. Aborting",
							new_ref_id);
		return;
	}
}
