#include "AiControlledAvatar.h"

AiControlledAvatar::AiControlledAvatar(const char *ID, 
									   const char *ai_ID, 
									   DD_Resources* res)
{
	m_ID = ID;
	AiAgent_id = ai_ID;
	res_ptr = res;

	EventHandler update =
		std::bind(&AiControlledAvatar::update, this, std::placeholders::_1);
	AddCallback("update_ai", update);
	override_inst_update();
}

DD_Event AiControlledAvatar::update(DD_Event event)
{
	if (event.m_type == "update_ai") {
		// grab AI_Agent
		AI_Agent* ai = ResSpace::findAI_Agent(res_ptr, AiAgent_id.str());
		if (ai) {
			if (ai->move_now) {
				glm::vec3 p = glm::vec3(ai->current_pos);
				p += (ai->curr_vel) * event.m_time;
				glm::mat4 m = glm::translate(glm::mat4(), p);
				m = glm::scale(m, size());
				ai->current_pos = p;
				inst_m4x4[0] = m;
			}
			else {
				glm::mat4 m = glm::translate(glm::mat4(), ai->current_pos);
				m = glm::scale(m, size());
				inst_m4x4[0] = m;
			}
		}
	}
	return DD_Event();
}
