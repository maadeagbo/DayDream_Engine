#include "ControllerW.h"

namespace {
	const size_t buff_size = 64;
	char char_buff[buff_size];

	float speed = 1500.0f, mouseSensitivity = 1.f, Pitch = 0.f, Yaw = 0.f;
	float lastClick = 0.f;
}

ControllerW::ControllerW(const char * ID, const char * model, const char * parent) :
	clickedSpace(false)
{
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&ControllerW::Update, this, std::placeholders::_1);

	AddCallback("post", update);
	AddCallback("input", update);
}

DD_Event ControllerW::Update(DD_Event event) {
	// Parse throught events
	if (event.m_type == "input") {
		glm::vec3 forward = ForwardDir() * speed * event.m_time;
		glm::vec3 right = RightDir() * speed * event.m_time;

		inputBuff* input = (inputBuff*)event.m_message;

		mouseX = input->mouseX;
		mouseY = input->mouseY;

		if (input->rawInput[DD_Keys::W_Key]) {
			UpdatePosition(pos() + forward);
		}
		if (input->rawInput[DD_Keys::A_Key]) {
			UpdatePosition(pos() - right);
		}
		if (input->rawInput[DD_Keys::S_Key]) {
			UpdatePosition(pos() - forward);
		}
		if (input->rawInput[DD_Keys::D_Key]) {
			UpdatePosition(pos() + right);
		}
		if (input->rawInput[DD_Keys::Space_Key]) {
			clickedSpace = !clickedSpace;
		}
		if (input->rawInput[DD_Keys::Escape_Key]) {
			clickedEsc = true;
		}
		if (input->mouseLMR[0]) {
			clickedLM = true;
		}
		if (input->mouseLMR[2]) {
			//printf("X: %d Y: %d     \r", input->mouseXDelta, input->mouseYDelta);
			Pitch += input->mouseYDelta * mouseSensitivity * event.m_time;
			Yaw += input->mouseXDelta * mouseSensitivity * event.m_time;
			// constrain pitch to fps style camera (radians)
			if (Pitch > 1.3f) {
				Pitch = 1.3f;
			}
			if (Pitch < -1.3f) {
				Pitch = -1.3f;
			}
			/*
			DD_Terminal::post("Pitch: " + std::to_string(Pitch) + 
				" Yaw: " + std::to_string(Yaw) + "\n");
			//*/
		}
		glm::mat4 new_rot = glm::rotate(glm::mat4(), Yaw, glm::vec3(0.f, 1.f, 0.f));
		new_rot = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
		UpdateRotation(glm::quat(new_rot));

		//DD_Terminal::post(("Time: " + std::to_string(event.m_time)).c_str());
		/*
		snprintf(char_buff, buff_size, "Pitch: %02d, Yaw: %02d", input->mouseYDelta,
			input->mouseXDelta);
		DD_Terminal::post(char_buff);
		//*/
	}
	if (event.m_type == "post") {
		if (clickedEsc) {
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

void ControllerW::initRot(const float rad_pitch, const float rad_Yaw) {
	Pitch = rad_pitch;
	Yaw = rad_Yaw;
	glm::quat new_rot = glm::rotate(glm::quat(), Yaw, glm::vec3(0.f, 1.f, 0.f));
	new_rot = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
	UpdateRotation(new_rot);
}
