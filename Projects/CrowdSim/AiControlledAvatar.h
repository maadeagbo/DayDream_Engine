#pragma once

#include "DD_Agent.h"
#include "DD_ResourceLoader.h"

class AiControlledAvatar : public DD_Agent {
public:
	AiControlledAvatar(const char *ID, const char *ai_ID, DD_Resources *res);
	~AiControlledAvatar() {}

	DD_Event update(DD_Event event);

	DD_Resources *res_ptr;
	float lastClick = 0.f;
	cbuff<64> AiAgent_id;
};