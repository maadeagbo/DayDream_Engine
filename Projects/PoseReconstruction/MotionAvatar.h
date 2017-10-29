#pragma once

#include "DD_Agent.h"
#include "DD_ResourceLoader.h"

struct SkeletonDebug_PV
{
	float boxSize = 1.f;
	glm::vec4 lineColor = glm::vec4(1.f);
	cbuff<64> boxRender;
	cbuff<64> lineRender;
	dd_array<glm::uvec2> lineIdxs;
	DD_Resources* res_ptr;
	void setup(DD_Resources* res,
			   const char* mdl_id,
			   const char* line_id,
			   const char* skeleton_id);
	void render(const dd_array<glm::mat4> mats);
	bool active() const { return flagLoaded; }
private:
	bool flagLoaded = false;
};

class MotionAvatar : public DD_Agent {
public:
	MotionAvatar(const char* ID);
	~MotionAvatar() {}

	DD_Event Update(DD_Event event);
	void setInterface(const float dt);
	void toggleGlobalConfig();
	void load(DD_Resources* res,
			  const char* mdl_sk_id,
			  const char* mdl_sk_path,
			  const char* skeleton_id,
			  const char* skeleton_path);
	bool addAnimation(const char* anim_id,
					  const char* new_ref_id);

	bool clickedP = false, clickedR = false, clickedSpace = false;
	bool ignore_controls = true;
	bool edit_window = false;

	//DD_ModelSK* skinnedMdl;
	DD_Resources* res_ptr;
	SkeletonDebug_PV skeleton_viewer;
	cbuff<64> edit_cmd;
	cbuff<64> skinnedMdl_id;
	float lastClick = 0.f;
	glm::vec3 gui_rot;
	glm::vec4 gui_color;
	cbuff<64> strbuff;
};
