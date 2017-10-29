#include "Navigation.h"

namespace {
	const size_t buff_size = 64;
	char char_buff[buff_size];

	float scroll_speed = 10000.0f, mouseSensitivity = 1.f, Pitch = 0.f, Yaw = 0.f,
		scroll_dist = 1000.f;
	glm::quat deltaR;
}

Navigation::Navigation(const char * ID, const char * model, const char * parent) {
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&Navigation::Update, this, std::placeholders::_1);

	for (size_t i = 0; i < (size_t)NavUI::COUNT; i++) { pressed[i] = false; }

	AddCallback("post", update);
	AddCallback("input", update);

	// implement my own update to movement
	override_inst_update();
}

DD_Event Navigation::Update(DD_Event event) {
	// Parse throught events
	if (event.m_type == "input") {

		inputBuff* input = (inputBuff*)event.m_message;

		mouseX = input->mouseX;
		mouseY = input->mouseY;

		if (input->rawInput[DD_Keys::Space_Key]) {
			pressed[NavUI::SPACE] = true;
		}
		if (input->rawInput[DD_Keys::Escape_Key]) {
			pressed[NavUI::ESC] = true;
		}
		if (input->rawInput[DD_Keys::CTRL_L_Key]) {
			pressed[NavUI::CTRL_L] = true;
		}
		if (input->rawInput[DD_Keys::ALT_L_Key]) {
			pressed[NavUI::ALT_L] = true;
		}
		if (input->rawInput[DD_Keys::Shift_L_Key]) {
			pressed[NavUI::SHIFT_L] = true;
		}
		if (input->mouseScroll > 0) {
			scroll_dist -= scroll_speed * event.m_time;
		}
		if (input->mouseScroll < 0) {
			scroll_dist += scroll_speed * event.m_time;
		}
		if (input->mouseLMR[0]) {
			pressed[NavUI::LMOUSE] = true;
		}
		if (input->mouseLMR[2]) {
			pressed[NavUI::RMOUSE] = true;

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
		/*
		snprintf(char_buff, buff_size, "Pitch: %02d, Yaw: %02d", input->mouseYDelta,
			input->mouseXDelta);
		DD_Terminal::post(char_buff);
		//*/
	}
	if (event.m_type == "post") {
		// update instance
		glm::mat4 _p = glm::translate(glm::mat4(), glm::vec3(0.f, 0.f, scroll_dist));
		glm::mat4 transM = glm::translate(glm::mat4(), pos());
		inst_m4x4[0] = transM * glm::mat4_cast(deltaR) * _p;

		if (pressed[NavUI::ESC]) {
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

void Navigation::initRot(const float rad_pitch, const float rad_Yaw) {
	Pitch = rad_pitch;
	Yaw = rad_Yaw;
	glm::quat new_rot = glm::rotate(glm::quat(), Yaw, glm::vec3(0.f, 1.f, 0.f));
	new_rot = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
	UpdateRotation(new_rot);
}
