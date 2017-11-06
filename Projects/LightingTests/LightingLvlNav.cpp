#include "LightingLvlNav.h"

namespace {
	const size_t buff_size = 64;
	char char_buff[buff_size];

	float scroll_speed = 1000.0f, mouseSensitivity = 1.f, Pitch = 0.f, Yaw = 0.f,
		scroll_dist = 10.f;
	glm::quat deltaR;
}

LightingLvlNav::LightingLvlNav(const char * ID, 
							   const char * model, 
							   const char * parent) :
	b_space(false),
	b_lmouse(false),
	b_rmouse(false),
	b_esc(false),
	b_ctrl_l(false),
	b_alt_l(false),
	b_shift_l(false),
	ignore_controls(false)
{
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&LightingLvlNav::Update, this, std::placeholders::_1);

	AddCallback("post", update);
	AddCallback("input", update);

	// implement my own update to movement
	override_inst_update();
}

DD_Event LightingLvlNav::Update(DD_Event event) 
{
	// Parse throught events
	if (event.m_type == "input") {

		inputBuff* input = (inputBuff*)event.m_message;

		mouseX = input->mouseX;
		mouseY = input->mouseY;

		b_space = input->rawInput[DD_Keys::Space_Key];
		b_esc = input->rawInput[DD_Keys::Escape_Key];
		b_ctrl_l = input->rawInput[DD_Keys::CTRL_L_Key];
		b_alt_l = input->rawInput[DD_Keys::ALT_L_Key];
		b_shift_l = input->rawInput[DD_Keys::Shift_L_Key];
		b_lmouse = input->mouseLMR[0];
		b_rmouse = input->mouseLMR[2];

		if (input->mouseScroll > 0) {
			scroll_dist -= scroll_speed * event.m_time;
		}
		if (input->mouseScroll < 0) {
			scroll_dist += scroll_speed * event.m_time;
		}
		if (b_rmouse) {
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

		if (b_esc) {
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

void LightingLvlNav::initRot(const float rad_pitch, const float rad_Yaw) 
{
	Pitch = rad_pitch;
	Yaw = rad_Yaw;
	glm::quat new_rot = glm::rotate(glm::quat(), Yaw, glm::vec3(0.f, 1.f, 0.f));
	new_rot = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
	UpdateRotation(new_rot);
}
