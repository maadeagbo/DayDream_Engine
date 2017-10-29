#pragma once

#include "DD_Agent.h"

enum NavUI {
	SPACE,
	LMOUSE,
	RMOUSE,
	ESC,
	CTRL_L,
	ALT_L,
	SHIFT_L,
	COUNT
};

class Navigation : public DD_Agent {
public:
	Navigation(const char* ID, const char* model = "", const char* parent = "");
	~Navigation() {}

	DD_Event Update(DD_Event event);
	void initRot(const float rad_pitch, const float rad_Yaw);

	bool pressed[NavUI::COUNT];
	int mouseX = 0.f, mouseY = 0.f;
private:
};
