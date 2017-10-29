#pragma once

#include "DD_Agent.h"

class ControllerDP : public DD_Agent {
public:
	ControllerDP(const char* ID, const char* model = "", const char* parent = "");
	~ControllerDP() {}

	DD_Event Update(DD_Event event);
	void initRot(const float rad_pitch, const float rad_Yaw);

	bool clickedSpace = false, clickedLM = false, clickedEsc = false;
	int mouseX = 0.f, mouseY = 0.f;
private:
};
