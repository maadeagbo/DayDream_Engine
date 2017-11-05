#pragma once

#include "DD_Agent.h"

class LightingLvlNav : public DD_Agent {
public:
	LightingLvlNav(const char* ID, const char* model = "", const char* parent = "");
	~LightingLvlNav() {}

	DD_Event Update(DD_Event event);
	void initRot(const float rad_pitch, const float rad_Yaw);

	int mouseX = 0.f, mouseY = 0.f;
	bool b_space, b_lmouse, b_rmouse, b_esc, b_ctrl_l, b_alt_l, b_shift_l;
private:
};
