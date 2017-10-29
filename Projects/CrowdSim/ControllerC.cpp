#include "ControllerC.h"

namespace {
	float speed = 1000.0f, mouseSensitivity = 0.5f, Pitch = 0.f, Yaw = 0.f;
	float lastClick = 0.f;
	glm::quat OriginalRot;
}

ControllerC::ControllerC(const char * ID, const char * model, const char * parent) :
	clickedP(false), clickedT(false), clickedSpace(false), clickedRA(false),
	clickedLA(false)
{
	m_ID = ID;
	// store function pointer
	EventHandler update =
		std::bind(&ControllerC::Update, this, std::placeholders::_1);

	OriginalRot = rot();
	AddCallback("post", update);
	AddCallback("input", update);
}

DD_Event ControllerC::Update(DD_Event event) {
	// Parse throught events
	if (event.m_type == "input") {
		glm::vec3 forward = ForwardDir() * speed * event.m_time;
		glm::vec3 right = RightDir() * speed * event.m_time;
		lastClick += event.m_time;

		inputBuff* input = (inputBuff*)event.m_message;

		mouseX = input->mouseX;
		mouseY = input->mouseY;
		if (!ignore_controls) {
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
		}
		
		if (input->rawInput[DD_Keys::P_Key] && lastClick > 0.5f) {
			clickedP = !clickedP;
			lastClick = 0.f;
		}
		if (input->rawInput[DD_Keys::T_Key] && lastClick > 0.5f) {
			clickedT = !clickedT;
			lastClick = 0.f;
		}
		if (input->rawInput[DD_Keys::Space_Key] && lastClick > 0.5f) {
			clickedSpace = !clickedSpace;
			lastClick = 0.f;
		}
		if (input->rawInput[DD_Keys::R_Key] && lastClick > 0.5f) {
			clickedLA = !clickedLA;
			lastClick = 0.f;
		}
		if (input->rawInput[DD_Keys::F_Key] && lastClick > 0.5f) {
			clickedRA = !clickedRA;
			lastClick = 0.f;
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

			//DD_Terminal::post("Pitch: " + std::to_string(Pitch) + 
				//"Yaw: " + std::to_string(Yaw) + "\n");
		}
		if (input->rawInput[DD_Keys::Escape_Key]) {
			clickedEsc = true;
		}
		if (input->mouseLMR[0] && lastClick > 0.5f) {
			clickedLM = true;
			lastClick = 0.f;
		}
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
	glm::quat new_rot = glm::rotate(glm::quat(), Yaw, glm::vec3(0.f, 1.f, 0.f));
	new_rot = glm::rotate(new_rot, Pitch, glm::vec3(1.f, 0.f, 0.f));
	UpdateRotation(new_rot);
	
	return DD_Event();
}

void ControllerC::initRot(const float rad_pitch, const float rad_Yaw) {
	Pitch = rad_pitch;
	Yaw = rad_Yaw;
}
