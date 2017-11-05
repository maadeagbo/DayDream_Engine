#pragma once

#include "DD_MathLib.h"
#include "DD_GameLevel.h"
#include "LightingLvlNav.h"

class LightingLvl : public DD_GameLevel
{
public:
	LightingLvl() {}
	~LightingLvl();

	void Init();
	void setInterface(const float dt);
	DD_Event basePost(DD_Event& event);
private:

};