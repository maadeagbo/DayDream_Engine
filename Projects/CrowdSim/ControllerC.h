#pragma once

#include "DD_Agent.h"

class ControllerC : public DD_Agent {
public:
	ControllerC(const char* ID, const char* model = "", const char* parent = "");
	~ControllerC() {}

	DD_Event Update(DD_Event event);
	void initRot(const float rad_pitch, const float rad_Yaw);

	bool clickedP = false, clickedT = false, clickedSpace = false, 
		clickedRA = false, clickedLA = false, clickedLM = false, 
		clickedEsc = false, ignore_controls = false;
	int mouseX = 0.f, mouseY = 0.f;
private:

};
