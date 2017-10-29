#pragma once

#include "DD_MathLib.h"
#include "DD_GameLevel.h"
#include "Navigation.h"

class LevelBuilder : public DD_GameLevel
{
public:
	LevelBuilder() {}
	~LevelBuilder();

	void Init();
	void setInterface(const float dt);
	DD_Event basePost(DD_Event& event);
private:

};