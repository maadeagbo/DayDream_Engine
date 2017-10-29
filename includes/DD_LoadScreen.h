#pragma once

#include "DD_Types.h"
#include "DD_Text.h"
#include "DD_Shader.h"

namespace LoadScrSpace {
	bool LoadTextures(const int _screenW, const int _screenH);
	void DrawLoadScreen(float timeElasped, DD_Shader* shader);
	DD_Text GetArialTTF();
}
