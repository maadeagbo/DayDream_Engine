#pragma once

#include "DD_MathLib.h"
#include "DD_GameLevel.h"
#include "ControllerW.h"

class WaterLevel : public DD_GameLevel
{
public:
	WaterLevel() {}
	~WaterLevel();

	void Init();
	void setInterface(const float dt);
	DD_Event basePost(DD_Event& event);
private:

};