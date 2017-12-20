#pragma once

#include "DD_MeshTypes.h"

/// \brief Represents agents w/ in engine
struct DD_BaseAgent {
	DD_BaseAgent(const char* ID = "", const char* parent = "");
	~DD_BaseAgent();

	size_t id;
};