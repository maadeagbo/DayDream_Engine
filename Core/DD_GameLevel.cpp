#include "DD_GameLevel.h"

void DD_GameLevel::AddAgent(DD_Agent * agent)
{
	res_ptr->agents[res_ptr->m_num_agents] = agent;
	res_ptr->m_num_agents += 1;
}

void DD_GameLevel::AddCallback(const char * ticket, EventHandler handler)
{
	// resize if too small
	if( num_handlers >= handlers.size() ) {
		// functions
		dd_array<EventHandler> temp(num_handlers + 5);
		temp = handlers;
		handlers = std::move(temp);
		// tickets
		dd_array<std::string> temp2(num_handlers + 5);
		temp2 = tickets;
		tickets = std::move(temp2);
	}
	handlers[num_handlers] = handler;
	tickets[num_handlers] = ticket;
	num_handlers += 1;
}

DD_Material* DD_GameLevel::GetMaterial(const char* model_ID,
									   const size_t mesh_index,
									   const size_t LOD_lvl)
{
	DD_Material* mat;
	DD_Model* model = ResSpace::findDD_Model(res_ptr, model_ID);
	if( LOD_lvl == 0 ) {
		mat = ResSpace::findDD_Material(res_ptr, model->materials[mesh_index]);
		return mat;
	}
	// fix when lod's work
	return nullptr;
}
