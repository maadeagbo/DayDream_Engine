#include "AssetNav.h"

namespace {
	float scroll_speed = 10000.0f, mouseSensitivity = 1.f, Pitch = 0.f, Yaw = 0.f,
		scroll_dist = 1000.f, pan_speed = 100.f;
	glm::quat deltaR;
}

AssetNav::AssetNav(const char * ID) {
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&AssetNav::Update, this, std::placeholders::_1);

	for (size_t i = 0; i < (size_t)AssetNavUI::COUNT; i++) { pressed[i] = false; }

	AddCallback("post", update);
	AddCallback("input", update);

	// implement my own update to movement
	locked_rot = false;
	ignore_controls = false;
}

DD_Event AssetNav::Update(DD_Event event) {
	// Parse throught events
	if (event.m_type == "input") {

		inputBuff* input = (inputBuff*)event.m_message;
		glm::vec3 f_d = ForwardDir() * pan_speed * event.m_time;
		glm::vec3 r_d = RightDir() * pan_speed * event.m_time;
		mouseX = input->mouseX;
		mouseY = input->mouseY;

		if (locked_rot) {
			if (input->mouseScroll > 0) {
				scroll_dist -= scroll_speed * event.m_time;
			}
			if (input->mouseScroll < 0) {
				scroll_dist += scroll_speed * event.m_time;
			}
		}
		else {
			if (!ignore_controls) { // when using imgui, if mouse is in window
				if (input->rawInput[DD_Keys::W_Key]) {
					UpdatePosition(pos() + f_d);
				}
				if (input->rawInput[DD_Keys::S_Key]) {
					UpdatePosition(pos() - f_d);
				}
				if (input->rawInput[DD_Keys::D_Key]) {
					UpdatePosition(pos() + r_d);
				}
				if (input->rawInput[DD_Keys::A_Key]) {
					UpdatePosition(pos() - r_d);
				}
			}
		}
		if (input->rawInput[DD_Keys::Space_Key]) {
			pressed[(u32)AssetNavUI::SPACE] = true;
		}
		if (input->rawInput[DD_Keys::Escape_Key]) {
			pressed[(u32)AssetNavUI::ESC] = true;
		}
		if (input->rawInput[DD_Keys::CTRL_L_Key]) {
			pressed[(u32)AssetNavUI::CTRL_L] = true;
		}
		if (input->rawInput[DD_Keys::ALT_L_Key]) {
			pressed[(u32)AssetNavUI::ALT_L] = true;
		}
		if (input->rawInput[DD_Keys::Shift_L_Key]) {
			pressed[(u32)AssetNavUI::SHIFT_L] = true;
		}
		if (input->mouseLMR[0]) {
			pressed[(u32)AssetNavUI::LMOUSE] = true;
		}
		if (input->mouseLMR[2]) {
			pressed[(u32)AssetNavUI::RMOUSE] = true;

			Pitch += input->mouseYDelta * mouseSensitivity * event.m_time;
			Yaw += input->mouseXDelta * mouseSensitivity * event.m_time;

			// constrain pitch to fps style camera (radians)
			if (Pitch > 1.3f) {
				Pitch = 1.3f;
			}
			if (Pitch < -1.3f) {
				Pitch = -1.3f;
			}
		}
		glm::quat new_rot = glm::rotate(glm::quat(), Yaw, glm::vec3(0.f, 1.f, 0.f));
		deltaR = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
		if (!locked_rot) {
			UpdateRotation(deltaR);
		}
		/*
		snprintf(char_buff, buff_size, "Pitch: %02d, Yaw: %02d", input->mouseYDelta,
		input->mouseXDelta);
		DD_Terminal::post(char_buff);
		//*/
	}
	if (event.m_type == "post") {
		// update instance
		if (locked_rot) {
			glm::mat4 _p = glm::translate(
				glm::mat4(), glm::vec3(0.f, 0.f, scroll_dist));
			glm::mat4 transM = glm::translate(glm::mat4(), pos());
			inst_m4x4[0] = transM * glm::mat4_cast(deltaR) * _p;
		}

		if (pressed[(u32)AssetNavUI::ESC]) {
			DD_Event new_event = DD_Event();

			flagBuff* fb = new flagBuff();
			fb->flag = true;
			new_event.m_type = "EXIT";
			new_event.m_message = fb;

			return new_event;
		}
	}
	return DD_Event();
}

void AssetNav::initRot(const float rad_pitch, const float rad_Yaw) {
	Pitch = rad_pitch;
	Yaw = rad_Yaw;
	glm::quat new_rot = glm::rotate(glm::quat(), Yaw, glm::vec3(0.f, 1.f, 0.f));
	new_rot = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
	UpdateRotation(new_rot);
}

/// \brief set camera rotation type to free flow or locked
void AssetNav::lockRotMode(const bool set)
{
	if (set) {
		locked_rot = true;
		override_inst_update();
	} 
	else {
		locked_rot = false;
		default_inst_update();
	}
}
