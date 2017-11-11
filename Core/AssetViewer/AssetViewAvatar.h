#pragma once

#include "DD_Agent.h"
#include "DD_ResourceLoader.h"

class AssetViewAvatar : public DD_Agent {
public:
	AssetViewAvatar(const char* ID);
	~AssetViewAvatar() {}

	DD_Event Update(DD_Event event);
	void setInterface(const float dt);
	void appendAnimation(const char *animation_id, const char* new_ref_id);

	bool clickedP = false, clickedR = false, clickedSpace = false;
	bool ignore_controls = true;
	bool edit_window = false;

	DD_Resources* res_ptr;
	cbuff<64> strbuff;
	cbuff<64> skinnedMdl_id;
	float lastClick = 0.f;
	glm::vec3 gui_rot;
};