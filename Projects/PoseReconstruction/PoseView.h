#pragma once

#include "DD_MathLib.h"
#include "DD_GameLevel.h"
#include "MotionAvatar.h"

class PoseView : public DD_GameLevel
{
public:
	PoseView() {}
	~PoseView() {}

	void Init();
	void setInterface(const float dt);
	DD_Event Update(DD_Event& event);
private:
	bool loadAnimDDA(const char* anim_id, const char* path);
	bool loadSkeletonDDB(const char* sk_id, const char* path);
	bool createAgent(const char* agent_id, const char* sk_id);
	bool addAnimationToAgent(const char* agent_id, 
							 const char* anim_id, 
							 const char* new_state_id);
	void deleteAgent(const char* agent_id);
	void playAll();
	void pauseAll();
	
	unsigned total_agents;
};