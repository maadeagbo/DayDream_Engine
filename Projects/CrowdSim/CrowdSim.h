#pragma once

#include "DD_MathLib.h"
#include "DD_GameLevel.h"
#include "ControllerC.h"
#include "AiControlledAvatar.h"

enum class CS_Scenario : unsigned
{
	A,
	B,
	C,
	NUM_SCENARIOS
};

class CrowdSim : public DD_GameLevel
{
public:
	CrowdSim() {}
	~CrowdSim() {}

	void Init();
	void setInterface();
	DD_Event basePost(DD_Event& event);

	void createNewAgent(const glm::vec3 _A, 
						const glm::vec3 _B, 
						const unsigned mat_idx = 0);
	void initialize_aiobject(const char* ground_id, const char* obstacle_id);

	void scenario01(const size_t num_obst, 
					const float floor_x, 
					const float floor_z);
	void scenario02(const size_t num_obst, 
					const float floor_x, 
					const float floor_z);
	void scenario03(const float floor_x, const float floor_z);

	void deleteCurrentScenario();
};