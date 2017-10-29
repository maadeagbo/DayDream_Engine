#pragma once

#include "DD_Agent.h"

class ControllerP : public DD_Agent {
public:
	ControllerP(const char* ID, const char* model = "", const char* parent = "");
	~ControllerP() {}

	DD_Event Update(DD_Event event);
	void initRot(const float rad_pitch, const float rad_Yaw);

	bool clickedP = false, clickedT = false, clickedSpace = false, 
		clickedRA = false, clickedLA = false, clickedLM = false, clickedEsc = false;
	int mouseX = 0.f, mouseY = 0.f;
	bool ignore_controls;
private:

};
