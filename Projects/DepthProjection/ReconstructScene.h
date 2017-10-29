#pragma once

#include "DD_MathLib.h"
#include "DD_GameLevel.h"
#include "ControllerDP.h"

class ReconstructScene : public DD_GameLevel
{
public:
	ReconstructScene() {}
	~ReconstructScene();

	void Init();
	void setInterface(const float dt);
	size_t GetPNGFiles(const char* direc);
	void CreateTexForFrame(const size_t index);
	DD_Event basePost(DD_Event& event);
private:

};